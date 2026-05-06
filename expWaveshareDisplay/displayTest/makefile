# ===== Arduino CLI Makefile =====

# Defaults (override in command line)
PORT ?= /dev/ttyACM0
FQBN ?= esp32:esp32:esp32s3:CDCOnBoot=cdc,CPUFreq=240,DebugLevel=none,DFUOnBoot=default,EraseFlash=none,EventsCore=1,FlashMode=qio,FlashSize=8M,JTAGAdapter=default,LoopCore=1,MSCOnBoot=default,PartitionScheme=default,PSRAM=opi,UploadMode=default
BAUD ?= 115200
SKETCH ?= .

# ---- Core targets ----
compile:
	arduino-cli compile --fqbn $(FQBN) $(SKETCH)

upload:
	arduino-cli upload -p $(PORT) --fqbn $(FQBN) $(SKETCH)

monitor:
	arduino-cli monitor -p $(PORT) -c baudrate=$(BAUD)

plot:
	serialplot -p $(PORT) -b $(BAUD)

all: compile upload

help:
	@echo ""
	@echo "Targets:"
	@echo "  make compile"
	@echo "  make upload"
	@echo "  make monitor"
	@echo "  make plot"
	@echo "  make all"
	@echo ""
	@echo "Override vars:"
	@echo "  make upload PORT=/dev/ttyUSB0"
	@echo "  make compile FQBN=arduino:avr:nano"
	@echo "  make monitor BAUD=9600"
	@echo ""

