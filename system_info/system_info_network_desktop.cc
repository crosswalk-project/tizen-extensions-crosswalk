// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_network.h"

#include <NetworkManager.h>

#include "system_info/system_info_utils.h"
#include "system_info_marshaller.h"

using namespace system_info;

namespace {
 
#define DBUS_NM_SERVICE "org.freedesktop.NetworkManager"
// for StateChanged
#define DBUS_NM_DEVICE "/org/freedesktop/NetworkManager/Device"
#define DBUS_NM_DEVICE_INTERFACE "org.freedesktop.NetworkManager.Device"
// for property
#define DBUS_NM_DEVICES_0 "/org/freedesktop/NetworkManager/Devices/0"
#define DBUS_PROPERTY_INTERFACE "org.freedesktop.DBus.Properties"

}  // namespace

SysInfoNetwork::SysInfoNetwork(ContextAPI* api)
    : type_(SYSTEM_INFO_NETWORK_NONE),
      stopping_(false),
      dbus_nm_proxy_(NULL),
      dbus_prop_proxy_(NULL),
      nm_device_type_(NM_DEVICE_TYPE_UNKNOWN) {
  api_ = api;

  GError *error = NULL;
  DBusGConnection *connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
  if (!connection)
    return;

  dbus_prop_proxy_ = dbus_g_proxy_new_for_name(connection,
                                               DBUS_NM_SERVICE,
                                               DBUS_NM_DEVICES_0,
                                               DBUS_PROPERTY_INTERFACE);

  dbus_nm_proxy_ = dbus_g_proxy_new_for_name(connection,
                                             DBUS_NM_SERVICE,
                                             DBUS_NM_DEVICES_0,
                                             DBUS_NM_DEVICE_INTERFACE);

  // FIXME(halton): add marshaller will cause api expose failure
  /*
  dbus_g_object_register_marshaller(g_cclosure_user_marshal_VOID__UINT_UINT_UINT,
                                    G_TYPE_NONE,
                                    G_TYPE_UINT,
                                    G_TYPE_UINT,
                                    G_TYPE_UINT,
                                    G_TYPE_INVALID);
  */
  dbus_g_proxy_add_signal(dbus_nm_proxy_,
                          "StateChanged",
                          G_TYPE_UINT,
                          G_TYPE_UINT,
                          G_TYPE_UINT,
                          G_TYPE_INVALID);
  dbus_g_proxy_connect_signal(dbus_nm_proxy_, "StateChanged",
                              G_CALLBACK(SysInfoNetwork::OnNMStateChanged),
                              static_cast<void *>(this),
                              NULL);
}

SysInfoNetwork::~SysInfoNetwork() {
  if(dbus_nm_proxy_)
    g_object_unref(dbus_nm_proxy_);
  if(dbus_prop_proxy_)
    g_object_unref(dbus_prop_proxy_);
}

bool SysInfoNetwork::Update(picojson::value& error) {
  GError* err = NULL;
  GValue value = {0,};
  if (!dbus_g_proxy_call(dbus_prop_proxy_, "Get",
                         &err,
                         G_TYPE_STRING, DBUS_NM_DEVICE_INTERFACE,
                         G_TYPE_STRING, "DeviceType",
                         G_TYPE_INVALID,
                         G_TYPE_VALUE, &value,
                         G_TYPE_INVALID)) {
    SetPicoJsonObjectValue(error, "message",
        picojson::value(err->message));
    return false;
  }

  guint device_type = g_value_get_uint(&value);
  if (device_type != nm_device_type_) {
    nm_device_type_ = device_type;
    type_ = ToNetworkType(device_type);
  }
  return true;
}

void SysInfoNetwork::OnNMStateChanged(DBusGProxy* proxy,
                                      guint new_state,
                                      guint old_state,
                                      guint reason,
                                      gpointer user_data) {
  SysInfoNetwork* instance = static_cast<SysInfoNetwork*>(user_data);
  guint old_nm_device_type = instance->nm_device_type_;

  picojson::value error = picojson::value(picojson::object());;
  if(!instance->Update(error))
    return;

  if (old_nm_device_type != instance->nm_device_type_) {
    picojson::value output = picojson::value(picojson::object());;
    picojson::value data = picojson::value(picojson::object());

    SetPicoJsonObjectValue(data, "networkType",
        picojson::value(static_cast<double>(instance->type_)));
    SetPicoJsonObjectValue(output, "cmd",
        picojson::value("SystemInfoPropertyValueChanged"));
    SetPicoJsonObjectValue(output, "prop", picojson::value("NETWORK"));
    SetPicoJsonObjectValue(output, "data", data);

    std::string result = output.serialize();
    instance->api_->PostMessage(result.c_str());
  }
}

SystemInfoNetworkType SysInfoNetwork::ToNetworkType(guint device_type) {
  SystemInfoNetworkType ret = SYSTEM_INFO_NETWORK_UNKNOWN;

  switch (device_type) {
    case NM_DEVICE_TYPE_ETHERNET:
      ret = SYSTEM_INFO_NETWORK_ETHERNET; 
      break;
    case NM_DEVICE_TYPE_WIFI:
      ret = SYSTEM_INFO_NETWORK_WIFI;
      break;
    case NM_DEVICE_TYPE_MODEM:
      // FIXME(halton): Identify 2G/2.5G/3G/4G
      break;
    case NM_DEVICE_TYPE_UNKNOWN:
    default:
      ret = SYSTEM_INFO_NETWORK_UNKNOWN;
  }

  return ret;
}
