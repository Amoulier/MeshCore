#!/usr/bin/env python3
from pathlib import Path

path = Path(__file__).with_name("apply_heltec_v4_energy_optimizations.py")
text = path.read_text(encoding="utf-8")
old = '''replace_once(
    "src/Dispatcher.cpp",
    "  radio_nonrx_start = _ms->getMillis();\\n",
    "  radio_nonrx_start = _ms->getMillis();\\n"
    "  floor_calib_started_at = radio_nonrx_start;\\n",
)
'''
new = '''replace_once(
    "src/Dispatcher.cpp",
    "  _err_flags = 0;\\n"
    "  radio_nonrx_start = _ms->getMillis();\\n",
    "  _err_flags = 0;\\n"
    "  radio_nonrx_start = _ms->getMillis();\\n"
    "  floor_calib_started_at = radio_nonrx_start;\\n",
)
'''
if old in text:
    text = text.replace(old, new, 1)
elif new not in text:
    raise RuntimeError("Dispatcher transform anchor block was not found")
path.write_text(text, encoding="utf-8")
print("Dispatcher transform anchor repaired")
