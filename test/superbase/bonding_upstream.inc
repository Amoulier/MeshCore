/**************************************************************************/
/*!
    @file     bonding.cpp
    @author   hathach (tinyusb.org)

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2018, Adafruit Industries (adafruit.com)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/**************************************************************************/

// Upstream fixture: framework-arduinoadafruitnrf52 1.10701.0,
// meshcore-dev/Adafruit_nRF52_Arduino, Bluefruit52Lib/src/utility/bonding.cpp.
// Original complete source SHA256:
// 220cc726b170cef2eac6f09825574a7c193badaffd0eb15edf0ea64a9def83eb.
// The helpers, CCCD storage callback and wrapper below are copied unchanged.
// Native tests replace only the dependencies, then apply the production patch.
#include "bonding.h"

static bool bdata_skip_field(Adafruit_LittleFS_Namespace::File* file)
{
  int len = file->read();
  VERIFY(len > 0);

  file->seek(len + file->position());
  return true;
}

static void bdata_write(Adafruit_LittleFS_Namespace::File* file, void const* buffer, uint16_t bufsize)
{
  file->write( (uint8_t) bufsize );
  file->write( (uint8_t const*) buffer, bufsize);
}

static void bond_save_cccd_dfr (uint8_t role, uint16_t conn_hdl, ble_gap_addr_t const* id_addr)
{
  uint16_t len=0;
  sd_ble_gatts_sys_attr_get(conn_hdl, NULL, &len, SVC_CONTEXT_FLAG);
  VERIFY(len, );

  uint8_t sys_attr[len];
  VERIFY_STATUS(sd_ble_gatts_sys_attr_get(conn_hdl, sys_attr, &len, SVC_CONTEXT_FLAG),);

  char filename[BOND_FNAME_LEN];
  get_fname(filename, role, id_addr->addr);

  Adafruit_LittleFS_Namespace::File file(filename, FO_WRITE, InternalFS);
  VERIFY(file,);

  file.seek(0); // write mode start at the end, seek to beginning
  bdata_skip_field(&file); // skip key
  bdata_skip_field(&file); // skip name

  // only write if there is any data changes
  bool do_write = true;

  if ( len == ((uint16_t) file.read()) )
  {
    uint8_t old_data[len];
    file.read(old_data, len);

    if ( 0 == memcmp(sys_attr, old_data, len) )
    {
      do_write = false;
      BOND_LOG("CCCD matches file %s contents, no need to write", filename);
    }
    else
    {
      // restore the position to the state before reading old_data for writing
      file.seek(file.size() - (len + 1));
    }
  }

  if (do_write)
  {
    bdata_write(&file, sys_attr, len);
    BOND_LOG("Saved CCCD to file %s ( offset = %ld, len = %d bytes )", filename, file.size() - (len + 1), len);
  }

  file.close();
}

bool bond_save_cccd (uint8_t role, uint16_t conn_hdl, ble_gap_addr_t const* id_addr)
{
  // queue to execute in Ada Callback thread
  return ada_callback(id_addr, sizeof(ble_gap_addr_t), bond_save_cccd_dfr, role, conn_hdl, id_addr);
}

