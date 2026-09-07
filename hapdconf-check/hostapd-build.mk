# Invoked from $(HOSTAPD_DIR)/hostapd. It overlays, but never modifies, the
# upstream build and keeps the only source-version-sensitive glue in one file.
.DEFAULT_GOAL := hapdconf-check
include Makefile

HAPDCONF_CHECK_DIR ?= $(abspath ../../hapdconf-check)
HAPDCONF_CHECK_OBJ := $(BUILDDIR)/$(PROJ)/hapdconf-check.o
HAPDCONF_CHECK_HOSTAPD_OBJS := $(filter-out %/main.o,$(OBJS))
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

$(HAPDCONF_CHECK_OBJ): $(HAPDCONF_CHECK_DIR)/hapdconf-check.c $(CONFIG_FILE) $(TOOL_BUILD_CONFIG_HEADER) | _make_dirs
	@echo $(CURDIR): '$(CC) -c -o $@ $(CFLAGS) $<' >$@.cmd
	$(Q)$(CC) -c -o $@ $(CFLAGS) -I$(HAPDCONF_CHECK_DIR) -I$(CURDIR) -I$(dir $(TOOL_BUILD_CONFIG_HEADER)) $<
	@$(E) "  CC " $<

hapdconf-check: $(HAPDCONF_CHECK_HOSTAPD_OBJS) $(HAPDCONF_CHECK_OBJ)
	$(Q)$(CC) $(LDFLAGS) -o $@ $(HAPDCONF_CHECK_HOSTAPD_OBJS) $(HAPDCONF_CHECK_OBJ) $(LIBS)
	@$(E) "  LD " $@

.PHONY: hapdconf-check
