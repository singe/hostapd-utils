# ifcap

`ifcap` reports an interface's Wi-Fi capability model using hostapd's own nl80211 driver code. It does not parse `iw` output or duplicate nl80211 attribute parsing.

## Build and run

On Linux, install a C toolchain, libnl-3/libnl-genl-3 development headers, and OpenSSL development headers. Then run:

```sh
make -C ifcap
sudo ./ifcap/ifcap wlan0 > wlan0-capabilities.json
```

Run the release test matrix with `make -C ifcap test-all`; individual targets are available as `test-2.10` and `test-2.12`.

`HOSTAPD_DIR` may point at a replacement hostapd source directory when porting:
`make -C ifcap HOSTAPD_DIR=../hostapd-NEW`.

The tool emits exactly one JSON document to stdout on success. Diagnostics go to stderr; missing arguments and query failures return non-zero.

The nl80211 AP-side initialization used by hostapd may bring the target interface up, so run it as root (or with the capability needed to administer the wireless interface). It does not create an AP or configure a channel.

## Schema and porting

Schema `1.2` is additive: existing fields will not change meaning; later hostapd ports may add optional fields in a later minor schema version. Bitfields always retain a hexadecimal value, including unknown future bits, and byte arrays are lowercase hexadecimal.

Each channel has an `ap` interpretation derived from the active nl80211 regulatory rules and hostapd's driver DFS state. `ap.usable` means immediately startable in AP mode: a radar channel is not usable until its DFS state is `available` (CAC has completed). `ap.requires_dfs_cac` identifies a radar channel that is otherwise usable after CAC, and `ap.dfs_cac_ms` is `null` when the kernel/driver does not publish a duration. `dfs_state` is hostapd's raw DFS state (`unknown`, `usable`, `available`, or `unavailable`). The same fields are present on each `channel_blocks` entry, which is usable only when every constituent 20 MHz channel is immediately usable and the block is aligned with hostapd's operating-center rules. `regulatory_radar` identifies radar learned from the active kernel regulatory rules even when the underlying hostapd channel flags did not contain `RADAR`.

`regulatory.alpha2` and `regulatory.nl80211_dfs_region` are the active kernel regulatory values at query time. If a hostapd configuration changes the country (for example with `country_code`), run ifcap after that hostapd instance has applied it; ifcap does not invent a future country setting from a config file.

The porting surface is intentionally small: review `ifcap.c` against `struct hostapd_hw_modes`, `struct wpa_driver_capa`, and the nl80211 driver initialization callbacks; then update the shared build profile and overlay if the upstream Makefile changes. `ifcap` invokes `hapd_init`, `get_capa`, and `hostapd_get_hw_feature_data`, so it uses the same driver path as hostapd.

`hostapd_select_hw_mode()` is not invoked because it evaluates a proposed hostapd configuration; ifcap v1 deliberately reports the model that selection would consume rather than inventing configuration verdicts.

## Build profile

`ifcap` and `hapdconf-check` use the same checked-in, Alpine Linux-derived hostapd profile in [`../build-config`](../build-config). It enables the common hostapd feature set, including integrated EAP and 802.11ac/ax/be support for the 2.12 build. The 2.10 build omits hostapd 2.12-only EHT data.

Show the exact profile embedded in a built binary:

```
./ifcap --build-config
```

To build against a different profile, set `HOSTAPD_CONFIG=/path/to/.config`.
