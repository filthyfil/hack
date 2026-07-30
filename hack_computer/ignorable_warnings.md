# Ignorable Warnings

## LibreLane will often end with warnings. Some of these can be safely ignored:

WARNING  [OpenROAD.CheckSDCFiles] 'PNR_SDC_FILE' is not defined. Using generic fallback SDC for OpenROAD PnR steps.

[OpenROAD.CheckSDCFiles] 'SIGNOFF_SDC_FILE' is not defined. Using generic fallback SDC for OpenROAD PnR steps.

If no timing SDC files are provided, defaults will be used. You can ignore this if you are not working with multiple macros.

[Odb.CustomIOPlacement] Overriding minimum distance 0.1 with 0.42 for pins on side N to avoid overlap.

Just a notice that the pins are being set wider to avoid overlap - safe to ignore.

OpenROAD.RepairDesignPostGPL] [STA-1140] /foss/pdks/sky130A/libs.ref/sky130_fd_sc_hd/lib/sky130_fd_sc_hd__tt_025C_1v80.lib - library sky130_fd_sc_hd__tt_025C_1v80 already exists. 

When trying to load multiple corners of the same library, OpenROAD emits this warning. Safe to ignore.

[OpenROAD.DetailedRouting] [DRT-0349] LEF58_ENCLOSURE with no CUTCLASS is not supported. Skipping for layer mcon

The LEF files for sky130 have some properties not supported by OpenROAD, so it emits this warning. Safe to ignore.

[Checker.WireLength] Threshold for Threshold-surpassing long wires is not set. The checker will be skipped.

You can set a threshold for the maximum wire length in the circuit where if there is any wire longer than that, an error is raised. It doesn't have a default sane value and for small designs it can be ignored.
