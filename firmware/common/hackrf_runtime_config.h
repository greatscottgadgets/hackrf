/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef HACKRF_RUNTIME_CONFIG_H
#define HACKRF_RUNTIME_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

/* Mayhem-to-HackRF runtime configuration ABI. Reserve the last 64 bytes of
 * ram_sleep in the LPC4320 linker layout (used by both HackRF One and Pro).
 * This RAM is excluded from C runtime initialization and Mayhem shared memory.
 * Publish only after Mayhem has stopped baseband and shut down its UI.
 *
 * Add runtime fields to the configuration below as needed, updating the ABI
 * version and serialization together. Unknown versions fall back to defaults.
 * Unused mailbox space is reserved for future configuration fields.
 */
/* Producer address for Mayhem; the HackRF consumer uses its linker symbol. */
#define HACKRF_RUNTIME_CONFIG_ADDRESS 0x10089fc0U
#define HACKRF_RUNTIME_CONFIG_CAPACITY 64U
#define HACKRF_RUNTIME_CONFIG_MAGIC 0x48524346U
#define HACKRF_RUNTIME_CONFIG_VERSION 1U
#define HACKRF_RUNTIME_CONFIG_SCREEN_OFF (1U << 0)
#define HACKRF_RUNTIME_CONFIG_KNOWN_FLAGS HACKRF_RUNTIME_CONFIG_SCREEN_OFF

typedef struct {
	uint32_t flags;
} hackrf_runtime_config_t;

/* Wire format: magic, version, payload size, payload, inverted payload flags.
 * All fields are 32-bit words in native little-endian LPC43xx byte order.
 * Defaults use zero flags; missing, malformed or unsupported data is ignored.
 */
static inline void hackrf_runtime_config_write(
	volatile uint32_t* words, const hackrf_runtime_config_t* config)
{
	words[0] = 0;
	words[1] = HACKRF_RUNTIME_CONFIG_VERSION;
	words[2] = sizeof(hackrf_runtime_config_t);
	words[3] = config->flags;
	words[4] = ~config->flags;
	words[0] = HACKRF_RUNTIME_CONFIG_MAGIC;
}

/* Consume once, independently of how the caller applies the configuration. */
static inline hackrf_runtime_config_t hackrf_runtime_config_consume(
	volatile uint32_t* words)
{
	hackrf_runtime_config_t config = {0};
	const uint32_t flags = words[3];
	if (words[0] == HACKRF_RUNTIME_CONFIG_MAGIC &&
	    words[1] == HACKRF_RUNTIME_CONFIG_VERSION &&
	    words[2] == sizeof(hackrf_runtime_config_t) &&
	    words[4] == ~flags && !(flags & ~HACKRF_RUNTIME_CONFIG_KNOWN_FLAGS)) {
		config.flags = flags;
	}
	words[0] = 0;
	return config;
}

#endif
