#!/bin/sh
set -eu
bin=$1
version=${2##*/}
if "$bin" >/tmp/ifcap-usage.out 2>/tmp/ifcap-usage.err; then
  echo "ifcap accepted missing interface" >&2
  exit 1
fi
grep -q '^usage:' /tmp/ifcap-usage.err
"$bin" --build-config >/tmp/ifcap-build-config.out 2>/tmp/ifcap-build-config.err
test ! -s /tmp/ifcap-build-config.err
if [ "$version" = hostapd-2.12 ]; then
  grep -q '^CONFIG_IEEE80211BE=y$' /tmp/ifcap-build-config.out
fi
grep -q '^CONFIG_DEBUG_FILE=y$' /tmp/ifcap-build-config.out
if strings "$bin" | grep -Fq "${version#hostapd-}-hostap_"; then
  echo "ifcap embedded a Git suffix in the hostapd version" >&2
  exit 1
fi
