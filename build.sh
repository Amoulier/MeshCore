#!/usr/bin/env bash
set -euo pipefail

readonly SUPPORTED_TARGETS=(
  heltec_v4_repeater
  heltec_v4_solar_repeater
  heltec_v4_expansionkit_repeater
  heltec_v4_repeater_bridge_espnow
  heltec_v4_room_server
  heltec_v4_terminal_chat
  heltec_v4_companion_radio_usb
  heltec_v4_companion_radio_ble
  heltec_v4_companion_radio_wifi
  heltec_v4_sensor
  heltec_v4_kiss_modem
)

if ! command -v pio >/dev/null 2>&1; then
  echo "error: PlatformIO is not installed or 'pio' is not in PATH" >&2
  echo "install it with: python -m pip install --upgrade platformio" >&2
  exit 1
fi

is_supported_target() {
  local candidate="$1"
  local supported
  for supported in "${SUPPORTED_TARGETS[@]}"; do
    [[ "$candidate" == "$supported" ]] && return 0
  done
  return 1
}

if (( $# > 0 )); then
  targets=("$@")
else
  targets=("${SUPPORTED_TARGETS[@]}")
fi

for target in "${targets[@]}"; do
  if ! is_supported_target "$target"; then
    echo "error: unsupported target '$target'" >&2
    echo "supported Heltec V4 targets:" >&2
    printf '  %s\n' "${SUPPORTED_TARGETS[@]}" >&2
    exit 2
  fi
done

for target in "${targets[@]}"; do
  echo
  echo "==> Building ${target}"
  pio run -e "$target"
done

echo
echo "All requested Heltec V4 builds completed successfully."
