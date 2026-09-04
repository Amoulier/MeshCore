from pathlib import Path
import re


def replace_exact(path: str, old: str, new: str) -> None:
    file_path = Path(path)
    text = file_path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{path}: expected one exact match, found {count}")
    file_path.write_text(text.replace(old, new), encoding="utf-8")


def replace_regex(path: str, pattern: str, replacement: str) -> None:
    file_path = Path(path)
    text = file_path.read_text(encoding="utf-8")
    updated, count = re.subn(pattern, replacement, text, flags=re.MULTILINE | re.DOTALL)
    if count != 1:
        raise RuntimeError(f"{path}: expected one regex match, found {count}")
    file_path.write_text(updated, encoding="utf-8")


replace_exact(
    "variants/heltec_v4/platformio.ini",
    "  -D HELTEC_V4_ENABLE_DFS=1\n",
    "",
)
replace_exact(
    "variants/heltec_v4/platformio.ini",
    "  -D LORA_TX_POWER=22\n",
    "  -D LORA_TX_POWER=10\n",
)

replace_exact(
    "variants/heltec_v4/target.cpp",
    "    // LORA_TX_POWER is the desired antenna output. The board maps it to the\n"
    "    // safe SX1262 input required by the detected GC1109 or KCT8103L FEM.\n"
    "    radio_driver.setTxPower(LORA_TX_POWER);",
    "    // Preserve MeshCore's existing SX1262-input setting. The board applies a\n"
    "    // FEM-aware ceiling so estimated antenna output stays within 22 dBm.\n"
    "    radio_driver.setTxPower(LORA_TX_POWER);",
)

replace_exact(
    "variants/heltec_v4/HeltecV4Board.h",
    "  int8_t last_requested_output_dbm = 0;\n",
    "  int8_t last_requested_radio_dbm = 0;\n",
)
replace_exact(
    "variants/heltec_v4/HeltecV4Board.h",
    "  int8_t mapRadioTxPower(int8_t requested_output_dbm) override;\n",
    "  int8_t mapRadioTxPower(int8_t requested_radio_dbm) override;\n",
)

replace_regex(
    "variants/heltec_v4/HeltecV4Board.cpp",
    r"int8_t HeltecV4Board::mapRadioTxPower\(int8_t requested_output_dbm\)\n\{.*?\n\}\n\nconst char \*HeltecV4Board::getManufacturerName",
    """int8_t HeltecV4Board::mapRadioTxPower(int8_t requested_radio_dbm)
{
  if (requested_radio_dbm < -9) {
    requested_radio_dbm = -9;
  } else if (requested_radio_dbm > 22) {
    requested_radio_dbm = 22;
  }

  last_requested_radio_dbm = requested_radio_dbm;
  if (loRaFEMControl.getFEMType() == GC1109_PA) {
    last_radio_input_dbm = heltec_v4::clampGc1109RadioInput(
        requested_radio_dbm, HELTEC_V4_MAX_OUTPUT_POWER_DBM);
  } else if (loRaFEMControl.getFEMType() == KCT8103L_PA) {
    last_radio_input_dbm = heltec_v4::clampKct8103lRadioInput(
        requested_radio_dbm, HELTEC_V4_MAX_OUTPUT_POWER_DBM);
  } else {
    last_radio_input_dbm = requested_radio_dbm;
  }
  return last_radio_input_dbm;
}

const char *HeltecV4Board::getManufacturerName""",
)

replace_exact(
    "variants/heltec_v4/HeltecV4Board.cpp",
    "  if (strcmp(command, \"get radio.power\") == 0) {\n"
    "    sprintf(reply, \"> requested:%ddBm,radio:%ddBm\", last_requested_output_dbm, last_radio_input_dbm);\n"
    "    return true;\n"
    "  }",
    "  if (strcmp(command, \"get radio.power\") == 0) {\n"
    "    const int8_t estimated_output_dbm = loRaFEMControl.getFEMType() == GC1109_PA\n"
    "        ? heltec_v4::gc1109EstimatedOutput(last_radio_input_dbm)\n"
    "        : (loRaFEMControl.getFEMType() == KCT8103L_PA\n"
    "             ? heltec_v4::kct8103lEstimatedOutput(last_radio_input_dbm)\n"
    "             : last_radio_input_dbm);\n"
    "    sprintf(reply, \"> requested-radio:%ddBm,applied-radio:%ddBm,estimated-output:%ddBm\",\n"
    "            last_requested_radio_dbm, last_radio_input_dbm, estimated_output_dbm);\n"
    "    return true;\n"
    "  }",
)

