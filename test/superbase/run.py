"""Run SuperBase production helpers against deterministic hardware/storage failures."""
from pathlib import Path
import argparse
import shutil
import subprocess
import tempfile
import runpy
import hashlib
import json

root = Path(__file__).resolve().parents[2]
emoji_data = root / "src/helpers/ui/emoji"
manifest = json.loads((emoji_data / "manifest.json").read_text())
canonical = (emoji_data / "OpenMojiData.inc").read_text(encoding="ascii").encode("ascii")
assert hashlib.sha256(canonical).hexdigest() == manifest["generated_sha256"], "Emoji data does not match its manifest"
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--sanitize", action="store_true", help="Enable AddressSanitizer and UBSan (Linux)")
args = parser.parse_args()
patchers = runpy.run_path(str(root / "superbase-dfu.py"))
patch_dfu = patchers["patch_bledfu"]
patch_bonding = patchers["patch_bonding"]
anchor = "peer_data->crc16 = crc16((uint8_t*) peer_data, offsetof(peer_data_t, crc16));"
fixture = ("static void bledfu_control_wr_authorize_cb() {\n"
           "reply.params.write.gatt_status = BLE_GATT_STATUS_SUCCESS;\n"
           "      // Peer data information so that bootloader could re-connect after reboot\n" + anchor +
           "\n      bootloader_util_app_start(NRF_UICR->NRFFW[0]);\n}")
patched = patch_dfu(fixture)
assert patched.count("superbaseDfuAdaptPeerHandoff();") == 2
assert patched.count("superbaseDfuJumpToBootloader(") == 2
assert patched.count("reply.params.write.update = 1;") == 1
for incompatible in ("changed", fixture + anchor, fixture.replace("bootloader_util_app_start", "changed")):
    try:
        patch_dfu(incompatible)
    except RuntimeError:
        pass
    else:
        raise AssertionError("Changed upstream DFU must fail the build hook")
bonding_fixture = (root / "test/superbase/bonding_upstream.inc").read_text()
patched_bonding = patch_bonding(bonding_fixture)
assert '#include "utility/bonding.h"' in patched_bonding
assert patched_bonding.count("const uint32_t cccd_offset = file.position();") == 1
assert patched_bonding.count("VERIFY(file.seek(cccd_offset), );") == 1
assert "file.seek(file.size() - (len + 1));" not in patched_bonding
for incompatible in (
    "changed",
    bonding_fixture + "static void bond_save_cccd_dfr (",
    bonding_fixture.replace("// skip name", "// changed upstream field"),
    bonding_fixture.replace("bdata_write(&file, sys_attr, len);", "changed_write();"),
    bonding_fixture.replace("file.seek(file.size() - (len + 1));", "changed_seek();"),
    patched_bonding,
):
    try:
        patch_bonding(incompatible)
    except RuntimeError:
        pass
    else:
        raise AssertionError("Changed upstream CCCD storage must fail the build hook")
compiler = shutil.which("g++")
if not compiler:
    raise SystemExit("Install g++ to run the SuperBase regressions")

def unique_section(source, start, end):
    """Compile real production methods; source layout changes must fail closed."""
    if source.count(start) != 1 or source.count(end) != 1:
        raise AssertionError("GPS production extraction anchors changed or are ambiguous")
    begin = source.index(start)
    finish = source.index(end, begin)
    return source[begin:finish]

gps_declaration = unique_section(
    (root / "variants/muzi_base/target.h").read_text(encoding="utf-8"),
    "class MuziBaseSensorManager : public EnvironmentSensorManager {",
    "extern MuziBaseSensorManager sensors;")
gps_methods = unique_section(
    (root / "variants/muzi_base/target.cpp").read_text(encoding="utf-8"),
    "bool MuziBaseSensorManager::begin() {", "\n#ifdef DISPLAY_CLASS\n")
gps_initialization = unique_section(
    (root / "src/helpers/sensors/EnvironmentSensorManager.cpp").read_text(encoding="utf-8"),
    "void EnvironmentSensorManager::initBasicGPS() {",
    "\n// gps code for rak might be moved to MicroNMEALoactionProvider")
gps_production = gps_declaration + "\n" + gps_methods + "\n" + gps_initialization

with tempfile.TemporaryDirectory(prefix="superbase-tests-") as output:
    common = [compiler, "-std=c++17", "-Wall", "-Wextra", "-Wno-unused-parameter",
              *(["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-g"] if args.sanitize else []),
              "-DMUZI_BASE_SUPERIO", "-Itest/superbase/mocks", "-Isrc", "-Ivariants/muzi_base"]
    (Path(output) / "gps-production.inc").write_text(gps_production, encoding="utf-8")
    for superio in (True, False):
        executable = Path(output) / ("gps-switch-" + str(superio) + ".exe")
        gps_flags = common if superio else [flag for flag in common if flag != "-DMUZI_BASE_SUPERIO"]
        subprocess.run([*gps_flags, "-I" + output, "test/superbase/gps_switch.cpp",
                        "-o", str(executable)], cwd=root, check=True)
        subprocess.run([str(executable)], check=True)
    (Path(output) / "patched-bonding.inc").write_text(patched_bonding)
    executable = Path(output) / "bonding-storage.exe"
    subprocess.run([*common, "-Itest/superbase/bonding_mocks", "-I" + output,
                    "test/superbase/bonding_storage.cpp", "-o", str(executable)], cwd=root, check=True)
    subprocess.run([str(executable)], check=True)
    executable = Path(output) / "superbase-tests.exe"
    subprocess.run([
        *common, "-DSUPERBASE_USB_DIAGNOSTICS", "-DPIN_BUZZER=22",
        "test/superbase/regression.cpp", "variants/muzi_base/SuperbaseButton.cpp",
        "src/helpers/ui/MomentaryButton.cpp", "src/helpers/radiolib/RadioLibWrappers.cpp",
        "src/helpers/ui/buzzer.cpp", "variants/muzi_base/SuperbaseDFU.cpp",
        "src/helpers/ArduinoSerialInterface.cpp", "-o", str(executable),
    ], cwd=root, check=True)
    subprocess.run([str(executable)], check=True)
    executable = Path(output) / "bonded-dfu.exe"
    subprocess.run([*common, "test/superbase/bonded_dfu.cpp", "variants/muzi_base/SuperbaseBondedDFU.cpp",
                    "-o", str(executable)], cwd=root, check=True)
    subprocess.run([str(executable)], check=True)
    executable = Path(output) / "emoji-tests.exe"
    subprocess.run([*common, "test/superbase/emoji.cpp", "src/helpers/ui/EmojiText.cpp",
                    "-o", str(executable)], cwd=root, check=True)
    subprocess.run([str(executable)], check=True)
    executable = Path(output) / "dfu-peer.exe"
    subprocess.run([*common, "test/superbase/dfu_peer.cpp", "-o", str(executable)], cwd=root, check=True)
    subprocess.run([str(executable)], check=True)
    for tinyusb in (False, True):
        executable = Path(output) / ("dfu-usb-" + str(tinyusb) + ".exe")
        subprocess.run([
            *common, "-DNRF52_PLATFORM", *(["-DUSE_TINYUSB"] if tinyusb else []),
            "test/superbase/dfu_usb.cpp", "variants/muzi_base/SuperbaseDfuUsb.cpp",
            "-o", str(executable),
        ], cwd=root, check=True)
        subprocess.run([str(executable)], check=True)
