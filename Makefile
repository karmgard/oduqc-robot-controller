SKETCH=oduqc.ino

#$(shell cp $(SKETCH) $(subst .ino,.cpp,$(SKETCH)))

SRC=$(subst .ino,.cpp,$(SKETCH)) axisMotor.cpp circuit.cpp readSerial.cpp
HDR=axisMotor.h circuit.h config.h motorBoss.h motorShield.h readSerial.h
BIN=oduqc

DEVICE=/dev/ttyACM0

TMPDIR := .
#TMPDIR:=$(shell mktemp -u)

BASE_PATH=/usr/local/arduino/hardware
TOOL_PATH=$(BASE_PATH)/tools/avr/bin
AVR_PATH=$(TOOL_PATH)
ETC_PATH=$(BASE_PATH)/tools/avr/etc
CORE_PATH=$(BASE_PATH)/arduino/avr/cores
VAR_PATH=$(BASE_PATH)/arduino/avr/variants
LIB_PATH=$(BASE_PATH)/arduino/avr/libraries

CORESRC=$(CORE_PATH)/arduino/wiring_pulse.S $(CORE_PATH)/arduino/WInterrupts.c \
	$(CORE_PATH)/arduino/hooks.c $(CORE_PATH)/arduino/wiring.c $(CORE_PATH)/arduino/wiring_analog.c \
	$(CORE_PATH)/arduino/wiring_digital.c $(CORE_PATH)/arduino/wiring_pulse.c \
	$(CORE_PATH)/arduino/wiring_shift.c $(CORE_PATH)/arduino/CDC.cpp \
	$(CORE_PATH)/arduino/HardwareSerial.cpp $(CORE_PATH)/arduino/HardwareSerial0.cpp \
	$(CORE_PATH)/arduino/HardwareSerial1.cpp $(CORE_PATH)/arduino/HardwareSerial2.cpp \
	$(CORE_PATH)/arduino/HardwareSerial3.cpp $(CORE_PATH)/arduino/IPAddress.cpp \
	$(CORE_PATH)/arduino/PluggableUSB.cpp $(CORE_PATH)/arduino/Print.cpp \
	$(CORE_PATH)/arduino/Stream.cpp $(CORE_PATH)/arduino/Tone.cpp \
	$(CORE_PATH)/arduino/USBCore.cpp $(CORE_PATH)/arduino/WMath.cpp \
	$(CORE_PATH)/arduino/WString.cpp $(CORE_PATH)/arduino/abi.cpp $(CORE_PATH)/arduino/main.cpp \
	$(CORE_PATH)/arduino/new.cpp

LIBSRC=$(LIB_PATH)/SoftwareSerial/src/SoftwareSerial.cpp \
	$(LIB_PATH)/Adafruit_Motor_Shield_V2_Library/Adafruit_MotorShield.cpp \
	$(LIB_PATH)/Adafruit_Motor_Shield_V2_Library/utility/Adafruit_MS_PWMServoDriver.cpp \
	$(LIB_PATH)/Wire/src/Wire.cpp $(LIB_PATH)/Wire/src/utility/twi.c

# From boards.txt for the UNO board
MCU=atmega328p
CPU=16000000L
BOARD=ARDUINO_AVR_UNO
ARCH=ARDUINO_ARCH_AVR
BAUD=115200
PROTOCOL=arduino

# Version 1.8.5 of the Ardunio IDE suite/builder
AVERSION=10805

CCOPTS=-g -Os -w -std=gnu++11 -fpermissive -fno-exceptions -ffunction-sections \
	-fdata-sections -fno-threadsafe-statics -MMD -flto -mmcu=$(MCU) -DF_CPU=$(CPU) \
	-DARDUINO=$(AVERSION) -D$(BOARD) -D$(ARCH)

ASSOPT=-g -x assembler-with-cpp -flto -MMD -mmcu=$(MCU) -DF_CPU=$(CPU) \
	-DARDUINO=$(AVERSION) -D$(BOARD) -D$(ARCH)

INCLUDES = -I./ \
	-I$(CORE_PATH)/arduino \
	-I$(VAR_PATH)/standard \
	-I$(LIB_PATH)/SoftwareSerial/src \
	-I$(LIB_PATH)/EEPROM/src \
	-I$(LIB_PATH)/Wire/src \
	-I$(LIB_PATH)/Adafruit_Motor_Shield_V2_Library \
	-I$(LIB_PATH)/Adafruit_Motor_Shield_V2_Library/utility \
	-I$(LIB_PATH)/elapsedMillis

