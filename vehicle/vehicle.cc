// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "vehicle/vehicle.h"

#include <abstractpropertytype.h>
#include <debugout.h>
#include <gio/gio.h>
#include <glib.h>
#include <listplusplus.h>
#include <superptr.hpp>

#include <algorithm>
#include <map>
#include <memory>

#include "common/extension.h"
#include "common/picojson.h"

common::Instance* Vehicle::CallbackInfo::instance = nullptr;

namespace {
const char* amb_service = "org.automotive.message.broker";
const char* prop_iface = "org.freedesktop.DBus.Properties";

picojson::value::array AmbZoneToW3C(const std::vector<int>& amb_zones);
picojson::value::array AmbZoneToW3C(int amb_zone);

template<typename T> unique_ptr<T> make_unique(T* t) {
  return unique_ptr<T>(t);
}

void PostReply(Vehicle::CallbackInfo* cb_obj, picojson::value object) {
  DebugOut() << "Posting reply" << endl;
  picojson::object msg;

  msg["method"] = picojson::value(cb_obj->method);
  msg["asyncCallId"] = picojson::value(cb_obj->callback_id);
  msg["value"] = object;

  std::string message = picojson::value(msg).serialize();

  DebugOut() << "Reply message: " << message << endl;

  cb_obj->instance->PostMessage(message.c_str());
}

void PostError(Vehicle::CallbackInfo* cb_obj, const std::string& error) {
  picojson::object msg;
  msg["method"] = picojson::value(cb_obj->method);
  msg["error"] = picojson::value(true);
  msg["value"] = picojson::value(error);
  msg["asyncCallId"] =
      picojson::value(static_cast<double>(cb_obj->callback_id));

  std::string message = picojson::value(msg).serialize();

  DebugOut() << "Error Reply message: " << message << endl;

  cb_obj->instance->PostMessage(message.c_str());
}

picojson::value GetBasic(GVariant* value) {
  std::string type = g_variant_get_type_string(value);
  picojson::value v;

  if (type == "i") {
    v = picojson::value(static_cast<double>(GVS<int>::value(value)));
  } else if (type == "d") {
    v = picojson::value(GVS<double>::value(value));
  } else if (type == "q") {
    v = picojson::value(static_cast<double>(GVS<uint16_t>::value(value)));
  } else if (type == "n") {
    v = picojson::value(static_cast<double>(GVS<int16_t>::value(value)));
  } else if (type == "y") {
    v = picojson::value(static_cast<double>(GVS<char>::value(value)));
  } else if (type == "u") {
    v = picojson::value(static_cast<double>(GVS<uint32_t>::value(value)));
  } else if (type == "x") {
    v = picojson::value(static_cast<double>(GVS<int64_t>::value(value)));
  } else if (type == "t") {
    v = picojson::value(static_cast<double>(GVS<uint64_t>::value(value)));
  } else if (type == "b") {
    v = picojson::value(GVS<bool>::value(value));
  }

  return v;
}

void AsyncGetCallback(GObject* source, GAsyncResult* res, gpointer user_data) {
  debugOut("GetAll() method call completed");

  Vehicle::CallbackInfo *cb_obj =
    static_cast<Vehicle::CallbackInfo*>(user_data);

  auto cb_obj_ptr = make_unique(cb_obj);

  if (!cb_obj_ptr) {
    debugOut("invalid cb object");
    return;
  }

  GError* error = nullptr;

  auto property_map = amb::make_super(
        g_dbus_proxy_call_finish(G_DBUS_PROXY(source), res, &error));

  auto error_ptr = amb::make_super(error);

  if (error_ptr) {
    DebugOut() << "failed to call GetAll on interface: "
               << error_ptr->message << endl;
    PostError(cb_obj_ptr.get(), "unknown");
    return;
  }

  GVariantIter* iter;
  gchar* key;
  GVariant* value;

  g_variant_get(property_map.get(), "(a{sv})", &iter);

  auto iter_ptr = amb::make_super(iter);

  picojson::value::object object;

  while (g_variant_iter_next(iter_ptr.get(), "{sv}", &key, &value)) {
    auto key_ptr = amb::make_super(key);
    auto value_ptr = amb::make_super(value);

    std::string temp_key = key_ptr.get();

    std::transform(temp_key.begin(), temp_key.begin() + 1,
                   temp_key.begin(), ::tolower);

    object[temp_key] = GetBasic(value_ptr.get());

    if (temp_key == "zone") {
      object[temp_key] =
          picojson::value(AmbZoneToW3C(object[temp_key].get<double>()));
    }
  }

  PostReply(cb_obj_ptr.get(), picojson::value(object));
}

picojson::value::array AmbZoneToW3C(int amb_zone) {
  picojson::value::array z;

  if (amb_zone & Zone::Left) {
    z.push_back(picojson::value("Left"));
  }
  if (amb_zone & Zone::Right) {
    z.push_back(picojson::value("Right"));
  }
  if (amb_zone & Zone::Front) {
    z.push_back(picojson::value("Front"));
  }
  if (amb_zone & Zone::Middle) {
    z.push_back(picojson::value("Middle"));
  }
  if (amb_zone & Zone::Center) {
    z.push_back(picojson::value("Center"));
  }
  if (amb_zone & Zone::Rear) {
    z.push_back(picojson::value("Rear"));
  }

  return z;
}

picojson::value::array AmbZoneToW3C(const std::vector<int>& amb_zones) {
  picojson::value::array zones;

  for (auto i : amb_zones) {
    zones.push_back(picojson::value(AmbZoneToW3C(i)));
  }

  return zones;
}

static void SignalCallback(GDBusConnection* connection,
                           const gchar*,
                           const gchar* object_path,
                           const gchar*,
                           const gchar*,
                           GVariant* parameters,
                           gpointer user_data) {
  DebugOut() << "Got signal" << endl;
  std::vector<ObjectZone*> amb_objects_ =
      *(static_cast<std::vector<ObjectZone*>*>(user_data));

  GVariant* value_array;
  GVariant* iface_name;
  GVariant* invalidated;

  g_variant_get(parameters,
                "(&s@a{sv}^a&s)",
                &iface_name,
                &value_array,
                &invalidated);

  GVariantIter iter;

  g_variant_iter_init(&iter, value_array);

  ObjectZone* object = nullptr;

  for (auto i : amb_objects_) {
    if (i->object_path == object_path) {
      object = i;
    }
  }

  if (!object) {
    DebugOut(DebugOut::Error) << "received signal for which "
                              << "we have no corresponding amb object" << endl;
    return;
  }

  char* key;
  GVariant* value;

  while (g_variant_iter_next(&iter, "{sv}", &key, &value)) {
    auto key_ptr = amb::make_super(key);
    auto value_ptr = amb::make_super(value);

    std::string tempkey = key_ptr.get();

    std::transform(tempkey.begin(), tempkey.begin() + 1, tempkey.begin(),
                   ::tolower);

    object->value[tempkey] = GetBasic(value_ptr.get());

    if (tempkey == "zone") {
      object->value[tempkey] =
          picojson::value(AmbZoneToW3C(object->value[tempkey].get<double>()));
    }
  }

  object->value["interfaceName"] = picojson::value(object->name);

  Vehicle::CallbackInfo call;
  call.method = "subscribe";
  call.callback_id = -1;

  PostReply(&call, picojson::value(object->value));
}

}  // namespace

