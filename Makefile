ARDUINO_DIR=/opt/arduino
ARDMK_DIR=/usr/share/arduino
AVR_TOOLS_DIR=/usr
BOARD_TAG=uno
MONITOR_PORT=/dev/ttyUSB0

USER_LIB_PATH = ./libraries
ARDUINO_LIBS = SPI Gamebuino-Classic

LOCAL_C_SRCS   ?= $(shell find src -type f -name '*.c' | tr '\n' ' ')
LOCAL_CPP_SRCS ?= $(shell find src -type f -name '*.cpp' | tr '\n' ' ')
LOCAL_AS_SRCS  ?= $(shell find src -type f -name '*.S' | tr '\n' ' ')

include $(ARDMK_DIR)/Arduino.mk

