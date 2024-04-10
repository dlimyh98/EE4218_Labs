`timescale 1ns / 1ps

module Inference
	#(	parameter width = 8, 			// width is the number of bits per location
		parameter A_depth_bits = 3, 	// depth is the number of locations (2^number of address bits)
		parameter B_depth_bits = 2, 
		parameter C_depth_bits = 2,
		parameter RES_depth_bits = 1
	) 
	(
		input clk,										
		input Start,									    // myip_v1_0 -> matrix_multiply_0.
		output reg Done,									// matrix_multiply_0 -> myip_v1_0.
		
		output A_read_en,  								// matrix_multiply_0 -> A_RAM.
		output [A_depth_bits-1:0] A_read_address, 		// matrix_multiply_0 -> A_RAM. 
		input [width-1:0] A_read_data_out,				    // A_RAM -> matrix_multiply_0.
		
		output B_read_en, 								// matrix_multiply_0 -> B_RAM. 
		output [B_depth_bits-1:0] B_read_address, 		// matrix_multiply_0 -> B_RAM. 
		input [width-1:0] B_read_data_out,				    // B_RAM -> matrix_multiply_0.

		output reg C_read_en, 								// matrix_multiply_0 -> C_RAM. 
		output reg [C_depth_bits-1:0] C_read_address, 		// matrix_multiply_0 -> C_RAM. 
		input [width-1:0] C_read_data_out,				    // C_RAM -> matrix_multiply_0.
		
		output reg RES_write_en, 							// matrix_multiply_0 -> RES_RAM. 
		output reg [RES_depth_bits-1:0] RES_write_address, 	// matrix_multiply_0 -> RES_RAM. 
		output reg [width-1:0] RES_write_data_in 			// matrix_multiply_0 -> RES_RAM. 
	);

	/******************************** Inference Control  ********************************/

	/*
	reg is_inferencing = 1'b0;

	// Note: A_RAM and B_RAM are to be read synchronously.
	always @(posedge clk) begin
		// Input signal Start is pulsed for 1 cycle -> Triggers MAC unit to run
		if (Start && !is_inferencing) is_inferencing <= 1;
		// MAC unit stopped -> Output signal Done always pulled lowed
		if (Done && !is_inferencing) Done <= 0;

		//// Inference
		//if (is_inferencing == 1) begin
			
		//end 
		//else begin
			//// We enter here if we are done with matrix multiplication
			//// De-pulse the 'Done' signal that we raised previously
			//Done <= 0;
		//end
	end
	*/

	/******************************** A*B (Input->Hidden) ********************************/
	// Traversal along A(mxn) and B(nxp) matrices. Note matrices 1-indexed.
	// NOTE: B's first row is bias term, we disregard that for matrix multiplication.
	localparam A_m = 64;
	localparam A_n = 7;
	localparam B_n = 8;
	localparam B_p = 2;

	wire hidden_node1_ready;
	wire [width-1:0] hidden_node1_result;

	mmult
		#(	.width(width),
			.m(A_m),
			.n(A_n),
			.X_depth_bits(A_depth_bits),
			.Y_depth_bits(B_depth_bits),
			.Y_new_row_offset(B_p),
			.Y_starting_row_offset(1),
			.Y_column_offset(0)
		) mmult_input_hidden_node1
		(
			.clk(clk),
			.mmult_start(Start),
			.mmult_done(hidden_node1_ready),
			.mmult_results(hidden_node1_result),

			.X_read_data(A_read_data_out),
			.X_read_en(A_read_en),
			.X_read_address(A_read_address),

			.Y_read_data(B_read_data_out),
			.Y_read_en(B_read_en),
			.Y_read_address(B_read_address)
		);
	
	/******************************** (A*B)*C (Hidden->Output) ********************************/
	// A*B from the previous section gives R(mxp) matrix.
	// Traversal along R(mxp) and C(px1) matrices. Note matrices 1-indexed
	// NOTE: C's first row is bias term, we disregard that for matrix multiplication.
	localparam R_m = 64;
	localparam R_p = 2;
	localparam C_p = 3;

endmodule