/* 
----------------------------------------------------------------------------------
--	(c) Rajesh C Panicker, NUS
--  Description : Matrix Multiplication AXI Stream Coprocessor. Based on the orginal AXIS Coprocessor template (c) Xilinx Inc
-- 	Based on the orginal AXIS coprocessor template (c) Xilinx Inc
--	License terms :
--	You are free to use this code as long as you
--		(i) DO NOT post a modified version of this on any public repository;
--		(ii) use it only for educational purposes;
--		(iii) accept the responsibility to ensure that your implementation does not violate any intellectual property of any entity.
--		(iv) accept that the program is provided "as is" without warranty of any kind or assurance regarding its suitability for any particular purpose;
--		(v) send an email to rajesh.panicker@ieee.org briefly mentioning its use (except when used for the course EE4218 at the National University of Singapore);
--		(vi) retain this notice in this file or any files derived from this.
----------------------------------------------------------------------------------
*/
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
		// DO NOT EDIT BELOW THIS LINE ////////////////////
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
		// DO NOT EDIT ABOVE THIS LINE ////////////////////
	);

	input					ACLK;    // Synchronous clock
	input					ARESETN; // System reset, active low
	// slave in interface
	output	reg				S_AXIS_TREADY;  // Ready to accept data in
	input	[31 : 0]		S_AXIS_TDATA;   // Data in
	input					S_AXIS_TLAST;   // Optional data in qualifier
	input					S_AXIS_TVALID;  // Data in is valid
	// master out interface
	output	reg				M_AXIS_TVALID;  // Data out is valid
	output	wire [31 : 0]	M_AXIS_TDATA;   // Data Out
	output	reg				M_AXIS_TLAST;   // Optional data out qualifier
	input					M_AXIS_TREADY;  // Connected slave device is ready to accept data out

//----------------------------------------
// Implementation Section
//----------------------------------------

// RAM parameters
	localparam A_depth_bits = 9;  	// A is a 64x8 matrix
	localparam B_depth_bits = 3; 	// B is a 8x1 matrix
	localparam RES_depth_bits = 6;	// RES is a 64x1 matrix
	localparam width = 8;			// all 8-bit data
	localparam NUMBER_OF_A_WORDS = 2**A_depth_bits;
	localparam NUMBER_OF_B_WORDS = 2**B_depth_bits;
	localparam NUMBER_OF_INPUT_WORDS  = NUMBER_OF_A_WORDS + NUMBER_OF_B_WORDS;	// Total number of input data.
	localparam NUMBER_OF_OUTPUT_WORDS = 2**RES_depth_bits;	                    // Total number of output data
	
// wires (or regs) to connect to RAMs and matrix_multiply_0 for assignment 1
// those which are assigned in an always block of myip_v1_0 shoud be changes to reg.
	reg  	A_write_en = 0;								// myip_v1_0 -> A_RAM. To be assigned within myip_v1_0. Possibly reg.
	reg	[A_depth_bits-1:0] A_write_address;		    // myip_v1_0 -> A_RAM. To be assigned within myip_v1_0. Possibly reg. 
	reg	[width-1:0] A_write_data_in;			    // myip_v1_0 -> A_RAM. To be assigned within myip_v1_0. Possibly reg.
	wire	A_read_en;								// matrix_multiply_0 -> A_RAM.
	wire	[A_depth_bits-1:0] A_read_address;		// matrix_multiply_0 -> A_RAM.
	wire	[width-1:0] A_read_data_out;			// A_RAM -> matrix_multiply_0.

	reg	B_write_en = 0;								    // myip_v1_0 -> B_RAM. To be assigned within myip_v1_0. Possibly reg.
	reg	[B_depth_bits-1:0] B_write_address;		    // myip_v1_0 -> B_RAM. To be assigned within myip_v1_0. Possibly reg.
	reg	[width-1:0] B_write_data_in;			    // myip_v1_0 -> B_RAM. To be assigned within myip_v1_0. Possibly reg.
	wire	B_read_en;								// matrix_multiply_0 -> B_RAM.
	wire	[B_depth_bits-1:0] B_read_address;		// matrix_multiply_0 -> B_RAM.
	wire	[width-1:0] B_read_data_out;			// B_RAM -> matrix_multiply_0.

	wire	RES_write_en;							// matrix_multiply_0 -> RES_RAM.
	wire	[RES_depth_bits-1:0] RES_write_address;	// matrix_multiply_0 -> RES_RAM.
	wire	[width-1:0] RES_write_data_in;			// matrix_multiply_0 -> RES_RAM.
	reg	    RES_read_en = 0;  						// myip_v1_0 -> RES_RAM. To be assigned within myip_v1_0. Possibly reg.
	reg	    [RES_depth_bits-1:0] RES_read_address;	// myip_v1_0 -> RES_RAM. To be assigned within myip_v1_0. Possibly reg.
	wire	[width-1:0] RES_read_data_out;			// RES_RAM -> myip_v1_0
	
	// wires (or regs) to connect to matrix_multiply for assignment 1
	reg	Matrix_Start; 							 	// myip_v1_0 -> matrix_multiply_0. To be assigned within myip_v1_0. Possibly reg.
	wire Matrix_Done;							    // matrix_multiply_0 -> myip_v1_0. 
			
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

   // CAUTION:
   // The sequence in which data are read in and written out should be
   // consistent with the sequence they are written and read in the driver's hw_acc.c file

