all: lib hackrf-usb hackrf-tools

force: clean all

ifeq ($(MAKECMDGOALS),rebuild)
FORCE=FORCE
rebuild: all
endif

firmware/libopencm3/README:
	git submodule init

firmware/libopencm3/lib/libopencm3_lpc43xx.a: $(FORCE)
	git submodule update
	$(MAKE) -C firmware/libopencm3

lib: firmware/libopencm3/README firmware/libopencm3/lib/libopencm3_lpc43xx.a

FDIR=firmware/hackrf_usb/build

hackrf-usb: hackrf.bin

hackrf.bin: $(FDIR)/hackrf_usb.bin
	cp $< $@

$(FDIR)/hackrf_usb.bin: $(FDIR) $(FDIR)/Makefile $(FORCE)
	$(MAKE) -C $(FDIR)

$(FDIR)/Makefile:
	cd $(FDIR) && cmake -DBOARD=RAD1O -DRUN_FROM=RAM ..
	
$(FDIR):
	mkdir $(FDIR)

TDIR=host/build

$(TDIR):
	mkdir $(TDIR)

hackrf-tools: $(TDIR) $(TDIR)/hackrf-tools/src/hackrf_info

$(TDIR)/hackrf-tools/src/hackrf_info: $(FORCE)
	cd $(TDIR) && cmake .. && $(MAKE)


clean:
	$(RM) -r $(FDIR) $(TDIR)
	$(MAKE) -C firmware/libopencm3 clean

FORCE:
