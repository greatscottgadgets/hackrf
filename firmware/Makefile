# Hey Emacs, this is a -*- makefile -*-

TARGETS = blinky \
	      mixertx \
		  sgpio \
		  sgpio-rx \
		  simpletx \
		  startup \
		  startup_systick \
		  startup_systick_perfo \
		  usb_performance

#	      blinky_rom_to_ram
#		  sgpio_passthrough_rom_to_ram
#		  startup_systick_perfo_rom_to_ram

all: build

build: examples

examples:
	$(Q)for i in $(TARGETS); do \
		if [ -d $$i ]; then \
			printf "  BUILD   $$i\n"; \
			$(MAKE) -C $$i || exit $?; \
		fi; \
	done

clean:
	$(Q)for i in $(addprefix lib/,$(TARGETS)) \
		     $(TARGETS); do \
		if [ -d $$i ]; then \
			printf "  CLEAN   $$i\n"; \
			$(MAKE) -C $$i clean || exit $?; \
		fi; \
	done

.PHONY: build examples
