## Making files for production of HackRF One

### Check everything

In eeschema, run Inspect->Electrical Rules Checker
The migration to Kicad5 caused some errors with connection to GND and power nets. Not sure yet how to fix these.
The global label TX is no longer used. Don't know how to suppress this warning.

### BOM

In eeschema (schematic editor), select "Tools->Generate Bill of Materials".
Select the BOM plugin "KiCad_BOM_Wizard".
Click Generate. This creates the file hardware/hackrf-one/hackrf-one.bom.csv.
Move that file to doc/hardware/hackrf-one-gerbers/hackrf-one-bom.csv

### Assembly silk with references

In pcbnew (the PCB editor) select "Plot...".
Ensure F.SilkS (front silk) layer is selected.
Select "Use auxiliary axis as origin"
Deselect "Use Protel filename extensions"
De-select "Generate Gerber job file"
Select "Plot footprint references"
Click "Plot". This puts files into doc/hardware/hackrf-one-gerbers.
Rename doc/hardware/hackrf-one-gerbers/hackrf-one-F_SilkS.gbr to doc/hardware/hackrf-one-gerbers/hackrf-one-Placement.gbr

### Gerber files

In pcbnew (the PCB editor) select "Plot...".
Ensure all required layers are selected: C1F, C2, C3, C4B, F.Paste, F.SilkS, F.Mask, B.Mask, Edge.Cuts
De-select "Plot footprint references"
Click "Plot". This puts files into doc/hardware/hackrf-one-gerbers.
Change to that directory.

### Drill file

From pcbnew->Plot, select "Generate Drill Files".
Select "Excellon".
Tick "Minimal headers"
Tick "PTH and NPTH in single file"
Select Drill Origin "Auxiliary Axis"
Select Drill Units "Inches"
Select Zeros Format "Suppress trailing zeros"
Click "Generate Drill File"
This creates the file "doc/hardware/hackrf-one-gerbers/hackrf-one.drl"
Click "Generate Map File"
Click "Generate Report File" and Save

### Placement file

In pcbnew, select File->Fabrication Outputs->Footprint Position (.pos) File
Select CSV, Inches, Single file per board.
Click "Generate Position File"
This makes the file doc/hardware/hackrf-one-gerbers/hackrf-one-all-pos.csv

### Zip it up

cd into doc/hardware/hackrf-one-gerbers
zip -j ../hackrf-one-fabrication-draft-YYYY-NN-DD.zip ../README.production *
