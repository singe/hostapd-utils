# Invoked from $(HOSTAPD_DIR)/hostapd by ifcap/Makefile.  This deliberately
# overlays the upstream build instead of changing any vendored source file.
.DEFAULT_GOAL := ifcap
include Makefile

IFCAP_DIR ?= $(abspath ../../ifcap)
IFCAP_OBJ := $(BUILDDIR)/$(PROJ)/ifcap.o
IFCAP_REG_OBJ := $(BUILDDIR)/$(PROJ)/ifcap-regulatory.o
IFCAP_HOSTAPD_OBJS := $(filter-out %/main.o,$(OBJS))
TOOL_BUILD_CONFIG_HEADER := $(BUILDDIR)/$(PROJ)/tool_build_config.h
TOOL_BUILD_CONFIG_BASE ?= $(CONFIG_FILE)
TOOL_BUILD_CONFIG_EXTRA ?=

# Embed the exact effective profile used by this utility. Numeric bytes avoid
# quoting and escaping differences between shell, make, and C string syntax.
$(TOOL_BUILD_CONFIG_HEADER): $(TOOL_BUILD_CONFIG_BASE) $(CONFIG_FILE) | _make_dirs
	@{ \
		printf 'static const char tool_build_config[] = {'; \
		od -An -v -tu1 "$(TOOL_BUILD_CONFIG_BASE)" | awk 'BEGIN { sep = "" } { for (i = 1; i <= NF; i++) { printf "%s%s", sep, $$i; sep = "," } }'; \
		if [ -n "$(TOOL_BUILD_CONFIG_EXTRA)" ]; then \
			printf '%s\n' "$(TOOL_BUILD_CONFIG_EXTRA)" | od -An -v -tu1 | awk 'BEGIN { sep = "," } { for (i = 1; i <= NF; i++) { printf "%s%s", sep, $$i; sep = "," } }'; \
		fi; \
		printf ',0};\n'; \
	} > $@

$(IFCAP_OBJ): $(IFCAP_DIR)/ifcap.c $(IFCAP_DIR)/ifcap-regulatory.h $(CONFIG_FILE) $(TOOL_BUILD_CONFIG_HEADER) | _make_dirs
	@echo $(CURDIR): '$(CC) -c -o $@ $(CFLAGS) $<' >$@.cmd
	$(Q)$(CC) -c -o $@ $(CFLAGS) -I$(IFCAP_DIR) -I$(dir $(TOOL_BUILD_CONFIG_HEADER)) $<
	@$(E) "  CC " $<

$(IFCAP_REG_OBJ): $(IFCAP_DIR)/ifcap-regulatory.c $(IFCAP_DIR)/ifcap-regulatory.h $(CONFIG_FILE) | _make_dirs
	@echo $(CURDIR): '$(CC) -c -o $@ $(CFLAGS) $<' >$@.cmd
	$(Q)$(CC) -c -o $@ $(CFLAGS) -I$(IFCAP_DIR) $<
	@$(E) "  CC " $<

ifcap: $(IFCAP_HOSTAPD_OBJS) $(IFCAP_OBJ) $(IFCAP_REG_OBJ)
	$(Q)$(CC) $(LDFLAGS) -o $@ $(IFCAP_HOSTAPD_OBJS) $(IFCAP_OBJ) $(IFCAP_REG_OBJ) $(LIBS)
	@$(E) "  LD " $@

.PHONY: ifcap
