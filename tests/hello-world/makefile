TOOLCHAIN = arm-none-eabi-
CC = $(TOOLCHAIN)gcc
LD = $(TOOLCHAIN)ld
CFLAGS = -mcpu=cortex-a9 -mthumb -mfloat-abi=hard
LDFLAGS =

OBJDIR = obj/
BINDIR = bin/

OBJS := obj/main.o \
		obj/startup.o
BIN = $(BINDIR)hello-world.elf

BIN : $(OBJS)
	$(LD) link.ld $(LDFLAGS) $(OBJS) -o $(BIN)

obj/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(OBJS)