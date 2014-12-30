# Target specific macros
TARGET = BTdrive2
TARGET_SOURCES = \
	BTdrive2.c
TOPPERS_OSEK_OIL_SOURCE = ./BTdrive2.oil

# Don't modify below part
O_PATH ?= build
include ../../ecrobot/ecrobot.mak
