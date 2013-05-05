M2DIR?=../..
include $(M2DIR)/software/common.mak

OBJECTS=crt0.o isr.o main.o

all: dumper.bin

# pull in dependency info for *existing* .o files
-include $(OBJECTS:.o=.d)

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@
	chmod -x $@

dumper.elf: linker.ld $(OBJECTS) libs

%.elf:
	$(LD) $(LDFLAGS) -T $< -N -o $@ $(OBJECTS) \
		-L$(M2DIR)/software/libnet \
		-L$(M2DIR)/software/libbase \
		-L$(M2DIR)/software/libcompiler-rt \
		-lnet -lbase -lcompiler-rt
	chmod -x $@

main.o: main.c
	$(compile-dep)

%.o: %.c
	$(compile-dep)

%.o: %.S
	$(assemble)

libs:
	make -C $(M2DIR)/software/libcompiler-rt
	make -C $(M2DIR)/software/libbase

load: dumper.bin
	make -C $(M2DIR)/tools
	$(M2DIR)/tools/flterm --port /dev/ttyUSB0 --kernel dumper.bin

clean:
	rm -f $(OBJECTS) $(OBJECTS:.o=.d) dumper.elf dumper.bin .*~ *~

.PHONY: main.o clean libs load
