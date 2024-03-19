`timescale 1ns / 1ps

/* 
----------------------------------------------------------------------------------
--	(c) Rajesh C Panicker, NUS
--  Description : Template for the Matrix Multiply unit for the AXI Stream Coprocessor
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

// those outputs which are assigned in an always block of matrix_multiply shoud be changes to reg (such as output reg Done).

module matrix_multiply
	#(	parameter width = 8, 			// width is the number of bits per location
		parameter A_depth_bits = 3, 	// depth is the number of locations (2^number of address bits)
		parameter B_depth_bits = 2, 
		parameter RES_depth_bits = 1
	) 
	(
		input clk,										
		input Start,									    // myip_v1_0 -> matrix_multiply_0.
		output reg Done,									// matrix_multiply_0 -> myip_v1_0. Possibly reg.
		
		output reg A_read_en,  								// matrix_multiply_0 -> A_RAM. Possibly reg.
		output reg [A_depth_bits-1:0] A_read_address, 		// matrix_multiply_0 -> A_RAM. Possibly reg.
		input [width-1:0] A_read_data_out,				    // A_RAM -> matrix_multiply_0.
		
		output reg B_read_en, 								// matrix_multiply_0 -> B_RAM. Possibly reg.
		output reg [B_depth_bits-1:0] B_read_address, 		// matrix_multiply_0 -> B_RAM. Possibly reg.
		input [width-1:0] B_read_data_out,				    // B_RAM -> matrix_multiply_0.
		
		output reg RES_write_en, 							// matrix_multiply_0 -> RES_RAM. Possibly reg.
		output reg [RES_depth_bits-1:0] RES_write_address, 	// matrix_multiply_0 -> RES_RAM. Possibly reg.
		output reg [width-1:0] RES_write_data_in 			// matrix_multiply_0 -> RES_RAM. Possibly reg.
	);

	localparam NUMBER_OF_A_WORDS = 2**A_depth_bits;
	localparam NUMBER_OF_B_WORDS = 2**B_depth_bits;
	localparam NUMBER_OF_INPUT_WORDS  = NUMBER_OF_A_WORDS + NUMBER_OF_B_WORDS;	// Total number of input data.
	localparam NUMBER_OF_OUTPUT_WORDS = 2**RES_depth_bits;	                    // Total number of output data

	// Traversal along A(mxn) and B(nx1) matrices
	localparam m = 64;                                    // Note matrices 1-indexed
	localparam n = 8;                                     // Note matrices 1-indexed
	reg [$clog2(m):0] A_row_traversal = 0;                // Traversing along the m rows of A. Note 0-indexed.
	reg [$clog2(n)-1:0] A_column_B_row_traversal = 0;     // A and B can share the same counter (since both are n). Note 0-indexed.

	// For A(mxn)*B(nx1), each element of A&B is 8-bit unsigned number (maximal value 255)
	// Therefore when computing A*B, we need minimally 16-bit unsigned number (to store 255*255)
	// Then, we sum (A*B) n times. Assuming n=8, 19-bits is sufficient to hold it. Lets just put 32bits
	localparam MAXIMAL_SUM_BITS = 32;
	reg [MAXIMAL_SUM_BITS-1:0] sum = 32'b0;
	reg is_pipeline_filling = 1;
	reg [$clog2(n):0] count_sums = 0;    // We expect do to n summations when computing (Amn)*(Bn1) for some m
	reg [$clog2(m):0] which_row = 0;     // (Amn)*(Bn1) yields C(m1) matrix, thus this tracks which mth row of C to place result

	reg is_multiplying = 0;
	reg [MAXIMAL_SUM_BITS-1:0] before_trim = 32'b0;    // Debugging purposes

	// implement the logic to read A_RAM, read B_RAM, do the multiplication and write the results to RES_RAM
	// Note: A_RAM and B_RAM are to be read synchronously. Read the wiki for more details.
	always @(posedge clk) begin

		// Start is pulsed for 1 cycle -> Triggers matrixMultiplication to run
		if (Start && !is_multiplying) is_multiplying <= 1;
		// Done is pulsed for 1 cycle -> Triggers matrixMultiplication to stop
		if (Done && !is_multiplying) Done <= 0;

		// Matrix-Multiplication
		if (is_multiplying == 1) begin
			// Enable reading of RAM
			A_read_en <= 1;
			B_read_en <= 1;

			// Due to pipeline design, fetched data will arrive on every cycle (from Cycle 3 onwards)
			// Hence, since the pipeline requires 2 cycles to fill, we stall the pipeline ONCE (from Cycle 2-> Cycle 3)
			if (!is_pipeline_filling) begin
				sum <= sum + (A_read_data_out * B_read_data_out);
				count_sums <= count_sums + 1;

				// Check if we need to write to RES RAM
				if (count_sums == n-1) begin
					RES_write_en <= 1;
					RES_write_address <= which_row;
					before_trim <=  sum + A_read_data_out * B_read_data_out;
					RES_write_data_in <= (sum + (A_read_data_out * B_read_data_out)) >> 8;    // Divide the FINAL SUM by 256

					// Reset for next entry into RES RAM
					count_sums <= 0;
					which_row <= which_row + 1;
					sum <= 0;
				end
				else begin
					RES_write_en <= 0;
				end

				// Check if done with Matrix Multiplication (not just traversing it!)
				if (which_row == m) begin
					// (Aij)*B(i1), we have traversed all (ij). Thus Matrix-Multiplication is complete
					// Reset everything, signal DONE to State Machine outside
					A_read_en <= 0;
					B_read_en <= 0;

					A_row_traversal <= 0;
					A_column_B_row_traversal <= 0;
					is_pipeline_filling <= 1;
					sum <= 0;
					count_sums <= 0;
					which_row <= 0;

					Done <= 1;
					is_multiplying <= 0;
				end
			end

			// TRAVERSAL for Matrix Multiplication (Row-Column style)
			begin
				if (A_row_traversal != m) begin
					// Send request to RAM
					A_read_address <= (n * A_row_traversal) + (A_column_B_row_traversal);
					B_read_address <= A_column_B_row_traversal;

					// read_address takes 1 clock cycle to propagate to RAM
					// RAM furthur requires 1 clock cycle to output read data
					// i.e We need 2 cycles to fill our pipeline
					if (A_row_traversal == 0 && A_column_B_row_traversal == 0) begin
						is_pipeline_filling <= 1;
					end
					else begin
						is_pipeline_filling <= 0;
					end

					// Prepare to send ANOTHER request to RAM in next cycle
					if (A_column_B_row_traversal != n-1) begin
						// A(ij)*B(j1), continue summing along ith row
						A_column_B_row_traversal <= A_column_B_row_traversal + 1;
					end
					else begin
						// A(ij)*B(j1), ith row is completed
						A_column_B_row_traversal <= 0;
						A_row_traversal <= A_row_traversal + 1;
					end
				end
			end
		end 
		else begin
			// We enter here if we are done with matrix multiplication
			// De-pulse the 'Done' signal that we raised previously
			Done <= 0;
		end
	end

endmodule