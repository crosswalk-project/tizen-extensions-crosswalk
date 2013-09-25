// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device_capabilities/device_capabilities_storage.h"

#include <sys/statfs.h>

#include <sstream>

DeviceCapabilitiesStorage::DeviceCapabilitiesStorage() {
  DeviceStorageUnit internalUnit, mmcUnit;
  if (QueryInternal(internalUnit)) {
    storages_[internalUnit.id] = internalUnit;
  }
  if (QueryMMC(mmcUnit)) {
    storages_[mmcUnit.id] = mmcUnit;
  }
}

void DeviceCapabilitiesStorage::Get(picojson::value& obj) {
  picojson::value storages = picojson::value(picojson::array(0));
  picojson::array& units_arr = storages.get<picojson::array>();
  units_arr.clear();
  for (StoragesMap::iterator it = storages_.begin();
       it != storages_.end(); it++) {
    picojson::value unit = picojson::value(picojson::object());
    SetJsonValue(unit, it->second);
    units_arr.push_back(unit);
  }
  picojson::object& o = obj.get<picojson::object>();
  o["storages"] = storages;
}

void DeviceCapabilitiesStorage::AddEventListener(std::string event_name,
    DeviceCapabilitiesInstance* instance) {
  if (event_name == "onattach")
    device_storage_attach_events_.push_back(instance);
  else
    device_storage_detach_events_.push_back(instance);

  if ((device_storage_attach_events_.size() +
       device_storage_detach_events_.size()) == 1) {
    vconf_notify_key_changed(VCONFKEY_SYSMAN_MMC_STATUS,
        (vconf_callback_fn)OnStorageStatusChanged, this);
  }
}

void DeviceCapabilitiesStorage::RemoveEventListener(
    DeviceCapabilitiesInstance* instance) {
  device_storage_attach_events_.remove(instance);
  device_storage_detach_events_.remove(instance);
  if (device_storage_attach_events_.empty() &&
      device_storage_detach_events_.empty()) {
    vconf_ignore_key_changed(VCONFKEY_SYSMAN_MMC_STATUS,
        (vconf_callback_fn)OnStorageStatusChanged);
  }
}

bool DeviceCapabilitiesStorage::QueryInternal(DeviceStorageUnit& unit) {
  struct statfs fs;
  if (statfs("/opt/usr/media", &fs) < 0) {
    std::cout << "Internal Storage path Error \n";
    return false;
  }

  unsigned int id;
  memcpy(&id, &fs.f_fsid, sizeof(id));
  std::stringstream out;
  out << id;
  unit.id = out.str();
  // FIXME (qjia7) find which field reflects name
  unit.name = "test1";
  unit.type = "fixed";
  unit.capacity = static_cast<double>(fs.f_bsize) *
                  static_cast<double>(fs.f_blocks);
  return true;
}

bool DeviceCapabilitiesStorage::QueryMMC(DeviceStorageUnit& unit) {
  int sdcard_state;
  if ((vconf_get_int(VCONFKEY_SYSMAN_MMC_STATUS, &sdcard_state) != 0) ||
      (sdcard_state != VCONFKEY_SYSMAN_MMC_MOUNTED)) {
    return false;
  }

  struct statfs fs;
  if (statfs("/opt/storage/sdcard", &fs) < 0) {
    std::cout << "MMC mount path error \n";
    return false;
  }

  unsigned int id;
  memcpy(&id, &fs.f_fsid, sizeof(id));
  std::stringstream out;
  out << id;
  unit.id = out.str();
  // FIXME (qjia7) find which field reflects name
  unit.name = "test2";
  unit.type = "removable";
  unit.capacity = static_cast<double>(fs.f_bsize) *
                  static_cast<double>(fs.f_blocks);
  return true;
}

void DeviceCapabilitiesStorage::SetJsonValue(picojson::value& obj,
                                             const DeviceStorageUnit& unit) {
  picojson::object& o = obj.get<picojson::object>();
  o["id"] = picojson::value(unit.id);
  o["name"] = picojson::value(unit.name);
  o["type"] = picojson::value(unit.type);
  o["capacity"] = picojson::value(unit.capacity);
}

void DeviceCapabilitiesStorage::UpdateStorageUnits(std::string command) {
  picojson::value output = picojson::value(picojson::object());
  picojson::object& o = output.get<picojson::object>();
  o["cmd"] = picojson::value(command);

  DeviceStorageUnit mmcUnit;
  if (command == "attachStorage" && QueryMMC(mmcUnit)) {
    o["eventName"] = picojson::value("onattach");
    SetJsonValue(output, mmcUnit);
    storages_[mmcUnit.id] = mmcUnit;
    std::string result = output.serialize();
    const char* result_as_cstr = result.c_str();
    for (DeviceCapabilitiesEventsList::iterator
         it = device_storage_attach_events_.begin();
         it != device_storage_attach_events_.end(); it++) {
      (*it)->PostMessage(result_as_cstr);
    }
    return;
  }

  for (StoragesMap::iterator it = storages_.begin();
       it != storages_.end(); it++) {
    DeviceStorageUnit unit = it->second;
    if (unit.type == "removable") {
      o["eventName"] = picojson::value("ondetach");
      SetJsonValue(output, unit);
      storages_.erase(it);
      std::string result = output.serialize();
      const char* result_as_cstr = result.c_str();
      for (DeviceCapabilitiesEventsList::iterator
           it = device_storage_detach_events_.begin();
           it != device_storage_detach_events_.end(); it++) {
        (*it)->PostMessage(result_as_cstr);
      }
      break;
    }
  }
}

void DeviceCapabilitiesStorage::OnStorageStatusChanged(keynode_t* node,
                                                       void* user_data) {
  int status = vconf_keynode_get_int(node);
  DeviceCapabilitiesStorage* instance =
      static_cast<DeviceCapabilitiesStorage*>(user_data);
  // Attach status = 1
  // Detach status = 0
  if (status) {
    instance->UpdateStorageUnits("attachStorage");
  } else {
    instance->UpdateStorageUnits("detachStorage");
  }
}
