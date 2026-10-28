#include <cassert>
#include <iostream>
#include "patched-bonding.inc"

static std::vector<uint8_t> prefix() {
  // Synthetic key/name fields deliberately differ from both CCCD test payloads.
  std::vector<uint8_t> value{80};
  for (unsigned i=0;i<80;++i) value.push_back(uint8_t(0x60+i));
  const std::vector<uint8_t> name{'T','e','s','t',0};
  value.push_back(static_cast<uint8_t>(name.size()));
  value.insert(value.end(),name.begin(),name.end());
  return value;
}

static void exercise(const std::vector<uint8_t>& old_value, const std::vector<uint8_t>& replacement,
                     bool expect_write, bool fail_seek=false, bool field_missing=false) {
  const auto protected_fields=prefix();
  mock_bond_file=protected_fields;
  if (!field_missing) {
    mock_bond_file.push_back(static_cast<uint8_t>(old_value.size()));
    mock_bond_file.insert(mock_bond_file.end(),old_value.begin(),old_value.end());
  }
  const auto before=mock_bond_file;
  mock_sys_attributes=replacement;
  mock_file_writes=mock_seek_calls=0;
  // Initial seek, skip key, skip name, then the corrected seek before writing.
  mock_fail_seek_call=fail_seek ? 4 : 0;
  ble_gap_addr_t address;
  assert(bond_save_cccd(1,1,&address));
  assert(std::equal(protected_fields.begin(),protected_fields.end(),mock_bond_file.begin()));
  if (fail_seek || !expect_write) {
    assert(mock_file_writes==0 && mock_bond_file==before);
    if (fail_seek) assert(mock_seek_calls==4);
  } else {
    assert(mock_file_writes==2);
    const size_t offset=protected_fields.size();
    assert(mock_bond_file[offset]==replacement.size());
    assert(std::equal(replacement.begin(),replacement.end(),mock_bond_file.begin()+offset+1));
    // Read the field by its encoded length, as bond_load_cccd does. Trailing bytes
    // after a shrink are irrelevant and must not move the field's length slot.
    const std::vector<uint8_t> loaded(mock_bond_file.begin()+offset+1,
                                    mock_bond_file.begin()+offset+1+mock_bond_file[offset]);
    assert(loaded==replacement);
  }
}

int main() {
  exercise({1,2},{3,4,5,6},true);             // Growth rewrites the old length slot.
  exercise({1,2,3,4},{9,8},true);             // Shrink with unused bytes still in file.
  exercise({1,2,3},{7,8,9},true);             // Equal length, changed data.
  exercise({1,2,3},{1,2,3},false);            // Identical value avoids flash writes.
  exercise({1,2},{3,4,5,6},true,true);        // Failed final seek must not write.
  exercise({1,2,3},{7,8,9},true,true);        // Same-length update obeys the same gate.
  exercise({}, {3,4},true,false,true);        // First CCCD field in a key/name-only file.
  exercise({}, {3,4},true,true,true);         // Failed append seek preserves key/name-only file.
  std::cout << "PASS: patched upstream CCCD storage growth/shrink, changed/identical values, protected synthetic key/name fields and failed-seek isolation\n";
}
