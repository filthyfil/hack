module alu #(
	parameter WIDTH = 16
)
(
	// i/o
	// input
	input wire[WIDTH-1:0] x,
	input wire[WIDTH-1:0] y,
	
	// control
	input wire       zx, // zero x
	input wire       nx, // negate x
	input wire       zy, // zero y
	input wire       ny, // negate y
	input wire       f,  // if f==1 out=add(x,y), else out=and(x,y)
	input wire       no, // negate output
	
	// flags
	output wire zr, // is out == 0?
	output wire ng, // is out negative?
	
	// output
	output wire[WIDTH-1:0] out
);
	// combinational unit of the ALU described in figure 2.5 of EoCS
	// the API is described in 2.5c
	// the unit's sideeffects from modifying the inputs are described in 2.5b
	//  - and, or, subtraction, addition are some of the supported ops
	
	wire [WIDTH-1:0] x0, x1, y0, y1, out0;
	
	// input modifiers
	assign x0 = zx ? 0 : x;
	assign x1 = nx ? ~x0 : x0;
	assign y0 = zy ? 0 : y;
	assign y1 = ny ? ~y0 : y0;
	
	// ops
	assign out0 = f ? x1+y1 : x1&y1;
	
	// out
	assign out = no ? ~out0 : out0; 
	
	// flags
	assign zr = (out == 0);
	assign ng = out[WIDTH-1];
endmodule
