CC := arm-none-eabi-gcc
TARGET   := donttap
SOURCES  := .
INCLUDES := include
LIBS     := -lnds9

ARCH     := -mthumb -mthumb-interwork
CFLAGS   := -g -Wall -O2 -march=armv5te -mtune=arm946e-s -fomit-frame-pointer
CFLAGS   += -ffast-math $(ARCH) -DARM9
ASFLAGS  := -g $(ARCH)
LDFLAGS  := -specs=ds_arm9.specs -g $(ARCH) -Wl,-Map,$(TARGET).map

.PHONY: all clean

all: $(TARGET).nds

$(TARGET).nds: $(TARGET).elf
	ndstool -c $(TARGET).nds -9 $(TARGET).elf

$(TARGET).elf: $(TARGET).o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

$(TARGET).o: $(TARGET).c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o *.elf *.nds *.map