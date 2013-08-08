// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_storage.h"

#include <mntent.h>
#include <stdlib.h>
#include <sys/statvfs.h>

#include "common/picojson.h"
#include "system_info/system_info_utils.h"

namespace {

const char* sMountTable = "/proc/mounts";

}  // namespace

SysInfoStorage::SysInfoStorage(ContextAPI* api)
    : stopping_(false) {
  api_ = api;
  udev_ = udev_new();
  units_ = picojson::value(picojson::array(0));
}

SysInfoStorage::~SysInfoStorage() {
  if (udev_)
    udev_unref(udev_);
}

void SysInfoStorage::Get(picojson::value& error,
                         picojson::value& data) {
  if (!Update(error)) {
    return;
  }

  system_info::SetPicoJsonObjectValue(data, "units", units_);
  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

bool SysInfoStorage::Update(picojson::value& error) {
  picojson::array& units_arr = units_.get<picojson::array>();
  units_arr.clear();

  struct mntent *entry;
  FILE *aFile;

  aFile = setmntent(sMountTable, "r");
  if (!aFile) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Read mount table failed."));
    return false;
  }

  while (entry = getmntent(aFile)) {
    if (entry->mnt_fsname[0] == '/') {
      picojson::value unit = picojson::value(picojson::object());
      GetDetails(entry->mnt_fsname, entry->mnt_dir, error, unit);
      if (!error.get("message").to_str().empty()) {
        endmntent(aFile);
        return false;
      }
      units_arr.push_back(unit);
    }
  }

  endmntent(aFile);
  return true;
}

std::string
SysInfoStorage::GetDevPathFromMountPath(const std::string& mnt_path) {
  struct udev_enumerate *enumerate;
  struct udev_list_entry *devices, *dev_list_entry;

  if (mnt_path.empty() || mnt_path[0] != '/' || mnt_path.size() <=1) {
    return "";
  }

  enumerate = udev_enumerate_new(udev_);
  udev_enumerate_add_match_subsystem(enumerate, "block");
  udev_enumerate_scan_devices(enumerate);
  devices = udev_enumerate_get_list_entry(enumerate);

  udev_list_entry_foreach(dev_list_entry, devices) {
    struct udev_device *dev;
    const char *path;
    std::string dev_path, str;

    path = udev_list_entry_get_name(dev_list_entry);
    dev = udev_device_new_from_syspath(udev_, path);

    dev_path = system_info::GetUdevProperty(dev, "DEVPATH");
    if (dev_path.empty()) {
      udev_device_unref(dev);
      udev_enumerate_unref(enumerate);
      return "";
    }

    dev_path = "/sys" + dev_path;

    str = system_info::GetUdevProperty(dev, "DEVNAME");
    if (!str.empty() && (str == mnt_path)) {
      udev_device_unref(dev);
      udev_enumerate_unref(enumerate);
      return dev_path;
    }

    str = system_info::GetUdevProperty(dev, "DEVLINKS");
    if (!str.empty() && (std::string::npos != str.find(mnt_path))) {
      udev_device_unref(dev);
      udev_enumerate_unref(enumerate);
      return dev_path;
    }
  }

  udev_enumerate_unref(enumerate);
  return "";
}

void SysInfoStorage::GetDetails(const std::string& mnt_fsname,
                                const std::string& mnt_dir,
                                picojson::value& error,
                                picojson::value& unit) {
  struct udev_device* dev;
  struct udev_list_entry *attr_list_entry;
  struct udev_list_entry *attr_entry;
  const char* str;
  bool is_removable;

  std::string dev_path = GetDevPathFromMountPath(mnt_fsname);
  if (dev_path.empty()) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get storage DEVPATH failed."));
    return;
  }

  dev = udev_device_new_from_syspath(udev_, dev_path.c_str());
  if (!dev) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get storage udev from device_id failed."));
  }

  attr_list_entry = udev_device_get_properties_list_entry(dev);

  // FIXME(guanxian): may need to fix problems of encrypted or LVM storages
  struct udev_device* parent_dev =
    udev_device_get_parent_with_subsystem_devtype(dev, "block", "disk");
  str = udev_device_get_sysattr_value(parent_dev, "removable");
  if (!str) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get storage attribute removable failed."));
    udev_device_unref(parent_dev);
    udev_device_unref(dev);
    return;
  }

  is_removable = (1 == atoi(str));
  system_info::SetPicoJsonObjectValue(unit, "isRemovable",
      picojson::value(is_removable));
  // deprecated, same as isRemovable
  system_info::SetPicoJsonObjectValue(unit, "isRemoveable",
      picojson::value(is_removable));

  system_info::SetPicoJsonObjectValue(unit, "type", picojson::value("UNKNOWN"));
  if (!is_removable) {
    system_info::SetPicoJsonObjectValue(unit, "type",
        picojson::value("INTERNAL"));
  } else {
    attr_entry = udev_list_entry_get_by_name(attr_list_entry, "ID_BUS");
    if (attr_entry) {
      str = udev_list_entry_get_value(attr_entry);
      if (str && (strcmp(str, "usb") == 0)) {
        system_info::SetPicoJsonObjectValue(unit, "type",
            picojson::value("USB_HOST"));
      }
    }
    // FIXME(halton): Add MMC type support, we do not find a correct
    // attribute to identify.
  }

  str = udev_device_get_sysattr_value(dev, "size");
  if (!str) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get storage attribute size failed."));
    udev_device_unref(dev);
    return;
  }
  system_info::SetPicoJsonObjectValue(unit, "capacity",
      picojson::value(static_cast<double>(atoll(str)*512)));

  struct statvfs buf;
  int ret = statvfs(mnt_dir.c_str(), &buf);
  if (-1 == ret) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get storage availableCapacity failed."));
    udev_device_unref(dev);
    return;
  }

  udev_device_unref(dev);
  system_info::SetPicoJsonObjectValue(unit, "availableCapacity",
      picojson::value(static_cast<double>(buf.f_bavail * buf.f_bsize)));

  // set message to confirm no errors
  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

gboolean SysInfoStorage::OnUpdateTimeout(gpointer user_data) {
  SysInfoStorage* instance = static_cast<SysInfoStorage*>(user_data);

  if (instance->stopping_) {
    instance->stopping_ = false;
    return FALSE;
  }

  // can't to take a reference (&), just copy
  picojson::array old_units_arr = instance->units_.get<picojson::array>();
  picojson::value error = picojson::value(picojson::object());;
  instance->Update(error);

  bool is_changed = false;
  picojson::array& units_arr = instance->units_.get<picojson::array>();
  if (old_units_arr.size() != units_arr.size()) {
    is_changed = true;
  } else {
    for (int i = 0; i < units_arr.size(); i++) {
      if (old_units_arr[i] != units_arr[i]) {
        is_changed = true;
        break;
      }
    }
  }

  if (is_changed) {
    picojson::value output = picojson::value(picojson::object());;
    picojson::value data = picojson::value(picojson::object());

    system_info::SetPicoJsonObjectValue(data, "units", instance->units_);
    system_info::SetPicoJsonObjectValue(output, "cmd",
        picojson::value("SystemInfoPropertyValueChanged"));
    system_info::SetPicoJsonObjectValue(output, "prop",
        picojson::value("STORAGE"));
    system_info::SetPicoJsonObjectValue(output, "data", data);

    std::string result = output.serialize();
    instance->api_->PostMessage(result.c_str());
  }

  return TRUE;
}
