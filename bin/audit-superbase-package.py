"""Check SuperBase application-only UF2 and matching Nordic DFU payloads."""
import argparse
import binascii
import hashlib
import json
from pathlib import Path
import struct
import zipfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("uf2", type=Path)
parser.add_argument("dfu", type=Path)
parser.add_argument("--version")
parser.add_argument("--target", default="muzi_base_uno_superIO_companion_radio_ble")
parser.add_argument("--manufacturer", help="Require the product identity in the application")
parser.add_argument("--report", type=Path)
parser.add_argument("--elf", type=Path, help="Also verify RAM excludes the bootloader DFU handoff")
args = parser.parse_args()

data = args.uf2.read_bytes()
assert data and len(data) % 512 == 0, "Incomplete UF2 blocks"
application = bytearray()
for index, offset in enumerate(range(0, len(data), 512)):
    magic1, magic2, flags, address, size, number, count, family = struct.unpack_from("<8I", data, offset)
    assert (magic1, magic2) == (0x0A324655, 0x9E5D5157), "Invalid UF2 header"
    assert struct.unpack_from("<I", data, offset + 508)[0] == 0x0AB16F30, "Invalid UF2 trailer"
    assert flags & 0x2000 and family == 0xADA52840, "Wrong MCU family"
    assert not flags & 1, "Non-flash UF2 block"
    assert number == index and count == len(data) // 512 and size == 256, "Invalid UF2 sequence"
    assert address == 0x26000 + index * 256 and address + size <= 0xD3000, "Write outside application flash"
    application.extend(data[offset + 32:offset + 32 + size])

with zipfile.ZipFile(args.dfu) as package:
    assert package.testzip() is None, "Damaged DFU ZIP"
    manifest = json.loads(package.read("manifest.json"))["manifest"]
    assert set(manifest) <= {"application", "dfu_version"} and "application" in manifest, "Package contains non-application images"
    image = manifest["application"]
    binary = package.read(image["bin_file"])
    assert package.read(image["dat_file"]), "Missing DFU init packet"
    init = image["init_packet_data"]
    assert init["device_type"] == 0x52 and init["softdevice_req"] == [0xB6], "Wrong Nordic target or SoftDevice"
    assert init["firmware_crc16"] == binascii.crc_hqx(binary, 0xFFFF), "Invalid DFU CRC16"
assert application[:len(binary)] == binary and len(application) - len(binary) < 256, "DFU and UF2 payloads differ"
stack, entry = struct.unpack_from("<II", binary)
assert 0x20000000 <= stack <= 0x20040000, "Invalid application stack"
assert entry & 1 and 0x26000 <= entry - 1 < 0x26000 + len(binary), "Invalid application entry"
if args.version:
    assert args.version.encode() in binary, "Expected firmware version missing"
if args.manufacturer:
    assert args.manufacturer.encode() + b"\0" in binary, "Expected product identity missing"

ram_segments = []
if args.elf:
    elf = args.elf.read_bytes()
    assert elf[:7] == b"\x7fELF\x01\x01\x01", "Expected 32-bit little-endian ELF"
    header = struct.unpack_from("<HHIIIIIHHHHHH", elf, 16)
    assert header[1] == 40, "Expected ARM ELF"
    for index in range(header[9]):
        segment = struct.unpack_from("<IIIIIIII", elf, header[4] + index * header[8])
        kind, _, address, _, _, size, _, _ = segment
        if kind == 1 and size and 0x20000000 <= address < 0x20040000:
            assert address >= 0x20008000, "Application RAM overlaps bootloader DFU handoff"
            assert address + size <= 0x20040000, "Application RAM exceeds nRF52840"
            ram_segments.append({"start": hex(address), "end": hex(address + size)})
    assert ram_segments, "Missing application RAM segments"

report = {
    "status": "PASS",
    "target": args.target,
    "family": "0xADA52840",
    "application_start": "0x26000",
    "application_bytes": len(binary),
    "uf2_blocks": len(data) // 512,
    "matching_application_payloads": True,
    "hardware_tested": False,
    "sha256": {path.name: hashlib.sha256(path.read_bytes()).hexdigest() for path in (args.uf2, args.dfu)},
}
if args.elf:
    report["dfu_handoff_ram_excluded"] = True
    report["ram_segments"] = ram_segments
    report["sha256"][args.elf.name] = hashlib.sha256(args.elf.read_bytes()).hexdigest()
if args.manufacturer:
    report["manufacturer_checked"] = args.manufacturer
if args.report:
    args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
print(json.dumps(report, indent=2))
