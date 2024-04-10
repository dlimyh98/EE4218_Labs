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
		input Start,					    
		output reg Done,

		input [width-1:0] A_read_data_out_1,
		input [width-1:0] A_read_data_out_2,
		output A_read_en_1,
		output A_read_en_2,
		output [A_depth_bits-1:0] A_read_address_1,
		output [A_depth_bits-1:0] A_read_address_2,
		
		input [width-1:0] B_read_data_out_1,
		input [width-1:0] B_read_data_out_2,
		output B_read_en_1,
		output B_read_en_2,
		output [B_depth_bits-1:0] B_read_address_1,
		output [B_depth_bits-1:0] B_read_address_2,
		
		input [width-1:0] C_read_data_out_1,
		input [width-1:0] C_read_data_out_2,
		output C_read_en_1,
		output C_read_en_2,
		output [C_depth_bits-1:0] C_read_address_1,
		output [C_depth_bits-1:0] C_read_address_2,
		
		
		output reg RES_write_en,
		output reg [RES_depth_bits-1:0] RES_write_address,
		output reg [width-1:0] RES_write_data_in
	);

	/******************************** A*B (Input->Hidden) ********************************/
	// Traversal along A(mxn) and B(nxp) matrices. Note matrices 1-indexed.
	// NOTE: B's first row is bias term, we disregard that for matrix multiplication.
	localparam A_m = 64;
	localparam A_n = 7;
	localparam B_n = 8;
	localparam B_p = 2;
	localparam SKIP_HIDDEN_BIAS = 1;

	// Hidden Layer - Node 1
	wire hidden_node1_computed;
	wire [width-1:0] hidden_node1_result;

	mmult
		#(	.width(width),
			.m(A_m),
			.n(A_n),
			.X_depth_bits(A_depth_bits),
			.Y_depth_bits(B_depth_bits),
			.Y_new_row_offset(B_p),
			.Y_starting_row_offset(SKIP_HIDDEN_BIAS),
			.Y_column_offset(0)
		) mmult_input_hidden_node1
		(
			.clk(clk),
			.mmult_start(Start),
			.mmult_done(hidden_node1_computed),
			.mmult_results(hidden_node1_result),

			.X_read_data(A_read_data_out_1),
			.X_read_en(A_read_en_1),
			.X_read_address(A_read_address_1),

			.Y_read_data(B_read_data_out_1),
			.Y_read_en(B_read_en_1),
			.Y_read_address(B_read_address_1)
		);
		
	// Hidden Layer - Node 2
	wire hidden_node2_computed;
	wire [width-1:0] hidden_node2_result;

	mmult
		#(	.width(width),
			.m(A_m),
			.n(A_n),
			.X_depth_bits(A_depth_bits),
			.Y_depth_bits(B_depth_bits),
			.Y_new_row_offset(B_p),
			.Y_starting_row_offset(SKIP_HIDDEN_BIAS),
			.Y_column_offset(1)
		) mmult_input_hidden_node2
		(
			.clk(clk),
			.mmult_start(Start),
			.mmult_done(hidden_node2_computed),
			.mmult_results(hidden_node2_result),

			.X_read_data(A_read_data_out_2),
			.X_read_en(A_read_en_2),
			.X_read_address(A_read_address_2),

			.Y_read_data(B_read_data_out_2),
			.Y_read_en(B_read_en_2),
			.Y_read_address(B_read_address_2)
		);

	
	/******************************** (A*B)*C (Hidden->Output) ********************************/
	// A*B from the previous section gives R(mxp) matrix.
	// Traversal along R(mxp) and C(px1) matrices. Note matrices 1-indexed
	// NOTE: C's first row is bias term, we disregard that for matrix multiplication.
	localparam R_m = 64;
	localparam R_p = 2;
	localparam C_p = 3;


	/******************************** Inference Control  ********************************/
	// Pulsed when BOTH nodes of the hidden layer are computed (for ONE datapoint)
	// Note: Both nodes of the hidden layer complete their mmult simultaneously
	wire hidden_node_computed;
	assign hidden_node_computed = hidden_node1_computed;

	//always @(posedge clk) begin
		/*
		// Input signal Start is pulsed for 1 cycle -> Triggers MAC unit to run
		if (Start && !is_inferencing) is_inferencing <= 1;
		// MAC unit stopped -> Output signal Done always pulled lowed
		if (Done && !is_inferencing) Done <= 0;

		// Inference
		if (is_inferencing == 1) begin
			
		end 
		else begin
			// We enter here if we are done with matrix multiplication
			// De-pulse the 'Done' signal that we raised previously
			Done <= 0;
		end
		*/
	//end

endmodule