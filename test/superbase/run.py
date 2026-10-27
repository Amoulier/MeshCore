"""Run SuperBase production helpers against deterministic hardware/storage failures."""
from pathlib import Path
import argparse
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--sanitize", action="store_true", help="Enable AddressSanitizer and UBSan (Linux)")
args = parser.parse_args()
compiler = shutil.which("g++")
if not compiler:
    raise SystemExit("Install g++ to run the SuperBase regressions")
with tempfile.TemporaryDirectory(prefix="superbase-tests-") as output:
    executable = Path(output) / "superbase-tests.exe"
    subprocess.run([
        compiler, "-std=c++17", "-Wall", "-Wextra", "-Wno-unused-parameter",
        *(["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-g"] if args.sanitize else []),
        "-DMUZI_BASE_SUPERIO", "-DPIN_BUZZER=22", "-Itest/superbase/mocks", "-Isrc", "-Ivariants/muzi_base",
        "test/superbase/regression.cpp", "variants/muzi_base/SuperbaseButton.cpp",
        "src/helpers/ui/MomentaryButton.cpp", "src/helpers/radiolib/RadioLibWrappers.cpp",
        "src/helpers/ui/buzzer.cpp",
        "variants/muzi_base/SuperbaseDFU.cpp",
        "-o", str(executable),
    ], cwd=root, check=True)
    subprocess.run([str(executable)], check=True)
