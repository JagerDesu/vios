#include <stdint.h>
#include <stddef.h>

#define IO_ADDR_START (void*)0xF0100000
#define IO_ADDR_SIZE 0xFF
#define IO_ADDR_END ((void*)((uint8_t*)IO_ADDR_START + IO_ADDR_SIZE))
#define IO_REG_READY (*(volatile uint32_t*)IO_ADDR_END)

void display_message(const char* message) {
	char* b = (char*)IO_ADDR_START;
	size_t i = 0;
	while ((i < IO_ADDR_SIZE) && *message) {
		*b = *message;
	}
	IO_REG_READY = 1;
}

void _start() {
	display_message("Hello, Vios!");
}