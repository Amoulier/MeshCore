#!/usr/bin/env python3
from pathlib import Path

path = Path(__file__).with_name("apply_heltec_v4_energy_optimizations.py")
text = path.read_text(encoding="utf-8")

old_dispatcher = '''replace_once(
    "src/Dispatcher.cpp",
    "  radio_nonrx_start = _ms->getMillis();\\n",
    "  radio_nonrx_start = _ms->getMillis();\\n"
    "  floor_calib_started_at = radio_nonrx_start;\\n",
)
'''
new_dispatcher = '''replace_once(
    "src/Dispatcher.cpp",
    "  _err_flags = 0;\\n"
    "  radio_nonrx_start = _ms->getMillis();\\n",
    "  _err_flags = 0;\\n"
    "  radio_nonrx_start = _ms->getMillis();\\n"
    "  floor_calib_started_at = radio_nonrx_start;\\n",
)
'''
if old_dispatcher in text:
    text = text.replace(old_dispatcher, new_dispatcher, 1)
elif new_dispatcher not in text:
    raise RuntimeError("Dispatcher transform anchor block was not found")

old_room = '''        anchor = "#ifndef LORA_FREQ\\n"
        if anchor not in text:
            raise RuntimeError(f"{rel}: config anchor missing")
'''
new_room = '''        anchor = "#ifndef LORA_FREQ\\n"
        if anchor not in text and rel == "examples/simple_room_server/MyMesh.cpp":
            anchor = "#define REPLY_DELAY_MILLIS"
        if anchor not in text:
            raise RuntimeError(f"{rel}: config anchor missing")
'''
if old_room in text:
    text = text.replace(old_room, new_room, 1)
elif new_room not in text:
    raise RuntimeError("Room Server transform anchor block was not found")

path.write_text(text, encoding="utf-8")
print("Energy transform anchors repaired")
