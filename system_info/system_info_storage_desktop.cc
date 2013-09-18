// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_storage.h"

#include <mntent.h>
#include <stdlib.h>
#include <sys/statvfs.h>

#include "common/picojson.h"

namespace {

const char* sMountTable = "/proc/mounts";

}  // namespace

SysInfoStorage::SysInfoStorage()
    : timeout_cb_id_(0) {
  udev_ = udev_new();
  units_ = picojson::value(picojson::array(0));
  pthread_mutex_init(&events_list_mutex_, NULL);
}

SysInfoStorage::~SysInfoStorage() {
  if (udev_)
    udev_unref(udev_);
  pthread_mutex_destroy(&events_list_mutex_);
}

bool SysInfoStorage::Update(picojson::value& error) {
  picojson::array& units_arr = units_.get<picojson::array>();
  units_arr.clear();

  FILE *aFile;
  aFile = setmntent(sMountTable, "r");
  if (!aFile) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Read mount table failed."));
    return false;
  }

  struct mntent *entry;
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
    const char *path;
    struct udev_device *dev;
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
  struct udev_list_entry *attr_entry;
  struct udev_list_entry *attr_list_entry;

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

  // FIXME(guanxian): May need to fix problems of encrypted or LVM storages.
  struct udev_device* parent_dev =
    udev_device_get_parent_with_subsystem_devtype(dev, "block", "disk");

  const char* str;
  str = udev_device_get_sysattr_value(parent_dev, "removable");
  if (!str) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get storage attribute removable failed."));
    udev_device_unref(parent_dev);
    udev_device_unref(dev);
    return;
  }

  bool is_removable;
  is_removable = (1 == atoi(str));
  system_info::SetPicoJsonObjectValue(unit, "isRemovable",
      picojson::value(is_removable));
  // Deprecated, same as isRemovable.
  system_info::SetPicoJsonObjectValue(unit, "isRemoveable",
      picojson::value(is_removable));

  system_info::SetPicoJsonObjectValue(unit, "type",
      picojson::value("UNKNOWN"));

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

  // Set message to confirm no errors.
  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}
