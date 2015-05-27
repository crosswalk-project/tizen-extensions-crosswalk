// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_SIM_H_
#define SYSTEM_INFO_SYSTEM_INFO_SIM_H_

#include <errno.h>
#include <stdlib.h>
#if defined(TIZEN_MOBILE)
#include <sim.h>
#else
#include <gio/gio.h>
#include <glib.h>
#include <glib-object.h>
#endif

#include <string>

#include "common/picojson.h"
#include "common/utils.h"
#include "system_info/system_info_instance.h"
#include "system_info/system_info_utils.h"

class SysInfoSim : public SysInfoObject {
 public:
  static SysInfoObject& GetInstance() {
    static SysInfoSim instance;
    return instance;
  }
  ~SysInfoSim();
  void Get(picojson::value& error, picojson::value& data);
  void StartListening();
  void StopListening();

#if defined(TIZEN_MOBILE)
  typedef int (*SIMGetterFunction1)(char** out);
  typedef int (*SIMGetterFunction2)(char** out1, char** out2);

  static void OnSimStateChanged(sim_state_e state, void *user_data);
#endif

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

  static const std::string name_;

 private:
  SysInfoSim();

  void SetJsonValues(picojson::value& data);
  std::string ToSimStateString(SystemInfoSimState state);

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
  SystemInfoSimState GetSystemInfoSIMState(sim_state_e state);
#else
  void GetSimProperties();
  void GetOperatorNameAndSpn();
  void UpdateSimProperty(const gchar* key, GVariant* val);

  static void OnSimPropertyChanged(GDBusConnection* conn,
                                   const gchar* sender_name,
                                   const gchar* object_path,
                                   const gchar* iface,
                                   const gchar* signal_name,
                                   GVariant* parameters,
                                   gpointer data);
#endif

  SystemInfoSimState state_;
  std::string operator_name_;
  std::string msisdn_;
  std::string iccid_;
  unsigned int mcc_;
  unsigned int mnc_;
  std::string msin_;
  std::string spn_;
#if defined(TIZEN_IVI)
  GDBusConnection* conn_;
  std::string modem_path_;
  guint prop_changed_watch_;
#endif

  DISALLOW_COPY_AND_ASSIGN(SysInfoSim);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_SIM_H_
