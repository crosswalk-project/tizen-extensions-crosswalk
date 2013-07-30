// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_storage.h"

#include <stdlib.h>
#include <sys/statvfs.h>
#include <mntent.h>

#include "common/picojson.h"
#include "system_info/system_info_utils.h"

namespace {

const char* sMountTable = "/proc/mounts";

}  // namespace

using namespace system_info;

SysInfoStorage::SysInfoStorage(picojson::value& error) {
  udev_ = udev_new();
  if (!udev_) {
    SetPicoJsonObjectValue(error, "message",
        picojson::value("Can't create udev."));
  }
}

SysInfoStorage::~SysInfoStorage() {
  if(udev_)
    udev_unref(udev_);
}

void SysInfoStorage::Update(picojson::value& error,
                            picojson::value& data) {
  struct mntent *entry;
  FILE *aFile;

  picojson::value units = picojson::value(picojson::array(0));
  picojson::array& units_arr = units.get<picojson::array>();

  aFile = setmntent(sMountTable, "r");
  if (!aFile) {
    SetPicoJsonObjectValue(error, "message",
        picojson::value("Read mount table failed."));
    return;
  }

  while (entry = getmntent(aFile)) {
    if(entry->mnt_fsname[0] == '/') {
      picojson::value unit = picojson::value(picojson::object());
      GetDetails(entry->mnt_fsname, entry->mnt_dir, error, unit);
      if (!error.get("message").to_str().empty()) {
        endmntent(aFile);
        return;
      }
      units_arr.push_back(unit);
    }
  }

  endmntent(aFile);
  SetPicoJsonObjectValue(data, "units", units);
  SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

std::string
SysInfoStorage::GetDevPathFromMountPath(const std::string& mnt_path) {
  struct udev_enumerate *enumerate;
  struct udev_list_entry *devices, *dev_list_entry;

  if(mnt_path.empty() || mnt_path[0] != '/' || mnt_path.size() <=1) {
    return NULL;
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

    dev_path = get_udev_property(dev, "DEVPATH");
    if (dev_path.empty()) {
      udev_device_unref(dev);
      udev_enumerate_unref(enumerate);
      return NULL;
    }

    dev_path = "/sys" + dev_path;

    str = get_udev_property(dev, "DEVNAME");
    if (!str.empty() && (str == mnt_path)) {
      udev_device_unref(dev);
      udev_enumerate_unref(enumerate);
      return dev_path;
    }

    str = get_udev_property(dev, "DEVLINKS");
    if (!str.empty() && (std::string::npos != str.find(mnt_path))) {
      udev_device_unref(dev);
      udev_enumerate_unref(enumerate);
      return dev_path;
    }
  }

  udev_enumerate_unref(enumerate);
  return NULL;
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
    SetPicoJsonObjectValue(error, "message",
        picojson::value("Get storage DEVPATH failed."));
    return;
  }

  dev = udev_device_new_from_syspath(udev_, (char *)dev_path.c_str());
  if (!dev) {
    SetPicoJsonObjectValue(error, "message",
        picojson::value("Get storage udev from device_id failed."));
  }

  attr_list_entry = udev_device_get_properties_list_entry(dev);

  struct udev_device* parent_dev;
  parent_dev = udev_device_get_parent_with_subsystem_devtype(dev, "block", "disk");
  str = udev_device_get_sysattr_value(parent_dev, "removable");
  if (!str) {
    SetPicoJsonObjectValue(error, "message",
        picojson::value("Get storage attribute removable failed."));
    udev_device_unref(parent_dev);
    udev_device_unref(dev);
    return;
  }

  is_removable = (1 == atoi(str));
  SetPicoJsonObjectValue(unit, "isRemovable", picojson::value(is_removable));
  // deprecated, same as isRemovable
  SetPicoJsonObjectValue(unit, "isRemoveable", picojson::value(is_removable));

  SetPicoJsonObjectValue(unit, "type", picojson::value("UNKNOWN"));
  if (!is_removable) {
    SetPicoJsonObjectValue(unit, "type", picojson::value("INTERNAL"));
  } else {
    attr_entry = udev_list_entry_get_by_name(attr_list_entry, "ID_BUS");
    if (attr_entry) {
      str = udev_list_entry_get_value(attr_entry);
      if (str && (strcmp(str, "usb") == 0)) {
        SetPicoJsonObjectValue(unit, "type", picojson::value("USB_HOST"));
      }
    }
    // FIXME(halton): Add MMC type support, we do not find a correct
    // attribute to identify.
  }

  str = udev_device_get_sysattr_value(dev, "size");
  if (!str) {
    SetPicoJsonObjectValue(error, "message",
        picojson::value("Get storage attribute size failed."));
    udev_device_unref(dev);
    return;
  }
  SetPicoJsonObjectValue(unit, "capacity", 
      picojson::value(static_cast<double>(atoll(str)*512)));

  struct statvfs buf;
  int ret = statvfs(mnt_dir.c_str(), &buf);
  if (-1 == ret) {
    SetPicoJsonObjectValue(error, "message",
        picojson::value("Get storage availableCapacity failed."));
    udev_device_unref(dev);
    return;
  }

  udev_device_unref(dev);
  SetPicoJsonObjectValue(unit, "availableCapacity", 
      picojson::value(static_cast<double>(buf.f_bavail * buf.f_bsize)));
}
