DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM

CC       := $(DEVKITARM)/bin/arm-none-eabi-gcc
NDSTOOL  := $(DEVKITARM)/bin/ndstool

TARGET   := donttap
SOURCES  := .
LIBS     := -L$(DEVKITPRO)/libnds/lib -lnds9
ARCH     := -mthumb -mthumb-interwork
CFLAGS   := -g -Wall -O2 -march=armv5te -mtune=arm946e-s -fomit-frame-pointer
CFLAGS   += -ffast-math $(ARCH) -DARM9 -I$(DEVKITPRO)/libnds/include
LDFLAGS  := -specs=ds_arm9.specs -g $(ARCH) -Wl,-Map,$(TARGET).map -L$(DEVKITPRO)/libnds/lib

all: $(TARGET).nds

$(TARGET).nds: $(TARGET).elf
	$(NDSTOOL) -c $(TARGET).nds -9 $(TARGET).elf

$(TARGET).elf: $(TARGET).o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

$(TARGET).o: $(TARGET).c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o *.elf *.nds *.map