// STATE MACHINE implemented as a single-always Moore machine
// a Mealy machine that asserts S_AXIS_TREADY and captures S_AXIS_TDATA etc can save a clock cycle
	assign M_AXIS_TDATA = RES_read_data_out;

	reg is_deasserted = 1'b1;
	reg [$clog2(NUMBER_OF_OUTPUT_WORDS):0] write_counter_prev = 0;

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
						S_AXIS_TREADY 	<= 1;    // SLAVE->MASTER: Indication from SLAVE to MASTER that SLAVE is accepting data
					end
				end


				Read_Inputs:
				begin
					Matrix_Start <= 0;

					// S_AXIS_TVALID comes from testbench (acting as Master)
					if (S_AXIS_TVALID == 1) begin    // Transaction only occurs if TREADY & TVALID are asserted simultaneously
						// If Master is placing valid data, then coprocessor (acting as Slave) will be ready to accept
						// Note that this is not AXI specification, Slave can assert S_AXIS_TREADY at anytime

						S_AXIS_TREADY 	<= 1;    // SLAVE->MASTER: Indication from SLAVE to MASTER that SLAVE is accepting data
						$display("S_TVALID and S_TREADY simult, %0t", $time);
						// Intuitively, all below code is like (S_AXIS_TVALID && S_AXIS_TREADY) of conventional AXI transfer

						// Slave (Coprocessor) should CAPTURE data if S_AXIS_TVALID and S_AXIS_TREADY is both true
						// Note: Capturing data and writing data to RAM is not the same 'transaction'

						// Read and store values into RAM (RAM_A & RAM_B) first
						if (read_counter < NUMBER_OF_A_WORDS) begin
							// Incoming data belongs to 'A' matrix
							A_write_en <= 1;
							A_write_address <= read_counter;
							A_write_data_in <= S_AXIS_TDATA[width-1:0];
						end 
						else begin
							// Incoming data belongs to 'B' matrix
							B_write_en <= 1; A_write_en <= 0;
							B_write_address <= read_counter - NUMBER_OF_A_WORDS;
							B_write_data_in <= S_AXIS_TDATA[width-1:0];
						end

						// Still continue filling RAM_A/RAM_B
						// Note we will keep filling as long as not last element
						read_counter <= read_counter + 1;
					end
					else begin
						// S_AXIS_TVALID is deasserted by testbench (acting as Master)

						// Slave should not be accepting data. Testbench will stop setting and sending S_AXIS_TDATA
						// No need to disable X_write_en, the one to watch out for is X_write_address
						S_AXIS_TREADY <= 0;
					end

					/*
					Potential problem : S_AXIS_TVALID is deasserted while we still need to write to RAM
					i.e - We have captured the value of S_AXIS_TDATA already, but we still need an additional 1 clock cycle to write to RAM
					    - Hence, B_write_en <= 0 and State <= Compute must be OUTSIDE of the check for S_AXIS_TVALID, to ensure that the last value is
						  captured in B_RAM
					*/

					// If we are expecting a variable number of words, we should make use of S_AXIS_TLAST.
					if (read_counter == NUMBER_OF_INPUT_WORDS)
					begin
						// RAM_A & RAM_B filled, can begin Matrix Multiplication
						S_AXIS_TREADY 	<= 0;	// SLAVE->MASTER: Indication from SLAVE to MASTER that SLAVE not accepting data
						state      		<= Compute;
						Matrix_Start    <= 1;   // Pulse Matrix_Start to begin 'Compute' phase
						B_write_en <= 0;
						read_counter <= 0;
					end
				end


				// Begin 'Compute' phase if Matrix_Start is pulsed
				Compute:
				begin
					Matrix_Start <= 0;             // De-pulse Matrix_Start (we only hold it HIGH for 1 cycle)

					if (Matrix_Done) begin
						// Finished computing when Matrix Multiply signals DONE
						//$display("About to change state, %0t", $time);
						//$strobe("STROBE state, %0t 0h", $time, state);
						state <= Write_Outputs;    // This will cause Matrix_Start to be deasserted as well
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

						M_AXIS_TVALID <= (write_counter >= NUM_CYCLES_FILL_RES_RAM_PIPELINE-1) ? 1 : 0;
						//is_deasserted <= 1;

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

							//$display("DISP is_deasserted, %0h %0t", is_deasserted, $time);
							//$strobe("STROBE is_deasserted, %0h %0t", is_deasserted, $time);

							RES_read_en <= 1;
							RES_read_address <= write_counter;
							write_counter <= write_counter+1;
							write_counter_prev <= write_counter;

							if (!is_deasserted) begin
								$display("DISP RES_read_address, %0h %0t", RES_read_address, $time);
								is_deasserted <= 1;
							end
						end

						if (M_AXIS_TLAST == 1) begin
							state <= Idle;
							M_AXIS_TLAST <= 0;
							M_AXIS_TVALID <= 0;
							RES_read_en <= 0;
						end
					end
					else begin
						/*	- In Vivado waveform, we see that write_counter is 19 at 105_450ns
								- But if M_AXIS_TREADY is pulled low at 105_450ns, then...
									- We encounter "write_counter <= 19" at 105_450ns
									- write_counter value only gets updated at end of time step
									- Thus we should see write_counter==19 in the waveforms only at 105_550ns (next clock cycle)

							- 105_450ns, M_AXIS_TREADY is pulled low via BLOCKING ASSIGNMENT
							- 105_450ns, Since posedge of CLK, we enter this else statement here (note we enter immediately since before was BA)
							- 105_450ns, $display shows write_counter is 5 (note $display is in Active Queue)
							- 105_450ns, $strobe shows write_counter is 19 (note $strobe displays value at END of timestep), and is in postponed queue
								- Thus, we first evaluate RHS of NBA (e.g evaluate 19)
								- Then, we update LHS of NBA (after all active events are done)
								- Then, before we move to next time delta, we display it using $strobe

							- TLDR, we should have pulled M_AXIS_TREADY low using a NON-BLOCKING ASSIGNMENT
								- Verilog waveforms also display the $strobe value for the current time-delta
						*/
						//$display("M_AXIS_TREADY LOW, %0t", $time);
						//$display("DISP write_counter = %0h", write_counter);
						//$strobe("STROBE write_counter = %0h", write_counter);
						//write_counter <= 19;

						// Prevent synthesis mismatch with behavioural
						// Synthesis will somehow jump an address value (e.g. going from 3 to 5 straightaway)
						//if (is_deasserted) begin
							//write_counter = write_counter_prev+1;
							//RES_read_en <= 0;
							//M_AXIS_TVALID <= 0;
							//is_deasserted <= 0;
							//write_counter <= 4;
						//end
						RES_read_address <= write_counter+1;
						RES_read_en <= 0;
						//M_AXIS_TVALID <= 0;

						//$display("DISP is_deasserted set to 0, %0h %0t", is_deasserted, $time);
						//$strobe("STROB is_deasserted set to 0, %0h %0t", is_deasserted, $time);
						is_deasserted <= 0;
					end
				end
			endcase
		end
	end


	// Connection to sub-modules / components for assignment 1
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
										
	matrix_multiply 
	#(
		.width(width), 
		.A_depth_bits(A_depth_bits), 
		.B_depth_bits(B_depth_bits), 
		.RES_depth_bits(RES_depth_bits) 
	) matrix_multiply_0
	(									
		.clk(ACLK),
		.Start(Matrix_Start),
		.Done(Matrix_Done),
		
		.A_read_en(A_read_en),
		.A_read_address(A_read_address),
		.A_read_data_out(A_read_data_out),
		
		.B_read_en(B_read_en),
		.B_read_address(B_read_address),
		.B_read_data_out(B_read_data_out),
		
		.RES_write_en(RES_write_en),
		.RES_write_address(RES_write_address),
		.RES_write_data_in(RES_write_data_in)
	);

endmodule