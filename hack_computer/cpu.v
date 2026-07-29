module cpu #(
	parameter WIDTH = 16
)(
	// i/o
	// input
	input wire clk, // physical oscillator
	input wire [WIDTH-1:0] instruction,
	input wire [WIDTH-1:0] inM,
	input wire reset,
	
	// output
	output wire [WIDTH-1:0] outM,
	output wire             writeM,
	output wire [WIDTH-2:0] addressM,
	output wire [WIDTH-1:0] pc
);	
	// instruction splitter:
	// note this assumes WIDTH==16:
	wire instruction_type = instruction[15]; // is c instruction?
	wire a = instruction[12];
	wire [5:0] c = instruction[11:6];
	wire [2:0] d = instruction[5:3];
	wire [2:0] j = instruction[2:0];
	
	wire rst_n = ~reset;
	// instantiate modules:
	// alu, 2 registers, program counter
	// ALU
	alu alu_unit #(.WIDTH(WIDTH))(
		// input
		.x(d_reg_out),
		.y(alu_y),

		.zx(c[5]),
		.nx(c[4]),
		.zy(c[3]),
		.ny(c[2]),
		.f (c[1]),
		.no(c[0]),
		
		// output
		.zr(alu_zero),
		.ng(alu_negative),
		.out(alu_out)
	); 
	// A REGISTER
	register a_register_unit #(.WIDTH(WIDTH))(
		// input
		.clk(clk),
		.rst_n(rst_n),
		.load(a_reg_load), // load if A instruction or if dest of a C instruction
		.d(instruction0),
		// output
		.q(a_reg_out)
	); 
	// D REGISTER
	register d_register_unit #(.WIDTH(WIDTH))(
		// input
		.clk(clk),
		.rst_n(rst_n),
		.load(d_reg_load),
		.d(alu_out),
		// output
		.q(d_reg_out)
	);
	// PROGRAM COUNTER
	// for SPI ram, have enable on PC while waiting for memory
	program_counter program_counter_unit #(.WIDTH(WIDTH))(
		// input
		.clk(clk), 
		.rst_n(rst_n),
		.in(a_reg_out),
		.load(p_c_load), // need to check if jump condition is met here
		.inc(p_c_inc), // negation of load; if jump fails, get next instruction
		// output
		.out(pc)
	);

	// internal wiring
	wire [WIDTH-1:0] alu_out;
	wire [WIDTH-1:0] instruction0 = instruction_type ? alu_out : instruction;
	wire [WIDTH-1:0] a_reg_out;
	wire a_reg_load = ~instruction_type | d[2];
	wire [WIDTH-1:0] d_reg_out;
	wire d_reg_load = d[1];
	wire [WIDTH-1:0] alu_y = a ? inM : a_reg_out;
	wire alu_zero;
	wire alu_negative;
	
	wire jump_condition = (j[0] & ~alu_negative & ~alu_zero) | 
                              (j[1] & alu_zero) | 
                              (j[2] & alu_negative);
	wire p_c_load = jump_condition & instruction_type;
	wire p_c_inc = ~p_c_load;

	// i/o assignments from internal
	assign addressM = a_reg_out[WIDTH-2:0];
	assign writeM = d[0] & instruction_type;
	assign outM = alu_out;
endmodule
