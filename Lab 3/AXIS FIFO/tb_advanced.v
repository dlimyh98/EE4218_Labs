
`timescale 1ns / 1ps

module tb_advanced(

    );
    
    reg                          ACLK = 0;    // Synchronous clock
    reg                          ARESETN;     // System reset, active low
    // slave in interface
    wire                         S_AXIS_TREADY;  // Ready to accept data in
    reg      [31 : 0]            S_AXIS_TDATA;   // Data in
    reg                          S_AXIS_TLAST;   // Optional data in qualifier
    reg                          S_AXIS_TVALID;  // Data in is valid
    // master out interface
    wire                         M_AXIS_TVALID;  // Data out is valid
    wire     [31 : 0]            M_AXIS_TDATA;   // Data out
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

	localparam A_depth_bits = 9;  	// A is a 64x8 matrix
	localparam B_depth_bits = 3; 	// B is a 8x1 matrix
	localparam RES_depth_bits = 6;	// RES is a 64x1 matrix
	localparam width = 8;			// all 8-bit data
	localparam NUMBER_OF_A_WORDS = 2**A_depth_bits;
	localparam NUMBER_OF_B_WORDS = 2**B_depth_bits;
	localparam NUMBER_OF_INPUT_WORDS  = NUMBER_OF_A_WORDS + NUMBER_OF_B_WORDS;	// Total number of input data.
	localparam NUMBER_OF_OUTPUT_WORDS = 2**RES_depth_bits;	                    // Total number of output data

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
        S_AXIS_TVALID = 1'b0;     // MASTER->SLAVE: no valid data placed on the S_AXIS_TDATA yet
        S_AXIS_TLAST = 1'b0; 	  // MASTER->SLAVE: not required unless we are dealing with an unknown number of inputs. 
                                    // Ignored by the coprocessor. We will be asserting it correctly anyway
        // S_AXIS_TREADY = 1'b0;  // SLAVE->MASTER: Slave is not ready to accept data yet
                                    // For our implementation, Slave asserts TREADY when it Slave gets TVALID==1 from Master
                                    // Note : This is not really how AXIS works (Slave can be ready without indication from Master)

        #150
        S_AXIS_TVALID = 1'b1;

        #200 S_AXIS_TVALID = 1'b0;    // In 2 clock cycles, deassert S_AXIS_TVALID
        #300 S_AXIS_TVALID = 1'b1;    // In 3 clock cycles, assert back S_AXIS_TVALID

        #2000 S_AXIS_TVALID = 1'b0;
        #600 S_AXIS_TVALID = 1'b1;
    end

    /********************** COPROCESSOR AS MASTER, TESTBENCH AS SLAVE **********************/
    initial begin
        #25                       // Just as main driver pulls reset to low
        M_AXIS_TREADY = 1'b0;	  // Not ready to receive data from the co-processor yet.   

        // At 53225ns, M_AXIS_TREADY is pulled high in main driver (hardcoded by looking at sim waveform)
        #53400 M_AXIS_TREADY = 1'b0;
        #300 M_AXIS_TREADY = 1'b1;
    end


    /********************** MAIN DRIVER **********************/
    initial begin
        $display("Loading Memory.");
        $readmemh("test_input.mem", test_input_memory);                     // add the .mem file to the project or specify the complete path
        $readmemh("test_result_expected.mem", test_result_expected_memory); // add the .mem file to the project or specify the complete path
        #25						// to make inputs and capture from testbench not aligned with clock edges

        ARESETN = 1'b0; 		// apply reset (active low)
        #100 ARESETN = 1'b1;    // hold reset for 100 ns before releasing
        word_cnt = 0;

        for (test_case_cnt=0; test_case_cnt < NUMBER_OF_TEST_VECTORS; test_case_cnt=test_case_cnt+1) begin
            /******************************** COPROCESSOR AS SLAVE, RECEIVE ********************************/
            while (word_cnt < NUMBER_OF_INPUT_WORDS) begin
                // - Master can send data to slave even if slave is not ready (S_AXIS_TREADY not asserted)
                // - However, Master still needs to check if slave has received the previous data, before sending something new
                // - Therefore, instead of repeatedly sending the same data and checking if slave has received it before sending
                //   something new, we just check S_AXIS_TREADY

                // S_AXIS_TVALID was set by Coprocessor-as-slave driver
                // S_AXIS_TREADY is asserted by the coprocessor in response to S_AXIS_TVALID
                if (S_AXIS_TREADY) begin
                    S_AXIS_TDATA = test_input_memory[word_cnt+test_case_cnt*NUMBER_OF_INPUT_WORDS]; 
                    S_AXIS_TLAST = (word_cnt == NUMBER_OF_INPUT_WORDS-1) ? 1'b1 : 1'b0;
                    word_cnt = word_cnt + 1;
                end

                // Wait for coprocessor
                #100;			
            end

            // Coprocessor has captured all data, but it doesn't mean that it has written it to RAM yet
            S_AXIS_TVALID = 1'b0;	// Incoming data to coprocessor is no longer valid
            S_AXIS_TLAST = 1'b0;    // Deassert the TLAST pulse

            /******************************** COPROCESSOR AS MASTER, SENDING ********************************/
            // Note: result_memory is not written at a clock edge, which is fine as it is just a testbench construct and not actual hardware
            word_cnt = 0;
            M_AXIS_TREADY = 1'b1;	// Testbench is ready to receive data

            while(M_AXIS_TLAST | ~M_AXIS_TLAST_prev) begin    // Testbench receives data until the falling edge of M_AXIS_TLAST
                if(M_AXIS_TVALID) begin
                    result_memory[word_cnt+test_case_cnt*NUMBER_OF_OUTPUT_WORDS] = M_AXIS_TDATA;
                    word_cnt = word_cnt+1;
                end
                #100;
            end						

            M_AXIS_TREADY = 1'b0;	// Testbench not ready to receive data
        end							


        // Checking correctness of results
        for(word_cnt=0; word_cnt < NUMBER_OF_TEST_VECTORS*NUMBER_OF_OUTPUT_WORDS; word_cnt=word_cnt+1)
                success = success & (result_memory[word_cnt] == test_result_expected_memory[word_cnt]);
        if(success)
            $display("Test Passed.");
        else
            $display("Test Failed.");
        
        $finish;       	
    end 

endmodule