# Shared hostapd build configuration

`alpine-hostapd.config` is a checked-in snapshot generated from the local hostapd `defconfig` using Alpine Linux's hostapd build configuration. It is deliberately versioned here: builds never fetch configuration from the network.

`alpine-hostapd-2.10.config` uses the Alpine 2.10 recipe from commit `84a227baf001b6e0208e3352b294e4d7a40e93de`.

Both utilities use `hostapd-tools.config`, which imports that snapshot and adds `CONFIG_DEBUG_FILE=y`. The latter is needed so hostapd diagnostics remain on stderr while `ifcap` reserves stdout for JSON.

Run `./refresh-alpine-config.sh 2.12` or `./refresh-alpine-config.sh 2.10` to compare a tracked snapshot with the corresponding local hostapd `defconfig`. Use `--apply` to replace it after reviewing the diff.
