HOSTAPD_DIR ?= ../hostapd-2.12
HOSTAPD_ABS := $(abspath $(HOSTAPD_DIR))
CHECK_ABS := $(abspath .)
BUILD_CONFIG_ABS := $(abspath ../build-config)
HOSTAPD_CONFIG ?=
CONFIG := $(if $(HOSTAPD_CONFIG),$(abspath $(HOSTAPD_CONFIG)),$(BUILD_CONFIG_ABS)/hostapd-tools.config)
HOSTAPD_PROFILE ?= alpine-hostapd.config
PROFILE := $(BUILD_CONFIG_ABS)/$(HOSTAPD_PROFILE)
EMBED_CONFIG := $(if $(HOSTAPD_CONFIG),$(abspath $(HOSTAPD_CONFIG)),$(PROFILE))
EMBED_EXTRA := $(if $(HOSTAPD_CONFIG),,CONFIG_DEBUG_FILE=y)
BIN ?= hapdconf-check

.PHONY: all clean test test-2.10 test-2.12 test-all test-version
all:
	$(MAKE) -C $(HOSTAPD_ABS)/hostapd -f $(CHECK_ABS)/hostapd-build.mk \
		CONFIG_FILE=$(CONFIG) HOSTAPD_ALPINE_CONFIG=$(PROFILE) \
		TOOL_BUILD_CONFIG_BASE=$(EMBED_CONFIG) TOOL_BUILD_CONFIG_EXTRA=$(EMBED_EXTRA) \
		HAPDCONF_CHECK_DIR=$(CHECK_ABS) OUT=hapdconf-check hapdconf-check
	@cp $(HOSTAPD_ABS)/hostapd/hapdconf-check $(CHECK_ABS)/$(BIN)

clean:
	rm -rf $(HOSTAPD_ABS)/build/hapdconf-check
	rm -f $(BIN)

test: test-2.12

test-2.10:
	$(MAKE) HOSTAPD_DIR=../hostapd-2.10 HOSTAPD_PROFILE=alpine-hostapd-2.10.config BIN=hapdconf-check-2.10 test-version

test-2.12:
	$(MAKE) HOSTAPD_DIR=../hostapd-2.12 HOSTAPD_PROFILE=alpine-hostapd.config BIN=hapdconf-check-2.12 test-version

test-all: test-2.10 test-2.12

test-version: all
	sh ./tests/test-checker.sh ./$(BIN) ./tests $(HOSTAPD_DIR)
