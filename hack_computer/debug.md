# Debugging LibreLane Errors

Read the last 20 or 30 lines of log output. A lot won’t make sense but it’s easy to spot basic configuration issues. 

    Broken configuration, typos or bad paths mean the tools barely get started.
    Synthesis tools don’t find design. Not all the source has been specified, or it has dependencies not yet fulfilled (for example uninitialised git submodules).
    Syntax error in the HDL of the design.
    Tools can’t find the clock - for example your clock is not named clk. Update the configuration file.
    Linter errors - take a look at the linter log file: 01-verilator-lint/verilator-lint.log
    Design too small for routing:

        Very small designs often need these parameters reducing:
        FP_CORE_UTIL (default is 45)
        Try making the design area bigger by reducing FP_CORE_UTIL
        Try spreading out the cells by reducing PL_TARGET_DENSITY_PCT

    Design too small for a power distribution network:

        Set the absolute size of the design:
        set FP_SIZING absolute
        set DIE_AREA to [0,0,200,200] to define a 200x200um area.
        If FP_SIZING is set to absolute then FP_CORE_UTIL is not used.

    Design too big (fails due to congestion or overlaps)

        Don’t set the design to be absolute, and reduce the density - allows the tools to pick the die size
        Use the summary.py tool to check the detailed placement
        Another option is to go back to absolute sizing, choosing 25% larger than what the tools calculated automatically. Use the layout from detailed placement to measure the size of the die on the previous failed run.

    Design fails hold timing:

        Use these variables to add extra hold timing slack (unit is in ns). The default is 0.1.

            PL_RESIZER_HOLD_SLACK_MARGIN to 0.8
            PL_RESIZER_HOLD_SLACK_MARGIN to 0.8

    DRC failures

        Load the xml marker database with KLayout and inspect the errors - this may help you to work out what is going wrong. Instructions for doing this are coming up.
        Ask in the #librelane or #part-4 channel on the discord.

 
Common issues when the tools finish

Run the summary.py --summary tool or check the metrics.csv file by hand. Check all the columns named error or violation. The most common ones are:

    Shorts: short circuits in the routing. This will cause many DRC and LVS issues as well.
    LVS: layout vs schematic:

        This can be due to shorts in the routing. Check to make sure you don’t have shorts.
        This can also mean your design is not driving all its outputs. Check in *-magic-writelef/magic-writelef.log for the keyword ‘Mismatch’ and check your design is driving all the mismatching pins.

    Antenna: the antenna DRC rules are not crucial. They are a bit of a fuzzy check. If we have connecting wires too long they can pick up charge and damage MOSFET gates. LibreLane deals with this by inserting antenna diodes but sometimes there are not enough or there isn’t enough space to do so. Try increasing the size of the design. You can also try changing the diode placement strategy. More information about the antenna report here. It is not vital to get 0 antenna issues.
    DRC issues. Most DRC issues can’t be waived and need to be fixed. The tools are getting better and better, but still sometimes need some help.
    Reports that are -1 are usually due to the check not being run. CVC errors are ok to -1, but DRC and LVS must be 0.

