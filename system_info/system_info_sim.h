// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_SIM_H_
#define SYSTEM_INFO_SYSTEM_INFO_SIM_H_

#if defined(TIZEN_MOBILE)
#include <errno.h>
#include <sim.h>
#include <stdlib.h>
#include <stdio.h>
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
  typedef int (*SIMGetterFunction1)(char** out);
  typedef int (*SIMGetterFunction2)(char** out1, char** out2);

  static void OnSimStateChanged(sim_state_e state, void *user_data);
#endif

 private:
  explicit SysInfoSim()
      : state_(SYSTEM_INFO_SIM_UNKNOWN),
        operatorName_(""),
        msisdn_(""),
        iccid_(""),
        mcc_(0),
        mnc_(0),
        msin_(""),
        spn_("") {
    pthread_mutex_init(&events_list_mutex_, NULL);
  }

#if defined(TIZEN_MOBILE)
  bool QuerySIMStatus();
  bool QuerySIM(SIMGetterFunction1 getter,
                std::string& member,
                const std::string& default_value = "N/A");
  bool QuerySIM(SIMGetterFunction2 getter,
                std::string& member,
                const std::string& default_value = "N/A");
  bool QuerySIM(SIMGetterFunction1 getter,
                unsigned int& member,
                const unsigned int& default_value = 0);
  void SetJsonValues(picojson::value& data);
  std::string ToSimStateString(SystemInfoSimState state);
  SystemInfoSimState GetSystemInfoSIMState(sim_state_e state);
#endif

  SystemInfoSimState state_;
  std::string operatorName_;
  std::string msisdn_;
  std::string iccid_;
  unsigned int mcc_;
  unsigned int mnc_;
  std::string msin_;
  std::string spn_;
  pthread_mutex_t events_list_mutex_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoSim);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_SIM_H_
