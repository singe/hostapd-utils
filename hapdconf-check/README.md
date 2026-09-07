# hapdconf-check

`hapdconf-check` validates a hostapd configuration using the parser and semantic checks from the hostapd source tree it was built against. It does not initialize a driver, access a wireless interface, or test adapter support.

## Build and run

```sh
make -C hapdconf-check
./hapdconf-check/hapdconf-check /etc/hostapd/hostapd.conf
```

Run the release test matrix with `make -C hapdconf-check test-all`; individual targets are available as `test-2.10` and `test-2.12`.

The command writes hostapd diagnostics to stderr, emits nothing on stdout, and returns 0 only for a valid complete configuration. Referenced PSK, ACL, EAP, and other auxiliary configuration files are read and validated as hostapd normally would.

The default is a broad feature profile. To validate the exact optional-feature
set of a deployment build, use its configuration at build time:

```sh
make -C hapdconf-check HOSTAPD_DIR=../hostapd-2.10 \
  HOSTAPD_CONFIG=/path/to/hostapd/.config
```

`HOSTAPD_DIR` selects the hostapd release. Thus a 2.10-built checker rejects a 2.12-only directive such as `i2r_lmr_policy`, while a compatible 2.12 build accepts it. The porting surface is the small wrapper, build overlay, and broad build profile; the validator itself is `hostapd_config_read()`.

## Build profile and feature diagnostics

`hapdconf-check` shares the checked-in, Alpine Linux-derived hostapd build profile in [`../build-config`](../build-config) with `ifcap`. It includes the integrated EAP server, so `eap_server`, certificate, and private-key settings are parsed by default.

Inspect the exact profile embedded in a binary:

```
./hapdconf-check --build-config
```

When a recognised directive is gated by a disabled hostapd feature, the checker reports the required `CONFIG_*` option and points to this command. It leaves all other syntax and semantic diagnostics to hostapd itself.

Build with an alternative profile using `HOSTAPD_CONFIG=/path/to/.config`.