replace_exact(
    "examples/companion_radio/ui-new/UITask.cpp",
    "    const int minMilliVolts = BATT_MIN_MILLIVOLTS;\n"
    "    const int maxMilliVolts = BATT_MAX_MILLIVOLTS;\n"
    "    int batteryPercentage = ((batteryMilliVolts - minMilliVolts) * 100) / (maxMilliVolts - minMilliVolts);\n"
    "    if (batteryPercentage < 0) batteryPercentage = 0; // Clamp to 0%\n"
    "    if (batteryPercentage > 100) batteryPercentage = 100; // Clamp to 100%",
    "    const int minMilliVolts = BATT_MIN_MILLIVOLTS;\n"
    "    const int maxMilliVolts = BATT_MAX_MILLIVOLTS;\n"
    "    int batteryPercentage = board.getBattPercent();\n"
    "    if (batteryPercentage < 0) {\n"
    "      batteryPercentage = ((batteryMilliVolts - minMilliVolts) * 100) / (maxMilliVolts - minMilliVolts);\n"
    "    }\n"
    "    if (batteryPercentage < 0) batteryPercentage = 0; // Clamp to 0%\n"
    "    if (batteryPercentage > 100) batteryPercentage = 100; // Clamp to 100%",
)

replace_exact(
    "src/helpers/CommonCLI.cpp",
    "#include <RTClib.h>\n",
    "#include <RTClib.h>\n#include <helpers/PersistentWriteGuard.h>\n",
)

replace_regex(
    "src/helpers/CommonCLI.cpp",
    r"void CommonCLI::loadPrefs\(FILESYSTEM\* fs\) \{.*?\n\}\n\nvoid CommonCLI::loadPrefsInt",
    """void CommonCLI::loadPrefs(FILESYSTEM* fs) {
  const char* prefs_path = fs->exists("/prefs.json")
      ? "/prefs.json"
      : (fs->exists("/prefs.json.bak") ? "/prefs.json.bak" : NULL);
  if (prefs_path) {
#if defined(RP2040_PLATFORM)
    File file = fs->open(prefs_path, "r");
#else
    File file = fs->open(prefs_path);
#endif
    if (file) {
      _prefs->loadSerial(file);
      file.close();
    }
  } else if (fs->exists("/com_prefs")) {
    loadPrefsInt(fs, "/com_prefs");
    savePrefs(fs);
  }
}

void CommonCLI::loadPrefsInt""",
)

replace_regex(
    "src/helpers/CommonCLI.cpp",
    r"bool CommonCLI::savePrefs\(FILESYSTEM\* fs\) \{.*?\n\}\n\n#define MIN_LOCAL_ADVERT_INTERVAL",
    """bool CommonCLI::savePrefs(FILESYSTEM* fs) {
  if (!meshcorePersistentWritesAllowed()) {
    return false;
  }

  const char* temporary_path = "/prefs.json.tmp";
  const char* backup_path = "/prefs.json.bak";
  fs->remove(temporary_path);
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  File file = fs->open(temporary_path, FILE_O_WRITE);
#elif defined(RP2040_PLATFORM)
  File file = fs->open(temporary_path, "w");
#else
  File file = fs->open(temporary_path, "w", true);
#endif
  if (!file) {
    return false;
  }

  const bool encoded = _prefs->saveSerial(file);
  file.flush();
  file.close();
  if (!encoded || !meshcorePersistentWritesAllowed()) {
    fs->remove(temporary_path);
    return false;
  }

  fs->remove(backup_path);
  const bool had_primary = fs->exists("/prefs.json");
  if (had_primary && !fs->rename("/prefs.json", backup_path)) {
    fs->remove(temporary_path);
    return false;
  }
  if (!fs->rename(temporary_path, "/prefs.json")) {
    if (had_primary) {
      fs->rename(backup_path, "/prefs.json");
    }
    fs->remove(temporary_path);
    return false;
  }
  return true;
}

#define MIN_LOCAL_ADVERT_INTERVAL""",
)

print("Heltec V4 final audit patch applied")
