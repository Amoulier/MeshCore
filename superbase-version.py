"""Supply local build metadata without overriding the release builder's macros."""
from datetime import datetime, timezone
import os

Import("env")

macros = env.ParseFlags(os.environ.get("PLATFORMIO_BUILD_FLAGS", "")).get("CPPDEFINES", [])
defined = {value[0] if isinstance(value, (tuple, list)) else value for value in macros}
version = "v1.18-sb2-rc1"
for name, value in (
    ("FIRMWARE_VERSION", version),
    ("FIRMWARE_BUILD_DATE", datetime.now(timezone.utc).strftime("%d-%b-%Y")),
):
    if name not in defined:
        env.Append(CPPDEFINES=[(name, env.StringifyMacro(value))])
