#!/bin/sh
set -eu

dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
version=2.12
apply=false
for arg do
	case "$arg" in
		2.10|2.12) version=$arg ;;
		--apply) apply=true ;;
		*) echo "usage: $0 [2.10|2.12] [--apply]" >&2; exit 2 ;;
	esac
done
source=$(CDPATH= cd -- "$dir/../hostapd-$version/hostapd" && pwd)/defconfig
target="$dir/alpine-hostapd-$version.config"
[ "$version" = 2.12 ] && target="$dir/alpine-hostapd.config"
tmp=$(mktemp "${TMPDIR:-/tmp}/hostapd-alpine-$version-config.XXXXXX")
trap 'rm -f "$tmp"' EXIT HUP INT TERM

if [ "$version" = 2.10 ]; then
	sed \
		-e '/^#CONFIG_DRIVER_NL80211=y/s/^#//' \
		-e '/^#CONFIG_RADIUS_SERVER=y/s/^#//' \
		-e '/^#CONFIG_DRIVER_WIRED=y/s/^#//' \
		-e '/^#CONFIG_DRIVER_NONE=y/s/^#//' \
		-e '/^#CONFIG_IEEE80211N=y/s/^#//' \
		-e '/^#CONFIG_IEEE80211R=y/s/^#//' \
		-e '/^#CONFIG_IEEE80211AC=y/s/^#//' \
		-e '/^#CONFIG_IEEE80211AX=y/s/^#//' \
		-e '/^#CONFIG_FULL_DYNAMIC_VLAN=y/s/^#//' \
		-e '/^#CONFIG_LIBNL32=y/s/^#//' \
		-e '/^#CONFIG_ACS=y/s/^#//' \
		-e '/^#CONFIG_WEP=y/s/^#//' \
		-e '/^#CONFIG_SAE=y/s/^#//' \
		"$source" >"$tmp"
else
	sed \
		-e '/^#CONFIG_RADIUS_SERVER=y/s/^#//' \
		-e '/^#CONFIG_DRIVER_WIRED=y/s/^#//' \
		-e '/^#CONFIG_DRIVER_NONE=y/s/^#//' \
		-e '/^#CONFIG_IEEE80211N=y/s/^#//' \
		-e '/^#CONFIG_WNM=y/s/^#//' \
		-e '/^#CONFIG_IEEE80211R=y/s/^#//' \
		-e '/^#CONFIG_IEEE80211AC=y/s/^#//' \
		-e '/^#CONFIG_IEEE80211AX=y/s/^#//' \
		-e '/^#CONFIG_IEEE80211BE=y/s/^#//' \
		-e '/^#CONFIG_FULL_DYNAMIC_VLAN=y/s/^#//' \
		-e '/^#CONFIG_LIBNL32=y/s/^#//' \
		-e '/^#CONFIG_ACS=y/s/^#//' \
		-e '/^#CONFIG_WEP=y/s/^#//' \
		-e '/^#CONFIG_SAE=y/s/^#//' \
		-e '/^#CONFIG_ELOOP_EPOLL=y/s/^#//' \
		-e '/^#CONFIG_FST=y/s/^#//' \
		-e '/^#CONFIG_FST_TEST=y/s/^#//' \
		-e '/^#CONFIG_MBO=y/s/^#//' \
		-e '/^#CONFIG_WPA_CLI_EDIT=y/s/^#//' \
		-e '/^#CONFIG_AIRTIME_POLICY=y/s/^#//' \
		-e '/^#CONFIG_OCV=y/s/^#//' \
		"$source" >"$tmp"
fi

cat >>"$tmp" <<'EOF'

CC ?= gcc
CFLAGS += -I/usr/include/libnl3
LIBS += -L/usr/lib
EOF

if cmp -s "$target" "$tmp"; then
	echo 'Alpine Linux hostapd config snapshot is current.'
	exit 0
fi

diff -u "$target" "$tmp" || true
if $apply; then
	mv "$tmp" "$target"
	trap - EXIT HUP INT TERM
	echo "Updated $target"
else
	echo 'Snapshot differs; review the diff and rerun with --apply to update.' >&2
	exit 1
fi
