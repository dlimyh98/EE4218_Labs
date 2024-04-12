`timescale 1ns / 1ps

// RAM supports two simultaneous reads OR one write; per cycle
// RAM is NOT dual ported (doesn't support reading and writing at the same time)
module memory_RAM
	#(
		// width is the number of bits per location; depth_bits is the number of address bits. 2^depth_bits is the number of locations
		parameter width = 8, 					// width is the number of bits per location
		parameter depth_bits = 2				// depth is the number of locations (2^number of address bits)
	) 
	(
		input clk,

		input write_en,
		input [depth_bits-1:0] write_address,
		input [width-1:0] write_data_in,

		input read_en_1,
		input [depth_bits-1:0] read_address_1,
		output reg [width-1:0] read_data_out_1,
		input read_en_2,
		input [depth_bits-1:0] read_address_2,
		output reg [width-1:0] read_data_out_2
	);
    
    reg [width-1:0] RAM [0:2**depth_bits-1];
    wire [depth_bits-1:0] address_1;
    wire [depth_bits-1:0] address_2;
    wire enable;
    
    // Convert external signals to a form followed in the template given in Vivado synthesis manual. 
  	// Following is from a template given in Vivado synthesis manual.
	// According to Xilinx HDL Coding Techniques Manual (WRITE MODES)...
	// READ-FIRST : Output previously stored data while new data is being written
	// WRITE-FIRST : Output newly written data onto output bus
	// NO-CHANGE : Mantains output previously generated by read operation
	// Consult https://docs.xilinx.com/r/en-US/am007-versal-memory/NO_CHANGE-Mode-DEFAULT for waveforms

    // Not really necessary, but to follow the spirit of using templates
    assign enable = (read_en_1 | read_en_2) | write_en;

	always @(posedge clk)
	begin
		 if (enable) begin
			if (write_en)
				RAM[write_address] <= write_data_in;
			else
				read_data_out_1 <= RAM[read_address_1];
				read_data_out_2 <= RAM[read_address_2];
		 end
	end

endmodule