Vehicle::Vehicle(common::Instance* instance)
  : main_loop_(g_main_loop_new(0, FALSE)),
    thread_(Vehicle::SetupMainloop, this),
    instance_(instance) {
  CallbackInfo::instance = instance_;
  thread_.detach();
}

Vehicle::~Vehicle() {
  g_main_loop_quit(main_loop_);
  g_main_loop_unref(main_loop_);

  for (auto i : amb_objects_) {
    delete i;
  }
}

void Vehicle::Get(const std::string& property, Zone::Type zone, double ret_id) {
  CallbackInfo* data = new CallbackInfo;

  data->callback_id = ret_id;
  data->method = "get";
  data->instance = instance_;

  std::string obj_pstr = FindProperty(property, zone);

  if (obj_pstr.empty()) {
    debugOut("could not find property " + property);
    PostError(data, "invalid_operation");
    return;
  }

  GError* error = nullptr;

  auto properties_proxy = amb::make_super(
      g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                    G_DBUS_PROXY_FLAGS_NONE, NULL,
                                    amb_service,
                                    obj_pstr.c_str(),
                                    prop_iface,
                                    NULL,
                                    &error));

  auto error_ptr = amb::make_super(error);

  if (error_ptr) {
    debugOut("failed to get properties proxy");
    return;
  }

  std::string interfaceName = "org.automotive." + property;

  g_dbus_proxy_call(properties_proxy.get(),
                    "GetAll",
                    g_variant_new("(s)", interfaceName.c_str()),
                    G_DBUS_CALL_FLAGS_NONE, -1, NULL,
                    AsyncGetCallback, data);
}

