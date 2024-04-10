`timescale 1ns / 1ps

module tb_advanced(

    );
    
    reg                          ACLK = 0;    // Synchronous clock
    reg                          ARESETN;     // System reset, active low
    // slave in interface
    wire                         S_AXIS_TREADY;  // Ready to accept data in
    reg [31 : 0]                 S_AXIS_TDATA;   // Data in
    reg                          S_AXIS_TLAST;   // Optional data in qualifier
    reg                          S_AXIS_TVALID;  // Data in is valid
    // master out interface
    wire                         M_AXIS_TVALID;  // Data out is valid
    wire [31 : 0]                M_AXIS_TDATA;   // Data out
    wire                         M_AXIS_TLAST;   // Optional data out qualifier
    reg                          M_AXIS_TREADY;  // Connected slave device is ready to accept data out
    
    myip_v1_0 dut ( 
                .ACLK(ACLK),
                .ARESETN(ARESETN),
                .S_AXIS_TREADY(S_AXIS_TREADY),
                .S_AXIS_TDATA(S_AXIS_TDATA),
                .S_AXIS_TLAST(S_AXIS_TLAST),
                .S_AXIS_TVALID(S_AXIS_TVALID),
                .M_AXIS_TVALID(M_AXIS_TVALID),
                .M_AXIS_TDATA(M_AXIS_TDATA),
                .M_AXIS_TLAST(M_AXIS_TLAST),
                .M_AXIS_TREADY(M_AXIS_TREADY)
	);

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

	localparam NUMBER_OF_TEST_VECTORS  = 1;  // number of such test vectors (cases)
	reg [width-1:0] test_input_memory [0:NUMBER_OF_TEST_VECTORS*NUMBER_OF_INPUT_WORDS-1];
	reg [width-1:0] test_result_expected_memory [0:NUMBER_OF_TEST_VECTORS*NUMBER_OF_OUTPUT_WORDS-1];
	reg [width-1:0] result_memory [0:NUMBER_OF_TEST_VECTORS*NUMBER_OF_OUTPUT_WORDS-1];    // same size as test_result_expected_memory
	
	integer word_cnt;
    integer test_case_cnt;
	reg success = 1'b1;
	reg M_AXIS_TLAST_prev = 1'b0;

	
	always @ (posedge ACLK) begin
		M_AXIS_TLAST_prev <= M_AXIS_TLAST;
    end

	always
		#50 ACLK = ~ACLK;

    /********************** COPROCESSOR AS SLAVE, TESTBENCH AS MASTER **********************/
    initial begin
        #25                       // Just as main driver pulls reset to low
        S_AXIS_TVALID <= 1'b0;    // MASTER->SLAVE: no valid data placed on the S_AXIS_TDATA yet
        S_AXIS_TLAST <= 1'b0; 	  // MASTER->SLAVE: not required unless we are dealing with an unknown number of inputs. 
                                    // Ignored by the coprocessor. We will be asserting it correctly anyway
        //S_AXIS_TREADY <= 1'b0;  // SLAVE->MASTER: Slave is not ready to accept data yet
                                    // For our implementation, Slave asserts TREADY when Slave gets TVALID==1 from Master
                                    // Note : This is not really how AXIS works (Slave can be ready without indication from Master)

        #125    // Pulled up on clock edge (FAILS)
        //#150  // Pulled up off-cycle
        S_AXIS_TVALID <= 1'b1;

        #200 S_AXIS_TVALID <= 1'b0;    // In 2 clock cycles, deassert S_AXIS_TVALID
        #100 S_AXIS_TVALID <= 1'b1;    // In 1 more clock cycle, assert back S_AXIS_TVALID


        // Second testcase, M_AXIS_TLAST pulsed at 111050ns for 1st testcase
        //#111000 S_AXIS_TVALID <= 1'b1;
        //#200 S_AXIS_TVALID <= 1'b0;
        //#100 S_AXIS_TVALID <= 1'b1;
    end

    /********************** COPROCESSOR AS MASTER, TESTBENCH AS SLAVE **********************/
    initial begin
        #25                       // Just as main driver pulls reset to low
        M_AXIS_TREADY <= 1'b0;	  // Not ready to receive data from the co-processor yet.   

        // At 53225ns, M_AXIS_TREADY is pulled high in main driver.
        // Doesn't mean coprocessor is ready to send (i.e assert M_AXIS_TVALID), since it still has to do Compute

        // Coprocessor enters Write_Output at 104_150ns
        // Coprocessor raises M_AXIS_TVALID at 2 clock cycles later (104_350), as need to fill pipeline first
        #104325

        #400  // Pulldown CLK's posedge
        M_AXIS_TREADY <= 1'b0;

        #100 M_AXIS_TREADY <= 1'b1;  // pulled back up on CLK's posedge


        // Second testcase
        // M_AXIS_TREADY pulled high by Main Driver (163825ns)
        //#400 M_AXIS_TREADY <= 1'b0;
        //#100 M_AXIS_TREADY <= 1'b1;
    end


    /********************** MAIN DRIVER **********************/
    initial begin
        $display("Loading Memory.");
        $readmemh("test_input.mem", test_input_memory);                     // add the .mem file to the project or specify the complete path
        $readmemh("test_result_expected.mem", test_result_expected_memory); // add the .mem file to the project or specify the complete path
        #25						// to make inputs and capture from testbench not aligned with clock edges

        ARESETN = 1'b0; 		// apply reset (active low)
        #100 ARESETN = 1'b1;    // hold reset for 100 ns before releasing

        for (test_case_cnt=0; test_case_cnt < NUMBER_OF_TEST_VECTORS; test_case_cnt=test_case_cnt+1) begin
            word_cnt = 0;
            /******************************** COPROCESSOR AS SLAVE, RECEIVE ********************************/
            while (word_cnt < NUMBER_OF_INPUT_WORDS) begin
                // - AXI implementation is that, Master can send data to slave even if slave is not ready (S_AXIS_TREADY not asserted)
                // - Master (Testbench) is NOT obliged to send data just because S_AXIS_TREADY is asserted by Slave
                // - Master just needs to raise S_AXIS_TVALID

                // - However for our implementation, Slave asserts ready (S_AXIS_TREADY) when Master (Testbench) asserts ready (S_AXIS_TVALID)
                // - Then, Master (Testbench) only ACTUALLY sends data when Slave is ready
                // - Thus, Master does not need to check if Slave actually received the data (it is guaranteed)

                // S_AXIS_TVALID was set by driver above (ie Testbench is MASTER, setting S_AXIS_TVALID)
                // S_AXIS_TREADY is asserted by the coprocessor in response to S_AXIS_TVALID

                // This is in accordance with AXI specs; even if ONLY TVALID is asserted it does not mean TDATA will just keep iterating and sending
                // It must wait for TVALID and TREADY to be asserted together, then TDATA will iterate and send
                if (S_AXIS_TVALID && S_AXIS_TREADY) begin
                    S_AXIS_TDATA <= test_input_memory[word_cnt+test_case_cnt*NUMBER_OF_INPUT_WORDS]; 
                    S_AXIS_TLAST <= (word_cnt == NUMBER_OF_INPUT_WORDS-1) ? 1'b1 : 1'b0;
                    word_cnt = word_cnt + 1;
                end

                // Wait for coprocessor
                #100;			
            end

            // Coprocessor(slave) has captured all data, but it doesn't mean that it has written it to RAM yet
            S_AXIS_TVALID <= 1'b0;	// Incoming data to coprocessor is no longer valid
            S_AXIS_TLAST <= 1'b0;    // Deassert the TLAST pulse

            /******************************** COPROCESSOR AS MASTER, SENDING ********************************/
            // Note: result_memory is not written at a clock edge, which is fine as it is just a testbench construct and not actual hardware
            word_cnt = 0;
            M_AXIS_TREADY <= 1'b1;	// Testbench (slave) is ready to receive data

            // Testbench 'ready' to receive data until falling edge of M_AXIS_TLAST
            // Note M_AXIS_TVALID is set by Coprocessor (master) in response to M_AXIS_TREADY from Testbench (slave)

            // Furthur check that M_AXIS_TREADY & M_AXIS_TVALID are simultaneously asserted together
            // This mimics how the actual AXI DMA works on the FPGA
            while(M_AXIS_TLAST | ~M_AXIS_TLAST_prev) begin
                if(M_AXIS_TVALID && M_AXIS_TREADY) begin
                    result_memory[word_cnt+test_case_cnt*NUMBER_OF_OUTPUT_WORDS] <= M_AXIS_TDATA;
                    word_cnt = word_cnt+1;
                end

                // Wait for coprocessor
                #100;
            end

            M_AXIS_TREADY <= 1'b0;	// Testbench not ready to receive data
        end							


        // Checking correctness of results
        for(word_cnt=0; word_cnt < NUMBER_OF_TEST_VECTORS*NUMBER_OF_OUTPUT_WORDS; word_cnt=word_cnt+1) begin
                success = success & (result_memory[word_cnt] == test_result_expected_memory[word_cnt]);

                if (success != 1) begin
                    $display("Expected %0h got %0h, %0d", test_result_expected_memory[word_cnt], result_memory[word_cnt], word_cnt);
                end
        end

        if(success)
            $display("Test Passed.");
        else
            $display("Test Failed.");
        
        $finish;       	
    end 

endmodule