HOSTAPD_DIR ?= ../hostapd-2.12
HOSTAPD_ABS := $(abspath $(HOSTAPD_DIR))
IFCAP_ABS := $(abspath .)
BUILD_CONFIG_ABS := $(abspath ../build-config)
HOSTAPD_CONFIG ?=
CONFIG := $(if $(HOSTAPD_CONFIG),$(abspath $(HOSTAPD_CONFIG)),$(BUILD_CONFIG_ABS)/hostapd-tools.config)
HOSTAPD_PROFILE ?= alpine-hostapd.config
PROFILE := $(BUILD_CONFIG_ABS)/$(HOSTAPD_PROFILE)
EMBED_CONFIG := $(if $(HOSTAPD_CONFIG),$(abspath $(HOSTAPD_CONFIG)),$(PROFILE))
EMBED_EXTRA := $(if $(HOSTAPD_CONFIG),,CONFIG_DEBUG_FILE=y)
BIN ?= ifcap

.PHONY: all clean test test-2.10 test-2.12 test-all test-version test-live test-live-2.10 test-live-2.12 test-live-version
all:
	$(MAKE) -C $(HOSTAPD_ABS)/hostapd -f $(IFCAP_ABS)/hostapd-build.mk \
		CONFIG_FILE=$(CONFIG) HOSTAPD_ALPINE_CONFIG=$(PROFILE) \
		TOOL_BUILD_CONFIG_BASE=$(EMBED_CONFIG) TOOL_BUILD_CONFIG_EXTRA=$(EMBED_EXTRA) \
		IFCAP_DIR=$(IFCAP_ABS) OUT=$(BIN) ifcap
	@cp $(HOSTAPD_ABS)/hostapd/ifcap $(IFCAP_ABS)/$(BIN)

clean:
	rm -rf $(HOSTAPD_ABS)/build/$(BIN)
	rm -f $(IFCAP_ABS)/$(BIN)

test: test-2.12

test-2.10:
	$(MAKE) HOSTAPD_DIR=../hostapd-2.10 HOSTAPD_PROFILE=alpine-hostapd-2.10.config BIN=ifcap-2.10 test-version

test-2.12:
	$(MAKE) HOSTAPD_DIR=../hostapd-2.12 HOSTAPD_PROFILE=alpine-hostapd.config BIN=ifcap-2.12 test-version

test-all: test-2.10 test-2.12

test-version: all
	sh ./tests/test-json.sh ./$(BIN) $(HOSTAPD_DIR)

test-live: all
	@test -n "$(IFCAP_INTERFACE)" || (echo 'Set IFCAP_INTERFACE, e.g. make test-live IFCAP_INTERFACE=wlan0' >&2; exit 2)
	./ifcap "$(IFCAP_INTERFACE)" | python3 -c 'import json,sys; x=json.load(sys.stdin); assert x.get("schema_version") == "1.2"; assert all("ap" in c and "dfs_state" in c and "regulatory_radar" in c for m in x.get("hardware_modes", []) for c in m.get("channels", [])); assert all(b.get("center_frequency_mhz") != 5670 for m in x.get("hardware_modes", []) for b in m.get("channel_blocks", []) if b.get("width_mhz") == 80), "invalid VHT80 center channel 134 emitted"'

test-live-2.10:
	$(MAKE) HOSTAPD_DIR=../hostapd-2.10 HOSTAPD_PROFILE=alpine-hostapd-2.10.config BIN=ifcap-2.10 test-live-version

test-live-2.12:
	$(MAKE) HOSTAPD_DIR=../hostapd-2.12 HOSTAPD_PROFILE=alpine-hostapd.config BIN=ifcap-2.12 test-live-version

test-live-version: all
	@test -n "$(IFCAP_INTERFACE)" || (echo 'Set IFCAP_INTERFACE, e.g. make test-live-2.10 IFCAP_INTERFACE=wlan0' >&2; exit 2)
	./$(BIN) "$(IFCAP_INTERFACE)" | python3 -c 'import json,sys; x=json.load(sys.stdin); assert x.get("schema_version") == "1.2"'
