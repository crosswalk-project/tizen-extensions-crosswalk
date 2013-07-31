// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_CONTEXT_H_
#define SYSTEM_INFO_SYSTEM_INFO_CONTEXT_H_

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "system_info/system_info_battery.h"
#include "system_info/system_info_build.h"
#include "system_info/system_info_cellular_network.h"
#include "system_info/system_info_cpu.h"
#include "system_info/system_info_device_orientation.h"
#include "system_info/system_info_display.h"
#include "system_info/system_info_locale.h"
#include "system_info/system_info_network.h"
#include "system_info/system_info_peripheral.h"
#include "system_info/system_info_sim.h"
#include "system_info/system_info_storage.h"
#include "system_info/system_info_wifi_network.h"

namespace picojson {
class value;
}  // namespace picojson

class SystemInfoContext {
 public:
  SystemInfoContext(ContextAPI* api);
  ~SystemInfoContext();

  // ExtensionAdapter implementation.
  static const char name[];
  static const char* GetJavaScript();
  void HandleMessage(const char*);
  void HandleSyncMessage(const char*);

 private:
  void HandleGetPropertyValue(const picojson::value& input,
                              picojson::value& output);
  void HandleStartListen(const picojson::value& input);
  void HandleStopListen(const picojson::value& input);
  void GetDeviceOrientation(picojson::value& error,
                            picojson::value& data);
  void GetNetwork(picojson::value& error,
                  picojson::value& data);
  void GetWifiNetwork(picojson::value& error,
                      picojson::value& data);
  void GetCellularNetwork(picojson::value& error,
                          picojson::value& data);
  void GetSIM(picojson::value& error,
              picojson::value& data);
  void GetPeripheral(picojson::value& error,
                     picojson::value& data);

  ContextAPI* api_;
  SysInfoBattery& battery_;
  SysInfoBuild& build_;
  SysInfoCellularNetwork& cellular_network_;
  SysInfoCpu& cpu_;
  SysInfoDeviceOrientation& device_orientation_;
  SysInfoDisplay& display_;
  SysInfoLocale& locale_;
  SysInfoNetwork& network_;
  SysInfoPeripheral& peripheral_;
  SysInfoSim& sim_;
  SysInfoStorage& storage_;
  SysInfoWifiNetwork& wifi_network_;
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_CONTEXT_H_
