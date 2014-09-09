// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_storage.h"

#include "common/picojson.h"

const std::string SysInfoStorage::name_ = "STORAGE";

SysInfoStorage::SysInfoStorage()
    : enumerate_(NULL),
      udev_(udev_new()),
      udev_monitor_(NULL),
      udev_monitor_fd_(-1),
      timeout_cb_id_(0) {
  units_ = picojson::value(picojson::array(0));
  InitStorageMonitor();
  QueryAllAvailableStorageUnits();
}

SysInfoStorage::~SysInfoStorage() {
  if (enumerate_)
    udev_enumerate_unref(enumerate_);
  if (udev_monitor_)
    udev_monitor_unref(udev_monitor_);
  if (udev_)
    udev_unref(udev_);
  if (timeout_cb_id_ > 0) {
    g_source_remove(timeout_cb_id_);
    timeout_cb_id_ = 0;
  }
}

void SysInfoStorage::Get(picojson::value& error,
                         picojson::value& data) {
  GetAllAvailableStorageDevices();
  system_info::SetPicoJsonObjectValue(data, "units", units_);
  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

void SysInfoStorage::GetAllAvailableStorageDevices() {
  picojson::array& units_arr = units_.get<picojson::array>();
  units_arr.clear();

  std::map<int, SysInfoDeviceStorageUnit>::const_iterator it;
  for (it = storages_.begin(); it != storages_.end(); ++it) {
    picojson::value unit = picojson::value(picojson::object());
    system_info::SetPicoJsonObjectValue(unit, "type",
        picojson::value(ToStorageUnitTypeString(it->second.type)));
    system_info::SetPicoJsonObjectValue(unit, "capacity",
        picojson::value(it->second.capacity));
    system_info::SetPicoJsonObjectValue(unit, "availableCapacity",
        picojson::value(it->second.available_capacity));
    system_info::SetPicoJsonObjectValue(unit, "isRemovable",
        picojson::value(it->second.is_removable));
    // Attribute 'isRemoveable' is deprecated. A typographic error.
    system_info::SetPicoJsonObjectValue(unit, "isRemoveable",
        picojson::value(it->second.is_removable));
    units_arr.push_back(unit);
  }
}

void SysInfoStorage::InitStorageMonitor() {
  if (!udev_) {
    std::cout << "Failed to create udev \n";
    return;
  }
  udev_monitor_ = udev_monitor_new_from_netlink(udev_, "udev");
  if (!udev_monitor_) {
    std::cout << "Failed to create udev monitor \n";
    return;
  }
  udev_monitor_filter_add_match_subsystem_devtype(udev_monitor_,
                                                  "block", NULL);
  udev_monitor_enable_receiving(udev_monitor_);
  udev_monitor_fd_ = udev_monitor_get_fd(udev_monitor_);

  enumerate_ = udev_enumerate_new(udev_);
  if (!enumerate_) {
    std::cout << "Failed to create udev enumerate \n";
    return;
  }
  udev_enumerate_add_match_subsystem(enumerate_, "block");
}

void SysInfoStorage::QueryAllAvailableStorageUnits() {
  storages_.clear();
  udev_list_entry* dev_list_entry;
  udev_device* dev;
  udev_enumerate_scan_devices(enumerate_);
  udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate_);
  udev_list_entry_foreach(dev_list_entry, devices) {
    const char* path = udev_list_entry_get_name(dev_list_entry);
    dev = udev_device_new_from_syspath(udev_, path);
    const char* type = udev_device_get_devtype(dev);
    // Here, type may be 'disk' or 'partition'. We neend to filter 'partition'.
    // For example, /dev/sda is disk. /dev/sda1 is partition.
    if (strcmp(type, "disk"))
      continue;
    SysInfoDeviceStorageUnit unit;
    MakeStorageUnit(unit, dev);
    storages_[unit.id] = unit;
    udev_device_unref(dev);
  }
}

void SysInfoStorage::MakeStorageUnit(SysInfoDeviceStorageUnit& unit,
                                     udev_device* dev) const {
  unit.id = udev_device_get_devnum(dev);
  unit.capacity = std::stof(udev_device_get_sysattr_value(dev, "size")) * 512;
  if (std::stoi(udev_device_get_sysattr_value(dev, "removable"))) {
    unit.type = USB_HOST;
    unit.is_removable = true;
  } else {
    unit.type = INTERNAL;
    unit.is_removable = false;
  }

  // TODO(qjia7): Find which attri reflects available capacity in udev.
  unit.available_capacity = 0.0;
}

std::string SysInfoStorage::ToStorageUnitTypeString(StorageUnitType type) {
  switch (type) {
    case INTERNAL:
      return "INTERNAL";
    case USB_HOST:
      return "USB_HOST";
    case MMC:
      return "MMC";
    case UNKNOWN:
    default:
      return "UNKNOWN";
  }
}

void SysInfoStorage::UpdateStorageList() {
  fd_set fds;
  timeval tv;

  FD_ZERO(&fds);
  FD_SET(udev_monitor_fd_, &fds);
  tv.tv_sec = 0;
  tv.tv_usec = 0;

  // select() will only operate on a single file descriptor, which is related
  // to the udev_monitor. The timeval object is set to 0 for not blocking
  // select().
  int ret = select(udev_monitor_fd_ + 1, &fds, NULL, NULL, &tv);
  if (ret > 0 && FD_ISSET(udev_monitor_fd_, &fds)) {
    udev_device* dev = udev_monitor_receive_device(udev_monitor_);
    if (!dev)
      return;

    int dev_id = udev_device_get_devnum(dev);
    std::string action = udev_device_get_action(dev);
    if (action == "add") {
      SysInfoDeviceStorageUnit unit;
      MakeStorageUnit(unit, dev);
      storages_[unit.id] = unit;
    } else if (action == "remove") {
      storages_.erase(dev_id);
    }
    udev_device_unref(dev);
  }
}

gboolean SysInfoStorage::OnUpdateTimeout(gpointer user_data) {
  SysInfoStorage* instance = static_cast<SysInfoStorage*>(user_data);
  int old_storage_count = instance->storages_.size();
  instance->UpdateStorageList();
  if (instance->storages_.size() == old_storage_count)
    return TRUE;

  instance->GetAllAvailableStorageDevices();
  picojson::value output = picojson::value(picojson::object());
  picojson::value data = picojson::value(picojson::object());

  system_info::SetPicoJsonObjectValue(data, "units", instance->units_);
  system_info::SetPicoJsonObjectValue(output, "cmd",
      picojson::value("SystemInfoPropertyValueChanged"));
  system_info::SetPicoJsonObjectValue(output, "prop",
      picojson::value("STORAGE"));
  system_info::SetPicoJsonObjectValue(output, "data", data);
  instance->PostMessageToListeners(output);
  return TRUE;
}

void SysInfoStorage::StartListening() {
  if (timeout_cb_id_ == 0) {
    timeout_cb_id_ = g_timeout_add(system_info::default_timeout_interval,
                                   SysInfoStorage::OnUpdateTimeout,
                                   static_cast<gpointer>(this));
  }
}

void SysInfoStorage::StopListening() {
  if (timeout_cb_id_ > 0) {
    g_source_remove(timeout_cb_id_);
    timeout_cb_id_ = 0;
  }
}
