#include <stdint.h>
#include <stddef.h>

#define IO_ADDR_START (void*)0xF0100000
#define IO_ADDR_SIZE 0x1000
#define IO_ADDR_END ((void*)((uint8_t*)IO_ADDR_START + IO_ADDR_SIZE))
#define IO_REG_READY (*(volatile uint32_t*)IO_ADDR_END)

void init_io() {
	IO_REG_READY = 1;
}

void display_message(const char* message) {
	while (!IO_REG_READY) {} /* Effectively a hacky fence */
	
	uint8_t* out = (uint8_t*)IO_ADDR_START;
	const uint8_t* out_end = (uint8_t*)IO_ADDR_END;
	const char* in = message;

	/* Perform an extremely basic copy */
	while ((out < out_end) && *in) {
		*out = (uint8_t)(*in);
		out++;
		in++;
	}

	/* Let the device know it needs to do some work */
	IO_REG_READY = 0;
}

void test_entry() {
	init_io();
	display_message("Hello, world, from Vios!");
	display_message("This is a second message to test the I/O \"fence.\"");
}