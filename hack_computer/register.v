module register #(
	parameter WIDTH = 16
)
(
	// i/o
	input wire clk,
	input wire rst_n,

	input wire load,
	input wire [WIDTH-1:0] d,
	output reg [WIDTH-1:0] q
);
	always @(posedge clk or negedge rst_n) begin
		if (rst_n == 0) 
			q <= 0;
		else if (load == 1) 
			q <= d;	
	end
endmodule
