"""Apply a scoped handoff hook to a build-local copy of pinned Bluefruit code."""
from pathlib import Path


def patch_bledfu(source):
    anchor = "peer_data->crc16 = crc16((uint8_t*) peer_data, offsetof(peer_data_t, crc16));"
    if source.count(anchor) != 1:
        raise RuntimeError("Bluefruit DFU changed: review the SuperBase handoff hook before building")
    return source.replace(anchor, anchor + "\n      extern void superbaseDfuAdaptPeerHandoff();"
                          "\n      superbaseDfuAdaptPeerHandoff();")


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
