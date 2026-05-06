.PHONY: all build upload clean monitor log clean-logs lib-init help

PLATFORMIO := pio
PYTHON := python3

all: build

build:
	@$(PLATFORMIO) run

upload:
	@$(PLATFORMIO) run --target upload

lib-init:
	@$(PLATFORMIO) pkg install

clean:
	@$(PLATFORMIO) run --target clean

monitor:
	@$(PLATFORMIO) device monitor --baud 115200

log:
	@mkdir -p logs
	@$(PYTHON) scripts/monitor.py

clean-logs:
	@rm -rf logs

help:
	@echo "HomeAutomation – PlatformIO Makefile"
	@echo ""
	@echo "Build:"
	@echo "  make build      - Compile firmware"
	@echo "  make upload     - Build and flash to device"
	@echo "  make clean      - Remove build artifacts"
	@echo ""
	@echo "Monitor:"
	@echo "  make monitor    - Open serial monitor (raw)"
	@echo "  make log        - Serial monitor → logs/serial_*.log"
	@echo "  make clean-logs - Delete all log files"
	@echo ""
	@echo "Other:"
	@echo "  make lib-init   - Install library dependencies"
	@echo "  make help       - Show this help"
