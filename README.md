# hostapd-utils

hostapd-utils contains small tools built on hostapd's own code: `ifcap` reports the Wi-Fi capability model of a Linux wireless interface, and `hapdconf-check` validates hostapd configuration files using hostapd's parser and semantic checks.

## Source setup

The hostapd releases are pinned as submodules. Clone this repository with `--recurse-submodules`, or initialize them in an existing checkout with `git submodule update --init --recursive`.

## Build and test

On Debian or another Linux distribution, install a C toolchain, libnl-3/libnl-genl-3 development headers, and OpenSSL development headers. Build and run `ifcap` with:

```sh
make -C ifcap
sudo ./ifcap/ifcap wlan0 > wlan0-capabilities.json
```

Build and run the configuration checker with:

```sh
make -C hapdconf-check
./hapdconf-check/hapdconf-check /etc/hostapd/hostapd.conf
```

Run the complete hostapd release matrix with:

```sh
make -C ifcap test-all
make -C hapdconf-check test-all
```

The individual `test-2.10` and `test-2.12` targets are also available. Live `ifcap` JSON validation requires an interface and can be run with `make -C ifcap test-live-2.10 IFCAP_INTERFACE=wlan0` or `make -C ifcap test-live-2.12 IFCAP_INTERFACE=wlan0`.

## Build profiles

The checked-in profiles in `build-config` are derived from Alpine Linux build recipes. The 2.10 profile is pinned to Alpine commit `84a227baf001b6e0208e3352b294e4d7a40e93de`; the 2.12 profile uses the current local hostapd 2.12 `defconfig`. Refresh either profile with `build-config/refresh-alpine-config.sh 2.10 --apply` or `build-config/refresh-alpine-config.sh 2.12 --apply` after reviewing the diff.

`ifcap` emits JSON schema `1.2`. The 2.10 build omits hostapd 2.12-only EHT data; the 2.12 build includes it. Diagnostics are written to stderr so stdout remains machine-readable.

See [`ifcap/README.md`](ifcap/README.md), [`hapdconf-check/README.md`](hapdconf-check/README.md), and [`build-config/README.md`](build-config/README.md) for details.