CC=$(TOOL_PATH)/avr-g++ $(CCOPTS) $(INCLUDES)
GCC=$(TOOL_PATH)/avr-gcc $(CCOPTS) -I$(CORE_PATH)/arduino -I$(VAR_PATH)/standard
ASM=$(TOOL_PATH)/avr-gcc $(ASSOPT) -I$(CORE_PATH)/arduino -I$(VAR_PATH)/standard
AR=$(TOOL_PATH)/avr-gcc-ar rcs $(TMPDIR)/core/core.a

LNKOPTS=-w -Os -g -flto -fuse-linker-plugin -Wl,--gc-sections,--relax -mmcu=$(MCU) -L. -L$(TMPDIR)
LNK=$(TOOL_PATH)/avr-gcc $(LNKOPTS)

OBJCPY=$(TOOL_PATH)/avr-objcopy 

UPL=$(AVR_PATH)/avrdude -C $(ETC_PATH)/avrdude.conf -v -p$(MCU) -c$(PROTOCOL) -P$(DEVICE) -b$(BAUD) -D

SRCOBJ=$(subst .cpp,.o,$(SRC))
LIBOBJ=$(subst .c,.o,$(subst .cpp,.o,$(LIBSRC)))
COREOBJ=$(subst .S,.o,$(subst .c,.o,$(subst .cpp,.o,$(CORESRC))))

OBJ=$(SRCOBJ) $(LIBOBJ) $(COREOBJ)

LNKOBJ=$(addprefix $(TMPDIR)/sketch/, $(SRCOBJ)) \
	$(addprefix $(TMPDIR)/libraries/, $(notdir $(LIBOBJ))) \
	$(TMPDIR)/core/core.a

LINK=$(TOOL_PATH)/avr-gcc $(LNKOPTS) -lm -L$(TMPDIR)

all: mkdir $(BIN)

$(BIN): mkdir $(SRCOBJ)
	@echo "\n>>>>>>>>>>>> Linking <<<<<<<<<<<<<"

	$(LINK) -o $(BIN).elf $(LNKOBJ)

	$(OBJCPY) -O ihex -j .eeprom \
	  --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 \
	  $(TMPDIR)/$(BIN).elf $(TMPDIR)/$(BIN).eep

	$(OBJCPY) -O ihex -R .eeprom  $(TMPDIR)/$(BIN).elf $(TMPDIR)/$(BIN).hex

	@rm $(BIN).elf $(BIN).eep

static: mkdir $(LIBOBJ) $(COREOBJ)

.cpp.o: mkdir $(HDR)
	@echo "\n>>>>>>>>>>>> Compiling $(notdir $<)  <<<<<<<<<<<<<"
	$(if $(findstring $(LIB_PATH),$<), $(CC) -c $< -o $(TMPDIR)/libraries/$(notdir $@), \
	  $(if $(findstring $(CORE_PATH),$<), $(GCC) -c $< -o $(TMPDIR)/core/$(notdir $@);$(AR) $(TMPDIR)/core/$(notdir $@), \
	    $(CC) -c $< -o $(TMPDIR)/sketch/$(notdir $@)\
	  )\
	)

.c.o: mkdir 
	@echo "\n>>>>>>>>>>>> Compiling $(notdir $<) <<<<<<<<<<<<<"
	$(if $(findstring $(LIB_PATH),$<), $(CC) -c $< -o $(TMPDIR)/libraries/$(notdir $@), \
	  $(if $(findstring $(CORE_PATH),$<), $(GCC) -c $< -o $(TMPDIR)/core/$(notdir $@);$(AR) $(TMPDIR)/core/$(notdir $@), \
	    $(CC) -c $<  -o $(TMPDIR)/sketch/$(notdir $@)\
	  )\
	)

.S.o: mkdir
	@echo "\n>>>>>>>>>>>> Assemblin $(notdir $<) <<<<<<<<<<<<<"
	$(ASM) $< -o $(TMPDIR)/core/$(subst $(dir $<),,$@)
	$(AR) $(TMPDIR)/core/$(subst $(dir $<),,$@)

upload:
	$(UPL) -Uflash:w:$(TMPDIR)/$(BIN).hex:i

backup:
	@tar -zcf $(BIN).tgz $(SRC) $(HDR) $(EXTRAS) Makefile

clean:
	@rm -rf $(TMPDIR)/core
	@rm -rf $(TMPDIR)/sketch
	@rm -rf $(TMPDIR)/libraries
	@rm -f $(TMPDIR)/$(BIN).elf $(TMPDIR)/$(BIN).eep

mkdir:
	@mkdir -p $(TMPDIR)
	@mkdir -p $(TMPDIR)/core
	@mkdir -p $(TMPDIR)/sketch
	@mkdir -p $(TMPDIR)/libraries
