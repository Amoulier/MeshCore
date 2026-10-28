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
patch_dfu = runpy.run_path(str(root / "superbase-dfu.py"))["patch_bledfu"]
anchor = "peer_data->crc16 = crc16((uint8_t*) peer_data, offsetof(peer_data_t, crc16));"
fixture = ("static void bledfu_control_wr_authorize_cb() {\n"
           "      // Peer data information so that bootloader could re-connect after reboot\n" + anchor +
           "\n      bootloader_util_app_start(NRF_UICR->NRFFW[0]);\n}")
patched = patch_dfu(fixture)
assert patched.count("superbaseDfuAdaptPeerHandoff();") == 2
assert patched.count("superbaseDfuJumpToBootloader(") == 2
for incompatible in ("changed", fixture + anchor, fixture.replace("bootloader_util_app_start", "changed")):
    try:
        patch_dfu(incompatible)
    except RuntimeError:
        pass
    else:
        raise AssertionError("Changed upstream DFU must fail the build hook")
compiler = shutil.which("g++")
if not compiler:
    raise SystemExit("Install g++ to run the SuperBase regressions")
with tempfile.TemporaryDirectory(prefix="superbase-tests-") as output:
    common = [compiler, "-std=c++17", "-Wall", "-Wextra", "-Wno-unused-parameter",
              *(["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-g"] if args.sanitize else []),
              "-DMUZI_BASE_SUPERIO", "-Itest/superbase/mocks", "-Isrc", "-Ivariants/muzi_base"]
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
