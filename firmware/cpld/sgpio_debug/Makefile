# xst -intstyle ise -ifn top.xst -ofn top.syr
# ngdbuild -intstyle ise -dd _ngo -uc top.ucf -p xc2c64a-VQ100-7 top.ngc top.ngd
# cpldfit -intstyle ise -p xc2c64a-7-VQ100 -ofmt vhdl -optimize density -loc on -slew slow -init low -inputs 32 -pterms 28 -unused pullup -terminate float -iostd LVCMOS33 top.ngd
# tsim -intstyle ise top top.nga
# taengine -intstyle ise -f top -l top.tim -e {taengine.err}
# hprep6 -s IEEE1149 -i top
# vhdtdtfi -prj sgpio_debug -o top.vhi -module top -template /opt/Xilinx/14.7/ISE_DS/ISE/data/vhdlinst.tft -deleteonerror -lib work top.vhd

DESIGN=top

DEVICE=xc2c64a
DEVICE_SPEED=7
DEVICE_PACKAGE=VQ100

OUTFILES_XST=$(DESIGN).ngc $(DESIGN).syr $(DESIGN).ngr $(DESIGN)_xst.xrpt $(DESIGN).lso _xmsgs/xst.xmsgs
OUTFILES_NGDBUILD=$(DESIGN).ngd $(DESIGN).bld $(DESIGN)_ngdbuild.xrpt _xmsgs/ngdbuild.xmsgs _ngo/netlist.lst xlnx_auto_0_xdb/cst.xbcd
OUTFILES_CPLDFIT=$(DESIGN).vm6 $(DESIGN).tspec t6.phd t6.dat t55.phd t55.dat t1.phd t1.dat $(DESIGN).log $(DESIGN).chk $(DESIGN).rpt $(DESIGN).xml $(DESIGN)_build.xml $(DESIGN).pad $(DESIGN)_pad.csv $(DESIGN).pnx $(DESIGN).mfd $(DESIGN).cxt $(DESIGN).gyd _xmsgs/cpldfit.xmsgs
OUTFILES_HPREP6=$(DESIGN).jed tmperr.err _xmsgs/hprep6.xmsgs
OUTFILES_IMPACT_SVF=default.svf _impactbatch.log
OUTFILES_IMPACT_XSVF=default.xsvf _impactbatch.log

all: default.svf default.xsvf

$(DESIGN).ngc: $(DESIGN).xst $(DESIGN).prj
	mkdir -p xst/projnav.tmp
	xst -intstyle ise -ifn $(DESIGN).xst -ofn $(DESIGN).syr

$(DESIGN).ngd: $(DESIGN).ngc $(DESIGN).ucf
	ngdbuild -intstyle ise -dd _ngo -uc $(DESIGN).ucf -p $(DEVICE)-$(DEVICE_PACKAGE)-$(DEVICE_SPEED) $(DESIGN).ngc $(DESIGN).ngd

$(DESIGN).vm6: $(DESIGN).ngd
	cpldfit -intstyle ise -p $(DEVICE)-$(DEVICE_SPEED)-$(DEVICE_PACKAGE) -ofmt vhdl -optimize speed -loc on -slew slow -init low -inputs 32 -pterms 36 -unused pullup -terminate float -iostd LVCMOS33 $(DESIGN).ngd

$(DESIGN).jed: $(DESIGN).vm6
	hprep6 -s IEEE1149 -i $(DESIGN).vm6

default.svf: $(DESIGN).jed batch_svf
	impact -batch batch_svf

default.xsvf: $(DESIGN).jed batch_xsvf
	impact -batch batch_xsvf

.PHONY: clean

clean:
	rm -f $(OUTFILES_XST) $(OUTFILES_NGDBUILD) $(OUTFILES_CPLDFIT) $(OUTFILES_HPREP6) $(OUTFILES_IMPACT_SVF) $(OUTFILES_IMPACT_XSVF)
	rm -rf xlnx_auto_0_xdb/ _ngo/ _xmsgs/ xst/
