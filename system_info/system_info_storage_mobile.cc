// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_storage.h"

#include <sys/statfs.h>
#include <vconf.h>

#include "common/picojson.h"

namespace {

const char* sStorageInternalPath = "/opt/usr/media";
const char* sStorageSDCardPath = "/opt/storage/sdcard";

}  // namespace

SysInfoStorage::SysInfoStorage()
    : timeout_cb_id_(0) {
  units_ = picojson::value(picojson::array(0));
  pthread_mutex_init(&events_list_mutex_, NULL);
}

SysInfoStorage::~SysInfoStorage() {
  if (timeout_cb_id_ > 0)
    g_source_remove(timeout_cb_id_);
  pthread_mutex_destroy(&events_list_mutex_);
}

bool SysInfoStorage::Update(picojson::value& error) {
  picojson::array& units_arr = units_.get<picojson::array>();
  units_arr.clear();

  picojson::value unit_intern = picojson::value(picojson::object());
  if (GetInternal(error, unit_intern))
    units_arr.push_back(unit_intern);

  picojson::value unit_mmc = picojson::value(picojson::object());
  if (GetMMC(error, unit_mmc))
    units_arr.push_back(unit_mmc);

  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
  return true;
}

bool SysInfoStorage::GetInternal(picojson::value& error,
                                 picojson::value& unit) {
  struct statfs fs;
  if (statfs(sStorageInternalPath, &fs) < 0) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Internal Storage path Error"));
    return false;
  }

  double available_capacity = static_cast<double>(fs.f_bsize) *
                       static_cast<double>(fs.f_bavail);
  double capacity = static_cast<double>(fs.f_bsize) *
             static_cast<double>(fs.f_blocks);

  system_info::SetPicoJsonObjectValue(unit, "type",
      picojson::value("INTERNAL"));
  system_info::SetPicoJsonObjectValue(unit, "capacity",
      picojson::value(capacity));
  system_info::SetPicoJsonObjectValue(unit, "availableCapacity",
      picojson::value(available_capacity));
  // FIXME(guanxian): Internal storage unit is not removale.
  system_info::SetPicoJsonObjectValue(unit, "isRemoveable",
      picojson::value(false));
  system_info::SetPicoJsonObjectValue(unit, "isRemovable",
      picojson::value(false));

  return true;
}

bool SysInfoStorage::GetMMC(picojson::value& error, picojson::value& unit) {
  int sdcard_state;
  if ((vconf_get_int(VCONFKEY_SYSMAN_MMC_STATUS, &sdcard_state) != 0) ||
      (sdcard_state != VCONFKEY_SYSMAN_MMC_MOUNTED)) {
    return false;
  }

  struct statfs fs;
  if (statfs(sStorageInternalPath, &fs) < 0) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("MMC mount path error"));
    return false;
  }

  statfs(sStorageSDCardPath, &fs);
  double available_capacity = static_cast<double>(fs.f_bsize) *
                              static_cast<double>(fs.f_bavail);
  double capacity = static_cast<double>(fs.f_bsize) *
                    static_cast<double>(fs.f_blocks);

  system_info::SetPicoJsonObjectValue(unit, "type",
      picojson::value("MMC"));
  system_info::SetPicoJsonObjectValue(unit, "capacity",
      picojson::value(capacity));
  system_info::SetPicoJsonObjectValue(unit, "availableCapacity",
      picojson::value(available_capacity));
  // FIXME(guanxian): MMC Internal storage unit is not removale.
  system_info::SetPicoJsonObjectValue(unit, "isRemoveable",
      picojson::value(true));
  system_info::SetPicoJsonObjectValue(unit, "isRemovable",
      picojson::value(true));

  return true;
}
