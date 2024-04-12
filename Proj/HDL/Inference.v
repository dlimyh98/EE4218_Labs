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

		input [width-1:0] hidden_layer_bias_one,
		input [width-1:0] hidden_layer_bias_two,
		input [width-1:0] output_layer_bias,

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
		output C_read_en_1,
		output [C_depth_bits-1:0] C_read_address_1,
		
		output reg RES_write_en = 1'b1,
		output reg [RES_depth_bits-1:0] RES_write_address,
		output reg [width-1:0] RES_write_data_in
	);

	/******************************** A*B (Input->Hidden) ********************************/
	// Traversal along A(mxn) and B(nxp) matrices. Note matrices 1-indexed.
	// NOTE: B's first row is bias term, we disregard that for matrix multiplication.
	localparam A_m = 64;
	localparam A_n = 7;
	//localparam B_n = 8;
	localparam B_p = 2;
	localparam SKIP_HIDDEN_BIAS = 1;

	// Hidden Layer - Node 1
	wire hidden_node1_particular_done;
	wire hidden_node1_all_done;
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
			.mmult_bias_term(hidden_layer_bias_one),
			.mmult_particular_datapoint_done(hidden_node1_particular_done),
			.mmult_all_datapoints_done(hidden_node1_all_done),
			.mmult_results(hidden_node1_result),

			.X_read_data(A_read_data_out_1),
			.X_read_en(A_read_en_1),
			.X_read_address(A_read_address_1),

			.Y_read_data(B_read_data_out_1),
			.Y_read_en(B_read_en_1),
			.Y_read_address(B_read_address_1)
		);
		
	// Hidden Layer - Node 2
	wire hidden_node2_particular_done;
	wire hidden_node2_all_done;
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
			.mmult_bias_term(hidden_layer_bias_two),
			.mmult_particular_datapoint_done(hidden_node2_particular_done),
			.mmult_all_datapoints_done(hidden_node2_all_done),
			.mmult_results(hidden_node2_result),

			.X_read_data(A_read_data_out_2),
			.X_read_en(A_read_en_2),
			.X_read_address(A_read_address_2),

			.Y_read_data(B_read_data_out_2),
			.Y_read_en(B_read_en_2),
			.Y_read_address(B_read_address_2)
		);

	
	/******************************** (A*B)*C (Hidden->Output) ********************************/
	// A(mxn)*B(nxp) from the previous section gives R(mxp) matrix
	// However, note that we do not wait for the entire (A*B) process to complete
	// Instead, once the (1x2) matrix corresponding to both hidden layer nodes are available, we matrix multiply them DIRECTLY with output weights

	// Traversal along R(mxp) and C(px1) matrices. Note matrices 1-indexed
	// NOTE: C's first row is bias term, we disregard that for matrix multiplication.
	localparam R_m = 1;
	localparam R_p = 2;
	//localparam C_p = 3;

	// Note: Both nodes of the hidden layer complete their mmult simultaneously, so we just take the first node
	wire hidden_node_particular_done;	// Pulsed when BOTH nodes of the hidden layer are computed (for ONE datapoint)
	wire hidden_node_all_done;			// Pulsed when BOTH nodes of the hidden layer are computed (for ALL datapoints)

	// Output node
	wire output_node_particular_done;
	reg output_node_all_done = 1'b0;
	wire [width-1:0] output_node_result;

	// Create a makeshift 'RAM', mimicking RAM unit functionality
	wire [width-1:0] hidden_node_makeshift_RAM [0:1];
	wire hidden_node_makeshift_RAM_read_en;
	reg [width-1:0] hidden_node_read_data;
	reg hidden_node_makeshift_RAM_address = 1'b0;
	reg [$clog2(2**(RES_depth_bits)):0] num_output_nodes_calculated = 0;

	assign hidden_node_particular_done =  hidden_node1_particular_done;
	assign hidden_node_all_done = hidden_node1_all_done;
	assign hidden_node_makeshift_RAM[0] = hidden_node1_result;
	assign hidden_node_makeshift_RAM[1] = hidden_node2_result;

	// Computation of Output Nodes
	always @(posedge clk) begin
		// Read from our makeshift RAM, a (1x2) matrix storing hidden node values
		if (hidden_node_makeshift_RAM_read_en) begin
			hidden_node_read_data <= hidden_node_makeshift_RAM[hidden_node_makeshift_RAM_address];
			hidden_node_makeshift_RAM_address <= hidden_node_makeshift_RAM_address + 1;
		end

		// Keep count of how many datapoints have been outputted (ie. classified)
		if (output_node_particular_done) begin
			num_output_nodes_calculated <= num_output_nodes_calculated + 1;
		end

		// Signal that ALL datapoints at output node have been outputted
		if (num_output_nodes_calculated == A_m) begin
			output_node_all_done <= 1;
			RES_write_en <= 0;
		end
	end

	// Writing to RES RAM
	always @ (posedge clk) begin

		if (output_node_particular_done) begin
			// There's a value that needs to be written to RES_RAM.
			// Note: RES_RAM write enable is always HIGH

			RES_write_address <= num_output_nodes_calculated;
			RES_write_data_in <= output_node_result;
		end
	end

	mmult
		#(	.width(width),
			.m(R_m),
			.n(R_p),
			//.X_depth_bits(A_depth_bits),
			.Y_depth_bits(C_depth_bits),
			.Y_new_row_offset(1),
			.Y_starting_row_offset(SKIP_HIDDEN_BIAS),
			.Y_column_offset(0)
		) mmult_hidden_output_node
		(
			.clk(clk),
			.mmult_start(hidden_node_particular_done),
			.mmult_bias_term(output_layer_bias),
			.mmult_particular_datapoint_done(output_node_particular_done),
			//.mmult_all_datapoints_done(),
			.mmult_results(output_node_result),

			.X_read_data(hidden_node_read_data),
			.X_read_en(hidden_node_makeshift_RAM_read_en),
			//.X_read_address(),

			.Y_read_data(C_read_data_out_1),
			.Y_read_en(C_read_en_1),
			.Y_read_address(C_read_address_1)
		);


	/******************************** Inference State ********************************/
	always @ (posedge clk) begin
		Done <= (output_node_all_done);
	end

endmodule