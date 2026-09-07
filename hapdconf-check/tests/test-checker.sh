#!/bin/sh
set -eu

bin=$1
fixtures=$2
version=${3##*/}
tmp=${TMPDIR:-/tmp}/hapdconf-check-test-$$
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
mkdir -p "$tmp"

"$bin" --build-config >"$tmp/build-config" 2>"$tmp/build-config.err"
test ! -s "$tmp/build-config.err"
grep -q '^CONFIG_EAP=y$' "$tmp/build-config"
if [ "$version" = hostapd-2.12 ]; then
  grep -q '^CONFIG_IEEE80211BE=y$' "$tmp/build-config"
fi
grep -q '^CONFIG_DEBUG_FILE=y$' "$tmp/build-config"

"$bin" "$fixtures/valid.conf" >"$tmp/out" 2>"$tmp/err"
test ! -s "$tmp/out"

if [ "$version" = hostapd-2.12 ]; then
  "$bin" "$fixtures/newer-2.12.conf" >"$tmp/out" 2>"$tmp/err"
  test ! -s "$tmp/out"
else
  if "$bin" "$fixtures/newer-2.12.conf" >"$tmp/out" 2>"$tmp/err"; then
    echo '2.10 unexpectedly accepted a 2.12-only directive' >&2
    exit 1
  fi
  test ! -s "$tmp/out"
  grep -q 'unknown configuration item' "$tmp/err"
fi

for fixture in unknown.conf semantic-invalid.conf; do
	if "$bin" "$fixtures/$fixture" >"$tmp/out" 2>"$tmp/err"; then
		echo "unexpectedly accepted $fixture" >&2
		exit 1
	fi
	test ! -s "$tmp/out"
	test -s "$tmp/err"
done