void Vehicle::GetZones(const std::string& object_name, double ret_id) {
  auto manager_proxy = amb::make_super(GetAutomotiveManager());

  if (!manager_proxy) {
    return;
  }

  GError* error(nullptr);

  auto zones_variant = amb::make_super(
      g_dbus_proxy_call_sync(manager_proxy.get(),
                             "ZonesForObjectName",
                             g_variant_new("(s)",
                                           object_name.c_str()),
                             G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error));

  auto error_ptr = amb::make_super(error);

  if (error_ptr) {
    DebugOut() << "error calling ZonesForObjectName: "
               << error_ptr->message << endl;
    return;
  }

  if (!zones_variant) {
    DebugOut() << "Invalid response from ZonesForObjectName " << endl;
    return;
  }

  GVariantIter* iter(nullptr);

  g_variant_get(zones_variant.get(), "(ai)", &iter);

  if (!iter) {
    DebugOut() << "No zones for object " << object_name << endl;
    return;
  }

  auto iter_ptr = amb::make_super(iter);

  std::vector<int> zones_array;

  GVariant* value(nullptr);

  while ((value = g_variant_iter_next_value(iter_ptr.get()))) {
    auto value_ptr = amb::make_super(value);
    int v = 0;

    g_variant_get(value_ptr.get(), "(i)", &v);
    zones_array.push_back(v);
  }

  picojson::value::array w3c_zones = AmbZoneToW3C(zones_array);

  CallbackInfo* data = new CallbackInfo;

  data->callback_id = ret_id;
  data->method = "zones";
  data->instance = instance_;

  PostReply(data, picojson::value(w3c_zones));
}

std::string Vehicle::FindProperty(const std::string& object_name, int zone) {
  auto manager_proxy = amb::make_super(GetAutomotiveManager());

  if (!manager_proxy) {
    return "";
  }

  GError* error(nullptr);

  auto object_path_variant = amb::make_super(
      g_dbus_proxy_call_sync(manager_proxy.get(),
                             "FindObjectForZone",
                             g_variant_new("(si)",
                                           object_name.c_str(),
                                           zone),
                             G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error));

  auto error_ptr = amb::make_super(error);

  if (error_ptr) {
    DebugOut() << "error calling FindObjectForZone: "
               << error_ptr->message << endl;

    DebugOut() << "Could not find object in zone: " << zone << endl;
    return "";
  }

  if (!object_path_variant) {
    DebugOut() << "Could not find object in zone: "  << zone << endl;
    return "";
  }

  gchar* obj_path = nullptr;
  g_variant_get(object_path_variant.get(), "(o)", &obj_path);

  auto obj_path_ptr = amb::make_super(obj_path);

  DebugOut() << "FindObjectForZone() returned object path: "
             << obj_path_ptr.get() << endl;

  return obj_path;
}

