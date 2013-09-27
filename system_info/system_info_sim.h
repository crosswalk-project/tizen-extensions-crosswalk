// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_SIM_H_
#define SYSTEM_INFO_SYSTEM_INFO_SIM_H_

#if defined(TIZEN_MOBILE)
#include <sim.h>
#endif
#include <string>

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"
#include "system_info/system_info_utils.h"

class SysInfoSim {
 public:
  static SysInfoSim& GetSysInfoSim() {
    static SysInfoSim instance;
    return instance;
  }
  ~SysInfoSim() {
    for (SystemInfoEventsList::iterator it = sim_events_.begin();
         it != sim_events_.end(); it++)
      StopListening(*it);
    pthread_mutex_destroy(&events_list_mutex_);
  }
  void Get(picojson::value& error, picojson::value& data);
  void StartListening(ContextAPI* api);
  void StopListening(ContextAPI* api);

  enum SystemInfoSimState {
    SYSTEM_INFO_SIM_ABSENT = 0,
    SYSTEM_INFO_SIM_INITIALIZING,
    SYSTEM_INFO_SIM_READY,
    SYSTEM_INFO_SIM_REQUIRED,
    SYSTEM_INFO_SIM_PIN_REQUIRED,
    SYSTEM_INFO_SIM_PUB_REQUIRED,
    SYSTEM_INFO_SIM_NETWORK_LOCKED,
    SYSTEM_INFO_SIM_SIM_LOCKED,
    SYSTEM_INFO_SIM_UNKNOWN
  };

#if defined(TIZEN_MOBILE)
  static void OnSimStateChanged(sim_state_e state, void *user_data);
#endif

 private:
  explicit SysInfoSim()
      : state_(SYSTEM_INFO_SIM_UNKNOWN) {
    pthread_mutex_init(&events_list_mutex_, NULL);
  }

#if defined(TIZEN_MOBILE)
  std::string ToSimStateString(SystemInfoSimState state);
  SystemInfoSimState Get_systeminfo_sim_state(sim_state_e state);
#endif

  SystemInfoSimState state_;
  std::string operatorName_;
  std::string msisdn_;
  std::string iccid_;
  std::string mcc_;
  std::string mnc_;
  std::string msin_;
  std::string spn_;
  pthread_mutex_t events_list_mutex_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoSim);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_SIM_H_
