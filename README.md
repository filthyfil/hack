This repo is to hold the important projects 
associated with the nand2tetris campaign.

-------------------------------------------------
## Items ( in progress )
-------------------------------------------------
### Hardware:
 - COMPUTER.v
This is the verilog (HDL) file that encodes 
the hardware specfication of the computer. 

Note the harvard architecture, and the 
writability of the ROM. This is temporary 
so I can program the computer without an OS.

### Software:
 - hackassembler.cpp
This program is an assembler written in c++. 
It translates assembly into machine code.

 - hackVM2.cpp
This is a VM translator similar to JVM that converts
VM code to Hack assembly. Supports full VM command set
including arithmetic, memory access, program flow, and
function calls.

 - hackcompiler ( not started )

 - hackOS ( not started )

### Web Applications:
 - streamlit_apps/ ( completed )
User-friendly web interfaces for the development tools:
  * Main Dashboard - Central hub for all tools
  * Assembler App - Convert assembly to machine code
  * VM Translator App - Convert VM code to assembly
  * Built-in examples, code analysis, and download functionality

Run with: `cd streamlit_apps && streamlit run main_app.py` 