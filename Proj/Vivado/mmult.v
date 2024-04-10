`timescale 1ns / 1ps

// Accepts X(mxn), Y(nxp) matrices
// Multiplies them Block-Column style, i.e X(mxn)*Y(nx1), X(mxn)*Y(nx2), ... , X(mxn)*Y(nxp)
module mmult 
    #(  parameter width = 8, 
        parameter m = 1,
        parameter n = 1,
        parameter X_depth_bits = 1,
        parameter Y_depth_bits = 1,
        parameter Y_new_row_offset = 1,           // Number of elements to jump in contiguous memory, to get the next 'row'
        parameter Y_starting_row_offset = 1,      // Skip the first row (they are bias terms)
        parameter Y_column_offset = 0             // Offset to ith column if needed
    )
    (
        input clk,
        input mmult_start,
        output reg mmult_done = 1'b0,
        output reg [width-1:0] mmult_results,

        input [width-1:0] X_read_data,
        output reg X_read_en,
        output reg [X_depth_bits-1:0] X_read_address,

        input [width-1:0] Y_read_data,
        output reg Y_read_en,
        output reg [Y_depth_bits-1:0] Y_read_address
    );


    // Traversal along X(mn) and Y(n1) matrices. Note matrices are 1-indexed, but registers below are all 0-indexed.
    reg [$clog2(m):0] X_row_traversal = 0;                          // Traversing along the m rows of X.
    reg [$clog2(n)-1:0] X_column_traversal = 0;                     // Traversing along n rows of X.
    reg [$clog2(n)-1:0] Y_row_traversal = Y_starting_row_offset;

    // For X(mn)*Y(n1), each element of X&Y is 8-bit unsigned number (maximal value 255)
    // Therefore when computing X*Y, we need minimally 16-bit unsigned number (to store 255*255)
    // Then, we sum (X*Y) n times. Xssuming n=8, 19-bits is sufficient to hold it. Lets just put 32bits
    localparam MAXIMAL_SUM_BITS = 32;
    reg [MAXIMAL_SUM_BITS-1:0] sum = 32'b0;
    reg [$clog2(n):0] count_sums = 0;    // We expect do to n summations when computing (Xmn)*(Yn1) for some m
    reg [$clog2(m):0] which_row = 0;     // (Xmn)*(Yn1) yields R(m1) matrix, thus this tracks which mth row of R to place result

    reg is_pipeline_filling = 1'b1;
    reg mmult_active = 1'b0;
    reg [MAXIMAL_SUM_BITS-1:0] before_trim = 32'b0;    // Debugging purposes


    // Note: X_RAM and Y_RAM are to be read synchronously.
    always @(posedge clk) begin

        // Start is pulsed for 1 cycle -> Triggers mmult to run
		if (mmult_start && !mmult_active) mmult_active <= 1;
        // mmult is completed -> De-pulse mmult_done signal (It was brought high earlier)
		if (mmult_done && !mmult_active) mmult_done <= 0;

        if (mmult_active == 1) begin
            // Enable reading of RAM
            X_read_en <= 1;
            Y_read_en <= 1;

            // Due to pipeline design, fetched data will arrive on every cycle (from Cycle 3 onwards, assuming start counting from Cycle 1)
            // Hence, since the pipeline requires 2 cycles to fill, we stall the pipeline ONCE (from Cycle 2-> Cycle 3)
            if (!is_pipeline_filling) begin
                sum <= sum + (X_read_data * Y_read_data);
                count_sums <= count_sums + 1;

                // Check if we need to write to RES RAM
                if (count_sums == n-1) begin
                    //RES_write_en <= 1;
                    //RES_write_address <= which_row;
                    mmult_results <=  (sum + (X_read_data * Y_read_data)) >> 8;
                    //before_trim <=  sum + X_read_data * Y_read_data;
                    //RES_write_data_in <= (sum + (X_read_data * Y_read_data)) >> 8;    // Divide the FINAL SUM by 256

                    // Reset for next entry into RES RAM
                    count_sums <= 0;
                    which_row <= which_row + 1;
                    sum <= 0;
                end
                //else begin
                    //RES_write_en <= 0;
                //end

                // Done with Matrix Multiplication (not just traversing it!)
                if (which_row == m) begin
                    // (Xij)*Y(i1), we have traversed all (ij). Thus Matrix-Multiplication is complete
                    X_read_en <= 0;
                    Y_read_en <= 0;

                    X_row_traversal <= 0;
                    X_column_traversal <= 0;
                    Y_row_traversal <= Y_starting_row_offset;
                    is_pipeline_filling <= 1;
                    sum <= 0;
                    count_sums <= 0;
                    which_row <= 0;

                    mmult_done <= 1;
                    mmult_active <= 0;
                end
            end

            // TRAVERSAL for Matrix Multiplication (Row-Column style)
            if (X_row_traversal != m) begin
                // Send request to RAM
                X_read_address <= (n * X_row_traversal) + (X_column_traversal);
                Y_read_address <= (Y_row_traversal * Y_new_row_offset) + Y_column_offset;

                // read_address takes 1 clock cycle to propagate to RAM
                // RAM furthur requires 1 clock cycle to output read data
                // i.e We need 2 cycles to fill our pipeline
                if (X_row_traversal == 0 && Y_row_traversal == Y_starting_row_offset) begin
                    is_pipeline_filling <= 1;
                end
                else begin
                    is_pipeline_filling <= 0;
                end

                // Prepare to send ANOTHER request to RAM in next cycle
                if (X_column_traversal != n-1) begin
                    // X(ij)*Y(j1), continue summing along ith row
                    X_column_traversal <= X_column_traversal + 1;
                    Y_row_traversal <= Y_row_traversal + 1;
                end
                else begin
                    // X(ij)*Y(j1), ith row is completed
                    X_column_traversal <= 0;
                    Y_row_traversal <= Y_starting_row_offset;
                    X_row_traversal <= X_row_traversal + 1;
                end
            end
        end 
    end

endmodule