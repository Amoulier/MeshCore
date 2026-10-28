"""Apply scoped SuperBase fixes to build-local copies of pinned Bluefruit code."""
from pathlib import Path


def patch_bledfu(source):
    anchor = "peer_data->crc16 = crc16((uint8_t*) peer_data, offsetof(peer_data_t, crc16));"
    start = "      // Peer data information so that bootloader could re-connect after reboot"
    end = "      bootloader_util_app_start(NRF_UICR->NRFFW[0]);"
    callback = "static void bledfu_control_wr_authorize_cb("
    success = "reply.params.write.gatt_status = BLE_GATT_STATUS_SUCCESS;"
    if any(source.count(marker) != 1 for marker in (anchor, start, end, callback, success)):
        raise RuntimeError("Bluefruit DFU changed: review the SuperBase handoff hook before building")
    source = source.replace(anchor, anchor + "\n      extern void superbaseDfuAdaptPeerHandoff();"
                            "\n      superbaseDfuAdaptPeerHandoff();")
    source = source.replace(success, success + "\n    reply.params.write.update = 1;"
                            "\n    reply.params.write.offset = request->offset;"
                            "\n    reply.params.write.len = request->len;"
                            "\n    reply.params.write.p_data = request->data;")
    first, last = source.index(start), source.index(end) + len(end)
    if first >= last or first < source.index(callback):
        raise RuntimeError("Unexpected Bluefruit DFU handoff layout")
    body = source[first:last]
    source = source[:first] + "      superbaseDfuJumpToBootloader(conn_hdl);" + source[last:]
    helper = ("void superbaseDfuJumpToBootloader(uint16_t conn_hdl) {\n"
              "  BLEConnection* conn = Bluefruit.Connection(conn_hdl);\n"
              "  if (!conn) return;\n" + body + "\n}\n\n")
    return source.replace(callback, helper + callback)


def patch_bonding(source):
    start = "static void bond_save_cccd_dfr ("
    end = "\nbool bond_save_cccd ("
    include = '#include "bonding.h"'
    if any(source.count(marker) != 1 for marker in (start, end, include)):
        raise RuntimeError("Bluefruit bonding changed: review the SuperBase CCCD update before building")
    first, last = source.index(start), source.index(end)
    if first >= last:
        raise RuntimeError("Unexpected Bluefruit CCCD callback layout")
    body = source[first:last]
    fields = "  bdata_skip_field(&file); // skip key\n  bdata_skip_field(&file); // skip name"
    compare = "  if ( len == ((uint16_t) file.read()) )"
    write = "  if (do_write)\n  {\n    bdata_write(&file, sys_attr, len);"
    old_seek = ("    else\n    {\n"
                "      // restore the position to the state before reading old_data for writing\n"
                "      file.seek(file.size() - (len + 1));\n    }\n")
    if any(body.count(marker) != 1 for marker in (fields, compare, write, old_seek)):
        raise RuntimeError("Bluefruit CCCD storage changed: review the SuperBase offset fix before building")
    if not body.index(fields) < body.index(compare) < body.index(write):
        raise RuntimeError("Unexpected Bluefruit CCCD field/write order")
    body = body.replace(fields, fields + "\n  const uint32_t cccd_offset = file.position();")
    body = body.replace(old_seek, "")
    body = body.replace(write, "  if (do_write)\n  {\n"
                        "    // The length byte belongs at the start of this field even when its size changes.\n"
                        "    VERIFY(file.seek(cccd_offset), );\n"
                        "    bdata_write(&file, sys_attr, len);")
    patched = source[:first] + body + source[last:]
    # The generated .cpp is outside utility/, while Bluefruit's src/ is on the include path.
    return patched.replace(include, '#include "utility/bonding.h"')


if "Import" in globals():
    Import("env")

    def scoped_dfu_hook(build_env, node):
        original = Path(node.srcnode().get_abspath())
        patchers = {"BLEDfu.cpp": patch_bledfu, "bonding.cpp": patch_bonding}
        if original.name not in patchers:
            return node
        patched = patchers[original.name](original.read_text())
        generated = Path(build_env.subst("$BUILD_DIR")) / "superbase-dfu" / original.name
        generated.parent.mkdir(parents=True, exist_ok=True)
        if not generated.exists() or generated.read_text() != patched:
            generated.write_text(patched)
        # Compile only the workspace copy; never edit the shared framework package.
        return build_env.Object(node.get_abspath() + ".o", str(generated.resolve()))

    env.AddBuildMiddleware(scoped_dfu_hook, "*BLEDfu.cpp")
    env.AddBuildMiddleware(scoped_dfu_hook, "*bonding.cpp")
