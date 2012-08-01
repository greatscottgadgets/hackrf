#include <hackrf_core.h>

#include <libopencm3/lpc43xx/cgu.h>

int main(void) {

	pin_setup();
	enable_1v8_power();
	cpu_clock_init();

	CGU_BASE_PERIPH_CLK = (CGU_BASE_CLK_AUTOBLOCK
			| (CGU_SRC_PLL1 << CGU_BASE_CLK_SEL_SHIFT));

	CGU_BASE_APB1_CLK = (CGU_BASE_CLK_AUTOBLOCK
			| (CGU_SRC_PLL1 << CGU_BASE_CLK_SEL_SHIFT));
	
	while (1) {

	}

	return 0;
}
