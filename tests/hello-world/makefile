TOOLCHAIN = arm-none-eabi-
CC = $(TOOLCHAIN)gcc
LD = $(TOOLCHAIN)ld
OBJDUMP = $(TOOLCHAIN)objdump
CFLAGS = -mcpu=cortex-a9 -mthumb -mfloat-abi=hard -mfpu=neon-vfpv4 -ffreestanding
LDFLAGS = -nostdlib 

OBJDIR = obj/
BINDIR = bin/

OBJS := \
	obj/startup.o \
	obj/main.o

BIN = $(BINDIR)hello-world.elf

BIN : $(OBJS)
	$(LD) link.ld $(LDFLAGS) $(OBJS) -o $(BIN)
	$(OBJDUMP) bin/hello-world.elf --disassemble-all -D > dis.txt

disassemble:
	$(OBJDUMP) bin/hello-world.elf --disassemble-all -D > dis.txt

clean:
	rm $(OBJS) $(BIN) dis.txt

obj/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@