"""Apply a scoped handoff hook to a build-local copy of pinned Bluefruit code."""
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


if "Import" in globals():
    Import("env")

    def scoped_dfu_hook(build_env, node):
        original = Path(node.srcnode().get_abspath())
        if original.name != "BLEDfu.cpp":
            return node
        patched = patch_bledfu(original.read_text())
        generated = Path(build_env.subst("$BUILD_DIR")) / "superbase-dfu" / "BLEDfu.cpp"
        generated.parent.mkdir(parents=True, exist_ok=True)
        if not generated.exists() or generated.read_text() != patched:
            generated.write_text(patched)
        # Compile only the workspace copy; never edit the shared framework package.
        return build_env.Object(node.get_abspath() + ".o", str(generated.resolve()))

    env.AddBuildMiddleware(scoped_dfu_hook, "*BLEDfu.cpp")