GDBusProxy* Vehicle::GetAutomotiveManager() {
  GError* error = nullptr;
  GDBusProxy* am =
      g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                    G_DBUS_PROXY_FLAGS_NONE, NULL,
                                    amb_service,
                                    "/",
                                    "org.automotive.Manager",
                                    NULL,
                                    &error);

  auto error_ptr = amb::make_super(error);

  if (error_ptr) {
     DebugOut() << "error calling GetAutomotiveManager: "
                << error_ptr->message << endl;
  }

  return am;
}

void Vehicle::SetupMainloop(void* data) {
  Vehicle* self = reinterpret_cast<Vehicle*>(data);
  GMainContext* ctx = g_main_context_default();

  g_main_context_push_thread_default(ctx);
  g_main_loop_run(self->main_loop_);
}


void Vehicle::Subscribe(const std::string& object_name, Zone::Type zone) {
  std::string object_path = FindProperty(object_name, zone);

  if (object_path.empty()) {
    DebugOut() << "Error FindProperty failed for " << object_name;
    return;
  }

  bool already_subscribed = false;

  for (auto i : amb_objects_) {
    if (i->object_path == object_path) {
      already_subscribed = true;
      break;
    }
  }

  if (!already_subscribed) {
    GError* proxy_error = nullptr;

    auto properties_proxy =
        amb::make_super(g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                      G_DBUS_PROXY_FLAGS_NONE,
                                                      NULL,
                                                      amb_service,
                                                      object_path.c_str(),
                                                      prop_iface,
                                                      NULL,
                                                      &proxy_error));

    auto proxy_error_ptr = amb::make_super(proxy_error);

    if (proxy_error_ptr) {
      DebugOut() << "error creating properties proxy: "
                 << proxy_error_ptr->message << endl;
    }

    std::string interface_name = "org.automotive." + object_name;

    GError* get_all_error = nullptr;

    GVariant* property_map =
        g_dbus_proxy_call_sync(properties_proxy.get(),
                               "GetAll",
                               g_variant_new("(s)", interface_name.c_str()),
                               G_DBUS_CALL_FLAGS_NONE, -1, NULL,
                               &get_all_error);

    auto get_all_error_ptr = amb::make_super(get_all_error);

    if (get_all_error_ptr) {
      DebugOut(DebugOut::Error) << "failed to call GetAll on interface "
                                << interface_name << " "
                                << get_all_error_ptr->message << endl;
      return;
    }

    GVariantIter* iter;

    g_variant_get(property_map, "(a{sv})", &iter);

    auto iter_ptr = amb::make_super(iter);

    char* key;
    GVariant* value;

    ObjectZone* object = new ObjectZone(object_name, zone, object_path);

    while (g_variant_iter_next(iter_ptr.get(), "{sv}", &key, &value)) {
      auto key_ptr = amb::make_super(key);
      auto value_ptr = amb::make_super(value);

      std::string tempkey = key_ptr.get();

      std::transform(tempkey.begin(), tempkey.begin() + 1, tempkey.begin(),
                     ::tolower);

      object->value[tempkey] = GetBasic(value_ptr.get());
    }

    object->handle =
        g_dbus_connection_signal_subscribe(g_bus_get_sync(G_BUS_TYPE_SYSTEM,
                                                          NULL, NULL),
                                           amb_service,
                                           prop_iface,
                                           "PropertiesChanged",
                                           object_path.c_str(), NULL,
                                           G_DBUS_SIGNAL_FLAGS_NONE,
                                           SignalCallback, &amb_objects_,
                                           NULL);

    amb_objects_.push_back(object);
  } else {
    DebugOut() << "Already subscribed to " << object_name << endl;
  }
}


void Vehicle::Unsubscribe(const std::string& property, Zone::Type zone) {
  std::vector<ObjectZone*> to_clean;

  for (auto obj : amb_objects_) {
    if (obj->name == property && obj->zone == zone) {
      g_dbus_connection_signal_unsubscribe(g_bus_get_sync(G_BUS_TYPE_SYSTEM,
                                                          NULL, NULL),
                                           obj->handle);
      to_clean.push_back(obj);
    }
  }

  for (auto obj : to_clean) {
    removeOne(&amb_objects_, obj);
    delete obj;
  }
}
