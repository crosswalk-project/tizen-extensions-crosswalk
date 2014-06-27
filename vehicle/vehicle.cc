// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "vehicle/vehicle.h"

#include <abstractpropertytype.h>
#include <debugout.h>
#include <gio/gio.h>
#include <glib.h>

#include <algorithm>
#include <map>
#include <memory>

#include "common/extension.h"
#include "common/picojson.h"

namespace {

template<typename T> struct traits;

template<>
struct traits<GVariant> {
  struct delete_functor {
    void operator()(GVariant * p) const {
      if (p != nullptr)
        g_variant_unref(p);
    }
  };
};

template<>
struct traits<GError> {
  struct delete_functor {
    void operator()(GError * p) const {
      if (p != nullptr)
        g_error_free(p);
    }
  };
};

template<>
struct traits<GDBusProxy> {
  struct delete_functor {
    void operator()(GDBusProxy * p) const {
      if (p != nullptr)
        g_object_unref(p);
    }
  };
};

template<>
struct traits<GVariantIter> {
  struct delete_functor {
    void operator()(GVariantIter * p) const {
      if (p != nullptr)
        g_variant_iter_free(p);
    }
  };
};

template<>
struct traits<gchar> {
  struct delete_functor {
    void operator()(gchar * p) const {
      if (p != nullptr)
        g_free(p);
    }
  };
};

template<typename T> using super_ptr =
    ::std::unique_ptr<T, typename traits<T>::delete_functor>;

template<typename T> super_ptr<T> make_super(T* t) {
  return super_ptr<T>(t);
}

void PostReply(Vehicle::CallbackInfo* cb_obj, picojson::object object) {
  picojson::object msg;

  msg["method"] = picojson::value(cb_obj->method);
  msg["asyncCallId"] = picojson::value(cb_obj->callback_id);
  msg["value"] = picojson::value(object);

  std::string message = picojson::value(msg).serialize();

  DebugOut() << "Reply message: " << message << endl;

  cb_obj->instance->PostMessage(message.c_str());
}

void PostError(Vehicle::CallbackInfo* cb_obj, std::string error) {
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

void AsyncCallback(GObject* source, GAsyncResult* res, gpointer user_data) {
  debugOut("GetAll() method call completed");

  Vehicle::CallbackInfo *cb_obj =
    static_cast<Vehicle::CallbackInfo*>(user_data);

  if (!cb_obj) {
    debugOut("invalid cb object");
    return;
  }

  GError* error = nullptr;

  auto property_map = make_super(
        g_dbus_proxy_call_finish(G_DBUS_PROXY(source), res, &error));

  auto error_ptr = make_super(error);

  if (error_ptr) {
    DebugOut() << "failed to call GetAll on interface: "
               << error_ptr->message << endl;
    PostError(cb_obj, "unknown");
    delete cb_obj;
    return;
  }

  GVariantIter* iter;
  gchar* key;
  GVariant* value;

  g_variant_get(property_map.get(), "(a{sv})", &iter);

  auto iter_ptr = make_super(iter);

  std::map<std::string, GVariant*> return_value;

  while (g_variant_iter_next(iter_ptr.get(), "{sv}", &key, &value)) {
    return_value[key] = value;
    g_free(key);
  }

  picojson::value::object object;

  for (auto itr = return_value.begin(); itr != return_value.end(); itr++) {
    std::string key = (*itr).first;

    /// make key lowerCamelCase:
    std::transform(key.begin(), key.begin() + 1, key.begin(), ::tolower);

    auto variant = make_super((*itr).second);
    picojson::value v = GetBasic(variant.get());
    object[key] = v;
  }

  PostReply(cb_obj, object);
  delete cb_obj;
}

}  // namespace

Vehicle::Vehicle(common::Instance* instance)
  : main_loop_(g_main_loop_new(0, FALSE)),
    thread_(Vehicle::SetupMainloop, this),
    instance_(instance) {
  thread_.detach();
}

Vehicle::~Vehicle() {
  g_main_loop_quit(main_loop_);
  g_main_loop_unref(main_loop_);
}

void Vehicle::Get(const std::string& property, Zone::Type zone, double ret_id) {
  CallbackInfo * data = new CallbackInfo;

  data->callback_id = ret_id;
  data->method = "get";
  data->instance = instance_;

  std::string obj_pstr = FindProperty(property, zone);

  if (obj_pstr.empty()) {
    debugOut("could not find property " + property);
    PostError(data, "invalid_operation");
    return;
  }

  debugOut("Getting properties interface");

  GError* error = nullptr;

  auto properties_proxy = make_super(
      g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                    G_DBUS_PROXY_FLAGS_NONE, NULL,
                                    "org.automotive.message.broker",
                                    obj_pstr.c_str(),
                                    "org.freedesktop.DBus.Properties",
                                    NULL,
                                    &error));

  auto error_ptr = make_super(error);

  if (error_ptr) {
    debugOut("failed to get properties proxy");
    return;
  }

  std::string interfaceName = "org.automotive." + property;

  debugOut("Calling GetAll");

  g_dbus_proxy_call(properties_proxy.get(),
                    "GetAll",
                    g_variant_new("(s)", interfaceName.c_str()),
                    G_DBUS_CALL_FLAGS_NONE, -1, NULL,
                    AsyncCallback, data);
}

std::string Vehicle::FindProperty(const std::string& object_name, int zone) {
  auto manager_proxy = make_super(GetAutomotiveManager());

  if (!manager_proxy) {
    return "";
  }

  GError* error(nullptr);

  auto object_path_variant = make_super(
      g_dbus_proxy_call_sync(manager_proxy.get(),
                             "FindObjectForZone",
                             g_variant_new("(si)",
                                           object_name.c_str(),
                                           zone),
                             G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error));

  auto error_ptr = make_super(error);

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

  auto obj_path_ptr = make_super(obj_path);

  DebugOut() << "FindObjectForZone() returned object path: " <<
                obj_path_ptr.get() << endl;

  return obj_path;
}

GDBusProxy* Vehicle::GetAutomotiveManager() {
  GError* error = nullptr;
  GDBusProxy* am =
      g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                    G_DBUS_PROXY_FLAGS_NONE, NULL,
                                    "org.automotive.message.broker",
                                    "/",
                                    "org.automotive.Manager",
                                    NULL,
                                    &error);

  auto error_ptr = make_super(error);

  if (error_ptr) {
     DebugOut() << "error calling GetAutomotiveManager: "
                << error_ptr->message << endl;
  }

  return am;
}

void Vehicle::SetupMainloop(void *data) {
  Vehicle* self = reinterpret_cast<Vehicle*>(data);
  GMainContext* ctx = g_main_context_default();

  g_main_context_push_thread_default(ctx);
  g_main_loop_run(self->main_loop_);
}
