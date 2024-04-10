/*
-------------------------------------------------------------------------------
--
-- Definition of Ports
-- ACLK           : Synchronous clock
-- ARESETN        : System reset, active low
-- S_AXIS_TREADY  : Ready to accept data in
-- S_AXIS_TDATA   : Data in 
-- S_AXIS_TLAST   : Optional data in qualifier
-- S_AXIS_TVALID  : Data in is valid
-- M_AXIS_TVALID  : Data out is valid
-- M_AXIS_TDATA   : Data Out
-- M_AXIS_TLAST   : Optional data out qualifier
-- M_AXIS_TREADY  : Connected slave device is ready to accept data out
--
-------------------------------------------------------------------------------
*/

module myip_v1_0 
	(
		ACLK,
		ARESETN,
		S_AXIS_TREADY,
		S_AXIS_TDATA,
		S_AXIS_TLAST,
		S_AXIS_TVALID,
		M_AXIS_TVALID,
		M_AXIS_TDATA,
		M_AXIS_TLAST,
		M_AXIS_TREADY
	);

	/***************************************** I/O *****************************************/
	input					ACLK;    // Synchronous clock
	input					ARESETN; // System reset, active low

	// Slave in interface
	output	reg				S_AXIS_TREADY;  // Ready to accept data in
	input	[31 : 0]		S_AXIS_TDATA;   // Data in
	input					S_AXIS_TLAST;   // Optional data in qualifier
	input					S_AXIS_TVALID;  // Data in is valid

	// Master out interface
	output	reg				M_AXIS_TVALID;  // Data out is valid
	output	wire [31 : 0]	M_AXIS_TDATA;   // Data Out
	output	reg				M_AXIS_TLAST;   // Optional data out qualifier
	input					M_AXIS_TREADY;  // Connected slave device is ready to accept data out

	/***************************************** RAM *****************************************/
	localparam A_depth_bits = 9;  	// A is a 64x7 matrix
	localparam B_depth_bits = 4; 	// B is a 8x2 matrix
	localparam C_depth_bits = 2; 	// C is a 3x1 matrix
	localparam RES_depth_bits = 6;	// RES is a 64x1 matrix
	localparam width = 8;			// PS sends 32bit data along AXI-4, but the 'underlying' data is actually 8bit

	localparam NUMBER_OF_A_WORDS = 448;
	localparam NUMBER_OF_B_WORDS = 2**B_depth_bits;
	localparam NUMBER_OF_C_WORDS = 3;

	localparam NUMBER_OF_INPUT_WORDS  = NUMBER_OF_A_WORDS + NUMBER_OF_B_WORDS + NUMBER_OF_C_WORDS;	// Total number of input data.
	localparam NUMBER_OF_OUTPUT_WORDS = 2**RES_depth_bits;	                    					// Total number of output data
	
	reg A_write_en = 0;							// myip_v1_0 -> A_RAM.
	reg	[A_depth_bits-1:0] A_write_address;		// myip_v1_0 -> A_RAM.
	reg	[width-1:0] A_write_data_in;			// myip_v1_0 -> A_RAM.
	wire A_read_en;								// Inference_0 -> A_RAM.
	wire [A_depth_bits-1:0] A_read_address;		// Inference_0 -> A_RAM.
	wire [width-1:0] A_read_data_out;			// A_RAM -> Inference_0.

	reg	B_write_en = 0;					 		// myip_v1_0 -> B_RAM.
	reg	[B_depth_bits-1:0] B_write_address;		// myip_v1_0 -> B_RAM.
	reg	[width-1:0] B_write_data_in;			// myip_v1_0 -> B_RAM.
	wire B_read_en;								// Inference_0 -> B_RAM.
	wire [B_depth_bits-1:0] B_read_address;		// Inference_0 -> B_RAM.
	wire [width-1:0] B_read_data_out;			// B_RAM -> Inference_0.

	reg	C_write_en = 0;					 		// myip_v1_0 -> C_RAM.
	reg	[C_depth_bits-1:0] C_write_address;		// myip_v1_0 -> C_RAM.
	reg	[width-1:0] C_write_data_in;			// myip_v1_0 -> C_RAM.
	wire C_read_en;								// Inference_0 -> C_RAM.
	wire [C_depth_bits-1:0] C_read_address;		// Inference_0 -> C_RAM.
	wire [width-1:0] C_read_data_out;			// C_RAM -> Inference_0.

	wire RES_write_en;								// Inference_0 -> RES_RAM.
	wire [RES_depth_bits-1:0] RES_write_address;	// Inference_0 -> RES_RAM.
	wire [width-1:0] RES_write_data_in;				// Inference_0 -> RES_RAM.
	reg RES_read_en = 0;  							// myip_v1_0 -> RES_RAM. 
	reg [RES_depth_bits-1:0] RES_read_address;		// myip_v1_0 -> RES_RAM. 
	wire [width-1:0] RES_read_data_out;				// RES_RAM -> myip_v1_0

	/***************************************** STATE *****************************************/
	reg	Inference_Start; 							 	// myip_v1_0 -> Inference_0
	wire Inference_Done;							    // Inference_0 -> myip_v1_0. 
			
	// Define the states of state machine (one hot encoding)
	reg [3:0] state;
	localparam Idle  = 4'b1000;
	localparam Read_Inputs = 4'b0100;
	localparam Compute = 4'b0010;
	localparam Write_Outputs  = 4'b0001;

	// Counters to store the number inputs read & outputs written.
	// Could be done using the same counter if reads and writes are not overlapped (i.e., no dataflow optimization)
	// Left as separate for ease of debugging
	reg [$clog2(NUMBER_OF_INPUT_WORDS) - 1:0] read_counter = 0;
	reg [$clog2(NUMBER_OF_OUTPUT_WORDS):0] write_counter = 0;
	localparam NUM_CYCLES_FILL_RES_RAM_PIPELINE = 2;

	/***************************************** FSM *****************************************/
   // CAUTION:
   // The sequence in which data are read in and written out should be
   // consistent with the sequence they are written and read in the driver's hw_acc.c file

	// STATE MACHINE implemented as a single-always Moore machine
	// a Mealy machine that asserts S_AXIS_TREADY and captures S_AXIS_TDATA etc can save a clock cycle
	assign M_AXIS_TDATA = RES_read_data_out;

	reg is_deasserted = 1'b1;
	reg [$clog2(NUMBER_OF_OUTPUT_WORDS):0] write_counter_prev = 0;
	reg is_M_AXIS_pipeline_filling = 1'b1;

	always @(posedge ACLK) 
	begin
		/****** Synchronous reset (active low) ******/
		if (!ARESETN)
		begin
			// CAUTION: make sure your reset polarity is consistent with the system reset polarity
			state        <= Idle;
        end
		else
		begin
			case (state)

				Idle:
				begin
					read_counter 	<= 0;
					write_counter   <= 0;
					S_AXIS_TREADY 	<= 0;
					M_AXIS_TVALID 	<= 0;
					M_AXIS_TLAST  	<= 0;

					if (S_AXIS_TVALID == 1)    // MASTER->SLAVE: Data placed by MASTER on TDATA is valid
					begin
						// SLAVE: Only ready to accept data when signalled by MASTER
						// Note : This is not really how AXIS works (SLAVE can be ready without indication from MASTER)
						state       	<= Read_Inputs;

						// TODO: Below two comments may be outdated...
						// Need to raise S_AXIS_TREADY before entering Read_Inputs
						// Otherwise, Coprocessor (Slave) will have first value as XX, as it immediately reads from Coprocessor when it is not ready (under Read_Inputs)
						S_AXIS_TREADY 	<= 1;    // SLAVE->MASTER: Indication from SLAVE to MASTER that SLAVE is accepting data
					end
				end


				Read_Inputs:
				begin
					Inference_Start <= 0;

					// S_AXIS_TVALID comes from testbench (acting as Master)
					if (S_AXIS_TVALID) begin    // Transaction only occurs if TREADY & TVALID are asserted simultaneously
						// If Master is placing valid data, then coprocessor (acting as Slave) will change to be ready to accept
						// Note that this is not AXI specification, Slave can assert S_AXIS_TREADY at anytime
						S_AXIS_TREADY 	<= 1;    // SLAVE->MASTER: Indication from SLAVE to MASTER that SLAVE is accepting data
					end
					else begin
						// S_AXIS_TVALID is deasserted by testbench (acting as Master)
						// Slave should not be accepting data. Testbench will stop setting and sending S_AXIS_TDATA
						// No need to disable X_write_en, the one to watch out for is X_write_address
						S_AXIS_TREADY <= 0;
					end


					if (S_AXIS_TVALID && S_AXIS_TREADY) begin
						// Read and store values into RAM (RAM_A & RAM_B) first
						if (read_counter < NUMBER_OF_A_WORDS) begin
							// Incoming data belongs to 'A' matrix
							A_write_en <= 1;
							A_write_address <= read_counter;
							A_write_data_in <= S_AXIS_TDATA[width-1:0];
						end 
						else if (NUMBER_OF_A_WORDS <= read_counter
								&& read_counter < (NUMBER_OF_A_WORDS + NUMBER_OF_B_WORDS)) begin
							// Incoming data belongs to 'B' matrix
							B_write_en <= 1; A_write_en <= 0;
							B_write_address <= read_counter - NUMBER_OF_A_WORDS;
							B_write_data_in <= S_AXIS_TDATA[width-1:0];
						end
						else begin
							// Incoming data belongs to 'C' matrix
							C_write_en <= 1; B_write_en <= 0; A_write_en <= 0;
							C_write_address <= read_counter - NUMBER_OF_A_WORDS - NUMBER_OF_B_WORDS;
							C_write_data_in <= S_AXIS_TDATA[width-1:0];
						end

						// Still continue filling RAM_A/RAM_B/RAM_C
						// Note we will keep filling as long as not last element
						read_counter <= read_counter + 1;
					end

					/*
					Potential problem : S_AXIS_TVALID is deasserted while we still need to write to RAM
					i.e - We have captured the value of S_AXIS_TDATA already, but we still need an additional 1 clock cycle to write to RAM
					    - Hence, C_write_en <= 0 and State <= Compute must be OUTSIDE of the check for S_AXIS_TVALID, to ensure that the last value is
						  captured in C_RAM
					*/

					// If we are expecting a variable number of words, we should make use of S_AXIS_TLAST.
					if (read_counter == NUMBER_OF_INPUT_WORDS)
					begin
						// RAM_A & RAM_B filled, can begin Matrix Multiplication
						S_AXIS_TREADY 	<= 0;	// SLAVE->MASTER: Indication from SLAVE to MASTER that SLAVE not accepting data
						state      		<= Compute;
						Inference_Start    <= 1;   // Pulse Inference_Start to begin 'Compute' phase
						C_write_en <= 0;
						read_counter <= 0;
					end
				end


				// Begin 'Compute' phase if Inference_Start is pulsed
				Compute:
				begin
					Inference_Start <= 0;          // De-pulse Inference_Start (we only hold it HIGH for 1 cycle)

					if (Inference_Done) begin
						state <= Write_Outputs;    // This will cause Inference_Start to be deasserted as well
					end

					// Possible to save a cycle by asserting M_AXIS_TVALID and presenting M_AXIS_TDATA just before going into Write_Outputs state.
					// Alternatively, M_AXIS_TVALID and M_AXIS_TDATA can be asserted combinationally to save a cycle.
				end


				/*
				Assume that M_AXIS_TREADY is asserted by testbench already

				0th clock cycle
					- Request for 0th element in RES RAM

				1st clock cycle
					- Request for 1st element in RES RAM
					- RES RAM receives request for 0th element
						- RES RAM will output 0th element in next cycle
					- M_AXIS_TVALID signalled to be pulled high on next cycle

				2nd clock cycle
					- 0th element arrives (outputs to M_AXIS_TDATA)
					- RES RAM receives request for 1st element
					- M_AXIS_TVALID pulled high
					- M_AXIS_TLAST signalled to be pulled high on next cycle

				3rd clock cycle
					- 1st element arrives (outputs to M_AXIS_TDATA)
					- M_AXIS_TLAST pulled high
					- M_AXIS_TLAST signalled to be pulled low on next cycle
					- M_AXIS_TVALID signalled to be pulled low on next cycle

				4th clock cycle
					- Transit to Idle
				*/

				// Contents of RES RAM to be sent out through M_AXIS_TDATA (We must read RAM synchronously!)
				// assign M_AXIS_TDATA = RES_read_data_out;
				Write_Outputs:
				begin
					// M_AXIS_TREADY comes from testbench (as Slave)
					if (M_AXIS_TREADY == 1) begin    // SLAVE->MASTER: Slave is ready to accept data
						/*
						 - MASTER->SLAVE: Master is sending valid data from 2nd cycle onwards (since we need to wait for RES RAM)
						 - Master only begins the process of sending valid data, if Slave is ready to accept it. 
						 - Actually Master can send data even if Slave is not ready (ie M_AXIS_TREADY not asserted)
							- However, Master still needs to check if slave has received the previous data, before sending something new
							- Therefore, instead of repeatedly sending the same data and checking if slave has received it before sending something new,
							  we just check M_AXIS_TREADY

						 - Note, M_AXIS_TVALID will be deasserted in IDLE stage
						*/

						M_AXIS_TVALID <= (is_M_AXIS_pipeline_filling) ? 0 : 1;

						//M_AXIS_TVALID <= (write_counter >= NUM_CYCLES_FILL_RES_RAM_PIPELINE-1) ? 1 : 0;
						is_deasserted <= 1;

						if (write_counter == NUMBER_OF_OUTPUT_WORDS) begin
							// M_AXIS_TLAST, though optional in AXIS, is necessary in practice as AXI Stream FIFO and AXI DMA expects it.
							// M_AXIS_TLAST tells AXI_S2MM DMA that a transaction (of a packet) is done 
							// XAxiDma_SimpleTransfer() uses this to work properly
							M_AXIS_TLAST <= 1; 
						end
						else begin
							// 1 cycle for RES_read_address to update
							// Another 1 cycle for RES RAM to produce read_data_out
							// M_AXIS_TDATA is ASSIGNED to RES_read_data_out

							RES_read_en <= 1;
							RES_read_address <= write_counter;
							write_counter <= write_counter+1;
							write_counter_prev <= write_counter;
							is_M_AXIS_pipeline_filling <= 0;

							if (!is_deasserted) begin
								is_deasserted <= 1;
							end
						end

						if (M_AXIS_TLAST == 1) begin
							is_M_AXIS_pipeline_filling <= 1;
							state <= Idle;
							M_AXIS_TLAST <= 0;
							M_AXIS_TVALID <= 0;
							RES_read_en <= 0;
						end
					end
					else begin
						// Prevent synthesis mismatch with behavioural
						// Synthesis will somehow jump an address value (e.g. going from 3 to 5 straightaway)
						//if (is_deasserted) begin
							//write_counter = write_counter_prev+1;
							//RES_read_en <= 0;
							//M_AXIS_TVALID <= 0;
							//is_deasserted <= 0;
							//write_counter <= 4;
						//end

						//RES_read_en <= 0;
						M_AXIS_TVALID <= 0;

						// Need to retransmit the CURRENT M_AXIS_TDATA value to Slave when it is ready again (i.e when M_AXIS_TREADY asserted)
						if (is_deasserted) begin
							RES_read_address <= write_counter_prev-1;
							write_counter <= write_counter_prev-1;
							write_counter_prev <= write_counter_prev-1;
							is_M_AXIS_pipeline_filling <= 1;
						end
						is_deasserted <= 0;
					end
				end
			endcase
		end
	end

	/***************************************** INSTANTIATION *****************************************/
	// RAM
	memory_RAM 
	#(
		.width(width), 
		.depth_bits(A_depth_bits)
	) A_RAM 
	(
		.clk(ACLK),
		.write_en(A_write_en),
		.write_address(A_write_address),
		.write_data_in(A_write_data_in),
		.read_en(A_read_en),    
		.read_address(A_read_address),
		.read_data_out(A_read_data_out)
	);
										
										
	memory_RAM 
	#(
		.width(width), 
		.depth_bits(B_depth_bits)
	) B_RAM 
	(
		.clk(ACLK),
		.write_en(B_write_en),
		.write_address(B_write_address),
		.write_data_in(B_write_data_in),
		.read_en(B_read_en),    
		.read_address(B_read_address),
		.read_data_out(B_read_data_out)
	);

	memory_RAM 
	#(
		.width(width), 
		.depth_bits(C_depth_bits)
	) C_RAM 
	(
		.clk(ACLK),
		.write_en(C_write_en),
		.write_address(C_write_address),
		.write_data_in(C_write_data_in),
		.read_en(C_read_en),    
		.read_address(C_read_address),
		.read_data_out(C_read_data_out)
	);
										
	memory_RAM 
	#(
		.width(width), 
		.depth_bits(RES_depth_bits)
	) RES_RAM 
	(
		.clk(ACLK),
		.write_en(RES_write_en),
		.write_address(RES_write_address),
		.write_data_in(RES_write_data_in),
		.read_en(RES_read_en),    
		.read_address(RES_read_address),
		.read_data_out(RES_read_data_out)
	);

	// Inference Unit
	Inference 
	#(
		.width(width), 
		.A_depth_bits(A_depth_bits), 
		.B_depth_bits(B_depth_bits), 
		.C_depth_bits(C_depth_bits),
		.RES_depth_bits(RES_depth_bits) 
	) Inference_0
	(									
		.clk(ACLK),
		.Start(Inference_Start),
		.Done(Inference_Done),
		
		.A_read_en(A_read_en),
		.A_read_address(A_read_address),
		.A_read_data_out(A_read_data_out),
		
		.B_read_en(B_read_en),
		.B_read_address(B_read_address),
		.B_read_data_out(B_read_data_out),

		.C_read_en(C_read_en),
		.C_read_address(C_read_address),
		.C_read_data_out(C_read_data_out),
		
		.RES_write_en(RES_write_en),
		.RES_write_address(RES_write_address),
		.RES_write_data_in(RES_write_data_in)
	);

endmodule