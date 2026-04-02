# ===== Arduino CLI Makefile =====

# Defaults (override in command line)
PORT ?= /dev/ttyUSB0
FQBN ?= arduino:avr:uno
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
