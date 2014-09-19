// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of  source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "telephony/telephony_backend_ofono.h"
#include "telephony/telephony_logging.h"

#include <gio/gio.h>
#include <time.h>
#include <uuid/uuid.h>

#include <string>
#include <sstream>

namespace {

const char kDbusOfonoService[] = "org.ofono";
const char kDbusOfonoModemManager[] = "org.ofono.Manager";
const char kDbusOfonoModem[] = "org.ofono.Modem";
const char kDbusOfonoVoiceCallManager[] = "org.ofono.VoiceCallManager";
const char kDbusOfonoVoiceCall[] = "org.ofono.VoiceCall";
const char kDbusOfonoSimManager[] = "org.ofono.SimManager";

const char kCallStateInit[] = "init";
const char kCallStateActive[] = "active";
const char kCallStateHeld[] = "held";
const char kCallStateDialing[] = "dialing";
const char kCallStateAlerting[] = "alerting";
const char kCallStateIncoming[] = "incoming";
const char kCallStateWaiting[] = "waiting";
const char kCallStateDisconnected[] = "disconnected";
const char kCallStateConference[] = "conference";

}  // namespace

//////////////////////////////////////////////////////////////////////////////
// TelephonyBackend, DBUS/signal handling
///////////////////////////////////////////////////////////////////////////////

TelephonyBackend::TelephonyBackend(TelephonyInstance* instance)
    : instance_(instance), cancellable_(nullptr), default_service_(nullptr),
    active_call_(nullptr) {
}

TelephonyBackend::~TelephonyBackend() {
  DisableSignalHandlers();
  for (auto i : calls_)
    delete i;
  for (auto i : services_)
    delete i;
  for (auto i : removed_calls_)
    delete i;
}

void TelephonyBackend::SetDBusSignalHandler(const gchar* iface,
    const gchar* name, const gchar* obj, GDBusSignalCallback cb,
    gpointer user_data) {
  LOG_DBG("SetDBusSignalHandler: " << std::string(iface) << ":" <<
      std::string(name) << " [" << std::string(obj ? obj : "") << "]");

  GDBusConnection* dbus_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr,
      nullptr);

  guint handle = g_dbus_connection_signal_subscribe(dbus_conn, nullptr,
      iface, name, obj, nullptr, G_DBUS_SIGNAL_FLAGS_NONE, cb, user_data,
      nullptr);

  dbus_listeners_.push_back(handle);
}

GVariant* TelephonyBackend::SyncDBusCall(const gchar* object,
    const gchar* interface, const gchar* method, GVariant* parameters,
    GError** error) {
  LOG_DBG("SyncDBusCall: " << std::string(method));

  GDBusConnection* dbus_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr,
      nullptr);

  return g_dbus_connection_call_sync(dbus_conn, kDbusOfonoService,
    object, interface, method, parameters,
    nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, error);
}

void TelephonyBackend::AsyncDBusCall(const gchar* object,
    const gchar* interface, const gchar* method, GVariant* parameters,
    GAsyncReadyCallback callback, gpointer user_data) {

  GDBusConnection* dbus_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr,
      nullptr);

  g_dbus_connection_call(dbus_conn, kDbusOfonoService,
    object, interface, method, parameters,
    nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, callback, user_data);
}

void TelephonyBackend::EnableSignalHandlers() {
  SetDBusSignalHandler(kDbusOfonoModemManager, "ModemAdded", nullptr,
    SignalHandler, this);
  SetDBusSignalHandler(kDbusOfonoModemManager, "ModemRemoved", nullptr,
    SignalHandler, this);
  SetDBusSignalHandler(kDbusOfonoModem, "PropertyChanged", nullptr,
    SignalHandler, this);

  SetDBusSignalHandler(kDbusOfonoVoiceCallManager, "CallAdded", nullptr,
    SignalHandler, this);
  SetDBusSignalHandler(kDbusOfonoVoiceCallManager, "CallRemoved", nullptr,
    SignalHandler, this);
  SetDBusSignalHandler(kDbusOfonoVoiceCallManager, "PropertyChanged", nullptr,
    SignalHandler, this);

  SetDBusSignalHandler(kDbusOfonoVoiceCall, "PropertyChanged", nullptr,
    SignalHandler, this);
  SetDBusSignalHandler(kDbusOfonoVoiceCall, "DisconnectReason", nullptr,
    SignalHandler, this);
}

// Called when the last signal listener is removed.
void TelephonyBackend::DisableSignalHandlers() {
  LOG_DBG("DisableSignalHandlers");
  GDBusConnection* dbus_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr,
      nullptr);
  for (int i = 0; i < dbus_listeners_.size(); i++) {
    g_dbus_connection_signal_unsubscribe(dbus_conn, dbus_listeners_[i]);
  }
  dbus_listeners_.clear();
}

void TelephonyBackend::SignalHandler(GDBusConnection* connection,
    const gchar* sender, const gchar* object,
    const gchar* interface, const gchar* signal,
    GVariant* parameters, gpointer user_data) {

  TelephonyBackend* backend = static_cast<TelephonyBackend*>(user_data);
  if (!backend) {
    LOG_ERR("SignalHandler: failed to get backend");
    return;
  }
  LOG_DBG("SignalHandler: " << std::string(signal));

  if (!strcmp(interface, kDbusOfonoModemManager)) {
    if (!strcmp(signal, "ModemAdded")) {
      backend->OnModemAdded(object, parameters);
    } else if (!strcmp(signal, "ModemRemoved")) {
      backend->OnModemRemoved(object, parameters);
    }
    return;
  }

  if (!strcmp(interface, kDbusOfonoModem)) {
      backend->OnModemChanged(object, parameters);
      return;
  }

  if (!strcmp(interface, kDbusOfonoVoiceCallManager)) {
    if (!strcmp(signal, "CallAdded")) {
      backend->OnCallAdded(object, parameters);
    } else if (!strcmp(signal, "CallRemoved")) {
      backend->OnCallRemoved(object, parameters);
    } else if (!strcmp(signal, "PropertyChanged")) {
      backend->OnCallManagerChanged(object, parameters);
    }
    return;
  }

  if (!strcmp(interface, kDbusOfonoVoiceCall)) {
    if (!strcmp(signal, "PropertyChanged")) {
      backend->OnCallChanged(object, parameters);
    } else if (!strcmp(signal, "DisconnectReason")) {
      backend->OnCallDisconnectReason(object, parameters);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// TelephonyService
///////////////////////////////////////////////////////////////////////////////

picojson::object& TelephonyBackend::ToJson(TelephonyService* service,
                                           picojson::object& out) {
  out["serviceId"] = picojson::value(service->id);
  out["name"] = picojson::value(service->name);
  out["serviceType"] = picojson::value(service->type);
  out["enabled"] = picojson::value(service->online? true : false);
  out["provider"] = picojson::value(service->provider);
  out["protocol"] = picojson::value(service->protocol);
  out["emergency"] = picojson::value(service->emergency);
  return out;
}

void TelephonyBackend::LogService(TelephonyService* service) {
  std::cout << "{\n\tserviceId: " << service->id;
  std::cout << "\n\tname: " << service->name;
  std::cout << "\n\tenabled: " <<
      (service->powered && service->online ? "true" : "false");
  std::cout << "\n\tserial: " << service->serial;  // e.g. BT address comes here
  std::cout << "\n}" << std::endl;
}

// Return the affected JS property name
const char* TelephonyBackend::UpdateServiceProperty(TelephonyService* service,
    const char* key, GVariant* value) {
  if (!strcmp(key, "Powered")) {
    service->powered = g_variant_get_boolean(value);
    return "enabled";
  }

  if (!strcmp(key, "Online")) {
    service->online = g_variant_get_boolean(value);
    return "enabled";
  }

  if (!strcmp(key, "Lockdown")) {
    service->lockdown = g_variant_get_boolean(value);
    return nullptr;
  }

  if (!strcmp(key, "Emergency")) {
    service->emergency = g_variant_get_boolean(value);
    return "emergency";
  }

  if (!strcmp(key, "Manufacturer")) {
    service->provider = g_variant_get_string(value, nullptr);
    return "name";
  }

  if (!strcmp(key, "Name")) {
    service->name = g_variant_get_string(value, nullptr);
    return "name";
  }

  if (!strcmp(key, "Model")) {
    service->model = g_variant_get_string(value, nullptr);
    return "name";
  }

  if (!strcmp(key, "Revision")) {
    service->revision = g_variant_get_string(value, nullptr);
    return "name";
  }

  if (!strcmp(key, "Serial")) {
    service->serial = g_variant_get_string(value, nullptr);
    return "name";
  }

  if (!strcmp(key, "Type")) {
    const gchar* stype = g_variant_get_string(value, nullptr);
    if (!strcmp(stype, "hardware"))
      service->type = "hw";
    else if (!strcmp(stype, "hfp"))
      service->type = "hfp";
    else if (!strcmp(stype, "sap"))
      service->type = "sap";
    else
      service->type = "unknown";
    return "type";
  }

  return nullptr;
}

bool TelephonyBackend::InitService(TelephonyService* service,
    const std::string& sid, GVariantIter* props) {

  if (sid.empty() || !props) {
    return false;
  }

  service->id = sid;
  service->protocol = "gsm";

  char* key = nullptr;
  GVariant* value = nullptr;
  while (g_variant_iter_next(props, "{sv}", &key, &value)) {
    UpdateServiceProperty(service, key, value);
  }
  g_variant_unref(value);

  return true;
}

void TelephonyBackend::UpdateService(TelephonyService* service) {
  GError* err = nullptr;
  GVariant* res = SyncDBusCall(IdToDbus(service->id).c_str(),
      kDbusOfonoModemManager, "GetProperties", nullptr, &err);

  if (err) {
    LOG_ERR("UpdateService: " << err->message);
    return;
  }

  GVariantIter* props = nullptr;
  g_variant_get(res, "(a{sv})", &props);
  InitService(service, service->id, props);
  g_variant_iter_free(props);
  g_variant_unref(res);
}

///////////////////////////////////////////////////////////////////////////////
// TelephonyCall
///////////////////////////////////////////////////////////////////////////////

picojson::object& TelephonyBackend::ToJson(TelephonyCall* call,
                                           picojson::object& out) {
  out["callId"] = picojson::value(call->id);
  out["serviceId"] = picojson::value(call->service ?
                                     call->service->id :
                                     "");
  out["remoteParty"] = picojson::value(call->remote_party);
  out["state"] = picojson::value(call->state);
  out["stateReason"] = picojson::value(call->state_reason);
  out["startTime"] = picojson::value(call->start_time);
  out["duration"] = picojson::value(call->duration);
  out["protocol"] = picojson::value(call->protocol);
  out["emergency"] = picojson::value(call->emergency);
  out["conferenceId"] = picojson::value(call->conference ?
                                        call->conference->id : "");
  picojson::value::array p;
  for (auto c : call->participants) {
    if (c)
      p.push_back(picojson::value(c->id));
  }
  out["participants"] = picojson::value(p);
  return out;
}

void TelephonyBackend::LogCall(TelephonyCall* call) {
  std::cout << "{\n\tcallId: " << call->id;
  std::cout << "\n\tremoteParty: " << call->remote_party;
  std::cout << "\n\tstate: " << call->state;
  std::cout << "\n\tserviceId: " << call->service->id;
  std::cout << "\n\tstartTime: " << call->start_time;
  std::cout << "\n}" << std::endl;
}

void TelephonyBackend::UpdateDuration(TelephonyCall* call) {
  time_t endt = time(nullptr);
  struct tm tm;
  // time format from <ofono source>/src/voicecall.c
  strptime(call->start_time.c_str(), "%Y-%m-%dT%H:%M:%S%z", &tm);
  time_t start_time = mktime(&tm);
  call->duration = difftime(endt, start_time) * 1000.0;  // sec --> msec
}

const char* TelephonyBackend::UpdateCallState(TelephonyCall* call,
                                              std::string& state) {
  if (state == kCallStateDisconnected) {  // stamp end time
    UpdateDuration(call);
    call->state = state;
    return "state";
  }

  TelephonyCall* conf = call->conference;
  // Check on conference participants state change.
  if (conf && call != conf) {
    // Calls participating in a conference stay in 'conference' state, but
    // the shadow state is updated.
    // Note that with oFono objects, all connected calls participating in a
    // multiparty call are in 'active' or 'held' state, however, the W3C spec
    // treats them as 'conference' state. When the state of a participating
    // call object is changed, it is tracked separately, and the conference
    // call state inherits the shadow state of participating calls.
    call->saved_state = state;

    if (state != conf->state) {
      bool change_conf_state = false;
      if (state == kCallStateActive) {  // at least one active participant
        change_conf_state = true;
      } else if (state == kCallStateHeld) {
        // all participating calls must be held in order to change
        // the state of the conference calls
        change_conf_state = true;
        for (auto p : conf->participants) {
          if (p && p->state != kCallStateHeld)
            change_conf_state = false;
        }
      }

      if (change_conf_state) {
        conf->state = state;
        NotifyCallChanged("state", conf);
        return nullptr;  // no notification for shadow state change
      }
    }
  }

  call->state = state;
  return "state";
}

// Return the affected JS property name.
const char* TelephonyBackend::UpdateCallProperty(TelephonyCall* call,
    const char* key, GVariant* value) {

  if (!strcmp(key, "State")) {
    std::string state = g_variant_get_string(value, nullptr);
    return UpdateCallState(call, state);
  }

  if (!strcmp(key, "StartTime")) {
    call->start_time = g_variant_get_string(value, nullptr);
    return "startTime";
  }

  if (!strcmp(key, "LineIdentification")) {
    call->remote_party = g_variant_get_string(value, nullptr);
    return "remoteParty";
  }

  if (!strcmp(key, "Name")) {
    call->name = g_variant_get_string(value, nullptr);
    return "name";
  }

  if (!strcmp(key, "Multiparty")) {
    // this is the only notification from oFono for conference participants
    if (!g_variant_get_boolean(value) && call->conference) {
      // the call goes back to normal, split from conference
      call->conference = nullptr;
      call->state = call->saved_state.empty() ?
                        kCallStateActive : call->saved_state;
      call->saved_state.clear();
    }
    // when 'value' is true, createConference() has already updated this call
    // only need to trigger the state change notification from here
    return "state";
  }

  if (!strcmp(key, "Emergency")) {
    call->emergency = g_variant_get_boolean(value);
    return "emergency";
  }
}

bool TelephonyBackend::InitCall(TelephonyCall* call, const std::string& cid,
    GVariantIter* props) {

  if (cid.empty() || !props) {
    return false;
  }

  call->id = cid;
  call->protocol = "gsm";
  call->duration = 0;
  call->state = kCallStateInit;
  char* key = nullptr;
  GVariant* value = nullptr;
  while (g_variant_iter_next(props, "{sv}", &key, &value)) {
    UpdateCallProperty(call, key, value);
  }
  g_variant_unref(value);

  if (!call->service) {
    // extract service id from call id
    // format is <service_id>/voicecall<xx>
    size_t i = cid.find("voicecall");
    if (i <= 0)
      return 0;

    std::string sid = cid.substr(0, i - 1);
    call->service = FindService(sid);
    if (!call->service) {
      if (!QueryServices())
        return false;
      call->service = FindService(sid);
      if (!call->service)
        return false;
    }
  }

  return true;
}

void TelephonyBackend::UpdateCall(TelephonyCall* call) {
  GError* err = nullptr;
  GVariant* res = SyncDBusCall(IdToDbus(call->id).c_str(), kDbusOfonoVoiceCall,
                      "GetProperties", nullptr, &err);
  if (err) {
    LOG_ERR("UpdateCall: " << err->message);
    return;
  }

  GVariantIter* props = nullptr;
  g_variant_get(res, "(a{sv})", &props);
  InitCall(call, call->id, props);
  g_variant_iter_free(props);
  g_variant_unref(res);
}

void TelephonyBackend::LogServices() {
  std::cout << "\nServices:";
  for (int i = 0; i < services_.size(); i++)
    LogService(services_[i]);
}

void TelephonyBackend::LogCalls() {
  std::cout << "\nCalls:";
  for (int i = 0; i < calls_.size(); i++)
    LogCall(calls_[i]);
}


//////////////////////////////////////////////////////////////////////////////
// TelephonyBackend, service related notifications
///////////////////////////////////////////////////////////////////////////////

// In most cases there is only one or two services.
TelephonyService* TelephonyBackend::FindService(const std::string& id) {
  for (auto s : services_) {
    if (s && s->id == id) {
      return s;
    }
  }
  return nullptr;
}

void TelephonyBackend::OnModemAdded(const gchar* obj, GVariant* parameters) {
  LOG_DBG("OnModemAdded: " << std::string(obj));
  const char* path = nullptr;
  GVariantIter* props = nullptr;
  g_variant_get(parameters, "(oa{sv})", &path, &props);
  if (!path) {
    LOG_ERR("OnModemAdded: invalid object.");
    return;
  }

  std::string sid = IdFromDbus(path);
  RemoveService(FindService(sid));
  TelephonyService* service = new TelephonyService();
  InitService(service, sid, props);

  services_.push_back(service);
  NotifyServiceAdded(service);
  g_variant_iter_free(props);
}

void TelephonyBackend::OnModemChanged(const gchar* obj, GVariant* parameters) {
  LOG_DBG("OnModemChanged: " << std::string(obj));
  TelephonyService* service = FindService(IdFromDbus(obj));
  if (!service) {
    LOG_ERR("OnModemChanged: could not find service.");
    return;
  }

  const gchar* key = nullptr;
  GVariant* value = nullptr;
  g_variant_get(parameters, "(sv)", &key, &value);
  key = UpdateServiceProperty(service, key, value);
  if (key)
    NotifyServiceChanged(key, service);
  g_variant_unref(value);
}

void TelephonyBackend::OnModemRemoved(const gchar* obj, GVariant* parameters) {
  LOG_DBG("OnModemRemoved: " << std::string(obj));
  char* path = nullptr;
  g_variant_get(parameters, "(o)", &path);
  TelephonyService* service = FindService(IdFromDbus(path));

  if (!service) {
    LOG_ERR("OnModemRemoved: could not find service.");
    return;
  }

  NotifyServiceRemoved(service);
  RemoveService(service);
}

void TelephonyBackend::NotifyDefaultServiceChanged() {
  LOG_DBG("NotifyDefaultServiceChanged");
  picojson::object js, notif;
  notif["cmd"] = picojson::value("defaultServiceChanged");
  notif["service"] = default_service_ ?
                     picojson::value(ToJson(default_service_, js)) :
                     picojson::value();
  instance_->SendNotification(picojson::value(notif));
}

void TelephonyBackend::NotifyServiceAdded(TelephonyService* service) {
  LOG_DBG("NotifyServiceAdded");
  picojson::object js, notif;
  notif["cmd"] = picojson::value("serviceAdded");
  notif["service"] = service ?
                     picojson::value(ToJson(service, js)) :
                     picojson::value();
  instance_->SendNotification(picojson::value(notif));
}

void TelephonyBackend::NotifyServiceChanged(const char* key,
    TelephonyService* service) {
  LOG_DBG("NotifyServiceChanged: " << std::string(key));
  picojson::array changedProps;
  changedProps.push_back(picojson::value(key));
  picojson::object js, notif;
  notif["cmd"] = picojson::value("serviceChanged");
  notif["changedProperties"] = picojson::value(changedProps);
  notif["service"] = service ?
                     picojson::value(ToJson(service, js)) :
                     picojson::value();
  instance_->SendNotification(picojson::value(notif));
}

void TelephonyBackend::NotifyServiceRemoved(TelephonyService* service) {
  LOG_DBG("NotifyServiceRemoved");
  picojson::object js, notif;
  notif["cmd"] = picojson::value("serviceRemoved");
  notif["service"] = service ?
                     picojson::value(ToJson(service, js)) :
                     picojson::value();
  instance_->SendNotification(picojson::value(notif));
}

void TelephonyBackend::RemoveService(TelephonyService* service) {
  for (auto iter = services_.begin(); iter != services_.end();) {
    if (*iter == service) {
      delete *iter;
      iter = services_.erase(iter);
    }
  }
}

void TelephonyBackend::OnCallManagerChanged(const gchar* obj,
    GVariant* parameters) {
  // 'parameters' contains emergency numbers list a(s)
  LOG_DBG("OnCallManagerChanged");
  GVariantIter* prop_iter = nullptr;
  GVariant* value = nullptr;
  picojson::value::array results;
  g_variant_get(parameters, "(as)", &prop_iter);

  while (g_variant_iter_next(prop_iter, "s", &value)) {
    const gchar* num = g_variant_get_string(value, nullptr);
    results.push_back(picojson::value(num));
  }
  g_variant_iter_free(prop_iter);

  picojson::object notif;
  notif["cmd"] = picojson::value("emergencyNumbersChanged");
  notif["returnValue"] = picojson::value(results);
  instance_->SendNotification(picojson::value(notif));
}

//////////////////////////////////////////////////////////////////////////////
// TelephonyBackend, call related notifications
///////////////////////////////////////////////////////////////////////////////

// In most cases there is only one or two calls.
TelephonyCall* TelephonyBackend::FindCall(const std::string& id) {
  for (auto call : calls_) {
    if (call && (call->id == id))
      return call;
  }
  return nullptr;
}

void TelephonyBackend::OnCallAdded(const gchar* obj, GVariant* parameters) {
  std::string sid = IdFromDbus(obj);
  TelephonyService* service = FindService(sid);

  if (!service) {
    QueryServices();
    service = FindService(sid);
    if (!service) {
      LOG_ERR("OnCallAdded: invalid service id = " << sid);
      return;
    }
  }

  char* path = nullptr;
  GVariantIter* props = nullptr;
  g_variant_get(parameters, "(oa{sv})", &path, &props);
  if (!path) {
    LOG_ERR("OnCallAdded: invalid object path.");
    return;
  }

  std::string cid = IdFromDbus(path);
  TelephonyCall* call = FindCall(cid);
  if (!call) {
    call = new TelephonyCall(service);
    InitCall(call, cid, props);
    calls_.push_back(call);
  } else {
    LOG_ERR("OnCallAdded: duplicate call id found.");
    call->service = service;
    InitCall(call, cid, props);
  }

  NotifyCallAdded(call);
  LOG_DBG("OnCallAdded: " << call->id);
  g_variant_iter_free(props);
}

void TelephonyBackend::OnCallChanged(const gchar* obj, GVariant* parameters) {
  TelephonyCall* call = FindCall(IdFromDbus(obj));
  if (!call) {
    LOG_ERR("OnCallChanged: invalid object.");
    return;
  }

  const gchar* key = nullptr;
  GVariant* value = nullptr;
  g_variant_get(parameters, "(sv)", &key, &value);
  key = UpdateCallProperty(call, key, value);
  g_variant_unref(value);
  if (!key)
    return;

  NotifyCallChanged(key, call);
  LOG_DBG("OnCallChanged: " << call->id << "[" << key << "]");
}

void TelephonyBackend::OnCallDisconnectReason(const gchar* obj,
    GVariant* parameters) {
  LOG_DBG("OnCallDisconnectReason: " << std::string(obj));
  TelephonyCall* call = FindCall(IdFromDbus(obj));

  if (!call) {
    LOG_ERR("OnCallDisconnectReason: invalid object.");
    return;
  }

  gchar* result = nullptr;
  g_variant_get(parameters, "(s)", &result);
  call->state_reason = result;
  NotifyCallChanged("stateReason", call);
  g_free(result);
}

void TelephonyBackend::OnCallRemoved(const gchar* obj, GVariant* parameters) {
  LOG_DBG("OnCallRemoved: " << std::string(obj));
  char* path = nullptr;
  g_variant_get(parameters, "(o)", &path);
  TelephonyCall* call = FindCall(IdFromDbus(path));

  if (!call) {
    LOG_ERR("OnCallRemoved: could not find call.");
    return;
  }

  RemoveCall(call);
  // When the last call is removed, the conf call and all others are purged.
  if (calls_.empty()) {
    for (auto iter = removed_calls_.begin(); iter != removed_calls_.end();) {
      delete *iter;
      iter = removed_calls_.erase(iter);
    }
  }
}

void TelephonyBackend::CheckActiveCall(TelephonyCall* call) {
  if (!call) {  // call has been removed and need to find the next active call
    bool changed = false;
    for (auto c : calls_) {
      if (c && c->state == kCallStateActive &&
         (!c->conference || c->id == c->conference->id)) {
        active_call_ = c;
        changed = true;
      }
    }

    if (!changed)
      active_call_ = nullptr;
  } else if (active_call_ && call->id == active_call_->id &&
             call->state != kCallStateActive) {  // stop being active
    active_call_ = nullptr;
  } else if (active_call_ == call || call->state != kCallStateActive ||
      call->conference && call->conference->id != call->id) {
    // either no change, or not active, or participant in a conference
    // participants in a conference won't be active_call_, only the conf call
    return;
  } else {  // an active [conf] call different from the current active_call_
    active_call_ = call;
  }

  LOG_DBG("CheckActiveCall: active call set to " <<
      (active_call_ ? active_call_->id : "none."));
  picojson::object js, notif;
  notif["cmd"] = picojson::value("activeCallChanged");
  notif["call"] = active_call_ ?
                  picojson::value(ToJson(active_call_, js)) :
                  picojson::value();
  instance_->SendNotification(picojson::value(notif));
}

void TelephonyBackend::NotifyCallAdded(TelephonyCall* call) {
  LOG_DBG("NotifyCallAdded: " << call->id);
  picojson::object js, notif;
  notif["cmd"] = picojson::value("callAdded");
  notif["call"] = call ?
                  picojson::value(ToJson(call, js)) :
                  picojson::value();
  instance_->SendNotification(picojson::value(notif));
}

void TelephonyBackend::NotifyCallChanged(const char* key, TelephonyCall* call) {
  LOG_DBG("NotifyCallChanged: " << call->id << "[" << std::string(key) << "]");
  picojson::array changed_props;  // for compatibility with other backends
  changed_props.push_back(picojson::value(key));

  picojson::object js, notif;
  bool statechange = (key == "state");
  notif["cmd"] = statechange ?
                 picojson::value("callStateChanged") :
                 picojson::value("callChanged");
  notif["changedProperties"] = picojson::value(changed_props);
  notif["call"] = call ?
                  picojson::value(ToJson(call, js)) :
                  picojson::value();

  instance_->SendNotification(picojson::value(notif));

  if (statechange)
    CheckActiveCall(call);
}

void TelephonyBackend::NotifyCallRemoved(TelephonyCall* call) {
  LOG_DBG("NotifyCallRemoved: " << call->id);
  picojson::object js, notif;
  notif["cmd"] = picojson::value("callRemoved");
  notif["call"] = call ?
                  picojson::value(ToJson(call, js)) :
                  picojson::value();
  instance_->SendNotification(picojson::value(notif));
}

void TelephonyBackend::RemoveCall(TelephonyCall* call) {
  if (!call) {
    LOG_ERR(("RemoveCall: null call"));
    return;
  }

  LOG_DBG("RemoveCall: removing " << call->id);
  // remove from the global call list, and references from other calls
  for (auto iter = calls_.begin(); iter != calls_.end();) {
    TelephonyCall* c = *iter;
    if (c != call && c == call->conference) {
      RemoveParticipant(c, call);
      LOG_DBG("RemoveCall: removed from conference " << c->id);
      call->conference = nullptr;
    }

    if (c->id == call->id) {
      LOG_DBG("RemoveCall: removed from call list: " << c->id);
      iter = calls_.erase(iter);
      removed_calls_.push_back(call);
      NotifyCallRemoved(call);
      CheckActiveCall();
    } else {
      ++iter;
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
// TelephonyBackend, API calls
///////////////////////////////////////////////////////////////////////////////

void TelephonyBackend::GetServices(const picojson::value& msg) {
  LOG_DBG("GetServices");
  if (!QueryServices()) {
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  picojson::value::array results;
  for (int i = 0; i < services_.size(); i++) {
    if (services_[i]) {
      picojson::object js;
      ToJson(services_[i], js);
      results.push_back(picojson::value(js));
    }
  }

  instance_->SendSuccessReply(msg, picojson::value(results));
}

bool TelephonyBackend::QueryServices() {
  LOG_DBG("QueryServices");
  GError* err = nullptr;
  GVariant* res =
      SyncDBusCall("/", kDbusOfonoModemManager, "GetModems", nullptr, &err);

  if (err) {
    LOG_ERR("QueryServices: " << err->message);
    g_error_free(err);
    return false;
  }

  if (!res)
    return true;  // no services found, that's OK

  GVariantIter* modems;
  g_variant_get(res, "(a(oa{sv}))", &modems);
  char* path;
  GVariantIter* props;
  while (g_variant_iter_next(modems, "(oa{sv})", &path, &props)) {
    std::string sid = IdFromDbus(path);
    TelephonyService* service = FindService(sid);
    bool found = !!service;
    if (!service)
      service = new TelephonyService();
    InitService(service, sid, props);
    if (!found)
      services_.push_back(service);
  }
  g_variant_iter_free(props);
  g_variant_iter_free(modems);
  g_variant_unref(res);

  // set a default service
  if (!default_service_ && services_.size() > 0) {
    default_service_ = services_[0];
    NotifyDefaultServiceChanged();
  }

  return true;
}

void TelephonyBackend::SetServiceEnabled(const picojson::value& msg) {
  TelephonyService* service = FindService(msg.get("serviceId").to_str());
  if (!service) {
    LOG_ERR("TelephonyService not found.");
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (SetModemEnabled(service, msg.get("enabled").get<bool>()))
    instance_->SendSuccessReply(msg);
  else
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
}

bool TelephonyBackend::SetModemEnabled(TelephonyService* service,
    bool enabled) {
  LOG_DBG("SetModemEnabled: " << service->id << " : " << enabled);
  std::string path = IdToDbus(service->id);

  GError* err = nullptr;
  SyncDBusCall(path.c_str(), kDbusOfonoModemManager, "SetProperty",
      g_variant_new("(sv)", "Powered", g_variant_new("b", enabled)), &err);

  if (!err)
    SyncDBusCall(path.c_str(), kDbusOfonoModemManager, "SetProperty",
        g_variant_new("(sv)", "Online", g_variant_new("b", enabled)), &err);

  if (err) {
    LOG_ERR("SetModemEnabled: " << err->message);
    g_error_free(err);
    return false;
  }

  return true;
}

void TelephonyBackend::SetDefaultService(const picojson::value& msg) {
  TelephonyService* service = FindService(msg.get("serviceId").to_str());

  if (!service) {
    LOG_ERR("TelephonyService not found.");
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  LOG_DBG("SetDefaultService: " << service->id);
  if (!service->online)
    SetModemEnabled(service, true);

  default_service_ = service;
  //  No need to also notify, the Promise will confirm the action.
  instance_->SendSuccessReply(msg);
}

void TelephonyBackend::GetDefaultService(const picojson::value& msg) {
  std::string dsi = (default_service_ ? default_service_->id : "");
  instance_->SendSuccessReply(msg, picojson::value(dsi));
}

// Get the calls from all services.
void TelephonyBackend::GetCalls(const picojson::value& msg) {
  LOG_DBG("GetCalls");
  for (int i = 0; i < services_.size(); i++) {
    if (!services_[i]->online)
      continue;

    std::string path = IdToDbus(services_[i]->id);
    GError* err = nullptr;
    GVariant* res = SyncDBusCall(path.c_str(), kDbusOfonoVoiceCallManager,
                                 "GetCalls", nullptr, &err);

    if (err) {
      LOG_ERR("GetCalls: " << err->message);
      instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
      g_error_free(err);
      return;
    }

    if (!res) {
      LOG_ERR("Results from 'GetCalls' method is nullptr");
      instance_->SendSuccessReply(msg, picojson::value(""));
      return;
    }

    GVariantIter* calls;
    g_variant_get(res, "(a(oa{sv}))", &calls);

    char* obj;
    GVariantIter* props;
    picojson::value::array results;
    while (g_variant_iter_next(calls, "(oa{sv})", &obj, &props)) {
      std::string cid = IdFromDbus(obj);
      TelephonyCall* call = FindCall(cid);
      if (!call) {
        call = new TelephonyCall(services_[i]);
        InitCall(call, cid, props);
        calls_.push_back(call);
      }
    }

    for (auto c : calls_) {
      picojson::object js;
      ToJson(c, js);
      results.push_back(picojson::value(js));
    }

    instance_->SendSuccessReply(msg, picojson::value(results));
    g_variant_iter_free(calls);
    g_variant_unref(res);
  }
}

void TelephonyBackend::DialCall(const picojson::value& msg) {
  TelephonyService* service = FindService(msg.get("serviceId").to_str());

  if (!service) {
    // the app has called withouth querying services, expecting default service
    if (!default_service_) {
      if (!QueryServices() && !default_service_) {  // query affects default
        LOG_ERR("Dial: unable to find telephony services.");
        instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
        return;
      }
    }
    service = default_service_;
  }

  std::string remote = msg.get("remoteParty").to_str();
  if (!CheckRemoteParty(remote)) {
    LOG_ERR("Dial: invalid remote party " << remote);
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  LOG_DBG("Dial " << remote);
  GError* err = nullptr;
  std::string sid = IdToDbus(service->id);
  GVariant* res = SyncDBusCall(sid.c_str(),
                               kDbusOfonoVoiceCallManager,
                               "Dial",
                               g_variant_new("(ss)",
                                   remote.c_str(),
                                   msg.get("hideCallerId").get<bool>() ?
                                       "enabled" : ""),
                               &err);

  if (err) {
    LOG_ERR("Dial: " << err->message);
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    g_error_free(err);
    return;
  }

  if (!res) {
    LOG_ERR("Dial: nullptr call object.");
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  char* obj = nullptr;
  g_variant_get(res, "(o)", &obj);
  std::string cid = IdFromDbus(obj);
  TelephonyCall* call = FindCall(cid);

  // The call should already be there, added by OnCallAdded
  if (!call) {
    call = new TelephonyCall(service);
    call->id = cid;
    UpdateCall(call);
  }

  picojson::object js;
  ToJson(call, js);
  instance_->SendSuccessReply(msg, picojson::value(js));
  g_variant_unref(res);
}

void TelephonyBackend::DeflectCall(const picojson::value& msg) {
  std::string cid = msg.get("callId").to_str();
  TelephonyCall* call = FindCall(cid);

  if (!call) {
    LOG_ERR("Accept: invalid call id " << cid);
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  std::string remote = msg.get("remoteParty").to_str();
  LOG_DBG("Deflect " << remote);

  if (!CheckRemoteParty(remote)) {
    LOG_ERR("Deflect: invalid remote party " << remote);
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (call->state != kCallStateIncoming &&
      call->state != kCallStateWaiting) {
    LOG_ERR("Deflect: invalid call state: " << call->state);
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  GError* err = nullptr;
  SyncDBusCall(IdToDbus(call->id).c_str(), kDbusOfonoVoiceCall, "Deflect",
               g_variant_new("s", remote.c_str()), &err);

  if (err) {
    LOG_ERR("Deflect: " << err->message);
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    g_error_free(err);
    return;
  }

  instance_->SendSuccessReply(msg);
}

void TelephonyBackend::AcceptCall(const picojson::value& msg) {
  std::string cid = msg.get("callId").to_str();
  TelephonyCall* call = FindCall(cid);

  if (!call) {
    LOG_ERR("Accept: invalid call id " << cid);
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  LOG_DBG("Accept " << call->id);
  GError* err = nullptr;
  std::string path = IdToDbus(call->id);
  SyncDBusCall(path.c_str(), kDbusOfonoVoiceCall, "Answer", nullptr, &err);

  if (err) {
    LOG_ERR("Accept: " << err->message);
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    g_error_free(err);
    return;
  }

  instance_->SendSuccessReply(msg);
}

bool TelephonyBackend::HangupCall(std::string& call_id) {
  LOG_DBG("Disconnect " << call_id);
  GError* err = nullptr;
  std::string path = IdToDbus(call_id);
  SyncDBusCall(path.c_str(), kDbusOfonoVoiceCall, "Hangup", nullptr, &err);

  if (err) {
    LOG_ERR("Disconnect: " << err->message);
    g_error_free(err);
    return false;
  }

  return true;
}

void TelephonyBackend::HangupAllCalls(std::string& service_id) {
  LOG_DBG("Disconnect all calls on service " << service_id);
  GError* err = nullptr;
  std::string path = IdToDbus(service_id);
  SyncDBusCall(path.c_str(), kDbusOfonoVoiceCallManager, "HangupAll", nullptr,
      &err);

  if (err) {
    LOG_ERR("Disconnect all: " << err->message);
    g_error_free(err);
  }
}

void TelephonyBackend::DisconnectCall(const picojson::value& msg) {
  std::string cid = msg.get("callId").to_str();
  TelephonyCall* call = FindCall(cid);

  if (!call) {
    LOG_ERR("Disconnect: invalid call id " << cid);
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (!call->conference) {
    if (HangupCall(cid))
      instance_->SendSuccessReply(msg);
    else
      instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  UpdateDuration(call);
  call->state = kCallStateDisconnected;
  call->state_reason = "local";

  bool failed = false;
  for (auto c : call->participants) {
    if (!HangupCall(c->id))  // causes call state updates
      failed = true;
  }

  if (failed) {
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    HangupAllCalls(call->service->id);  // avoid spurious calls
    return;
  }

  RemoveCall(call);
  instance_->SendSuccessReply(msg);
}

void TelephonyBackend::HoldCall(const picojson::value& msg) {
  std::string cid = msg.get("callId").to_str();
  TelephonyCall* call = FindCall(cid);

  if (!call || !call->service) {
    LOG_ERR("Hold: invalid call id " << cid);
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  LOG_DBG("Hold " << call->id);
  // oFono has SwapCalls, and HoldAndAnswer. Find out which one to use.
  const gchar* method = nullptr;
  // cannot hold held, dialing, alerting, and disconnected calls
  if (call->state != kCallStateIncoming &&
      call->state != kCallStateActive &&
      call->state != kCallStateWaiting) {
    LOG_ERR("Hold: invalid call state: " << call->state);
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (call->state == kCallStateActive) {
    // check if there are waiting calls
    for (int i = 0; i < calls_.size(); i++) {
      if (calls_[i]->state == kCallStateWaiting) {
        method = "HoldAndAnswer";
        break;
      }
    }
  }

  if (!method)
    method = "SwapCalls";

  GError* err = nullptr;
  std::string path = IdToDbus(call->service->id);
  SyncDBusCall(path.c_str(), kDbusOfonoVoiceCallManager, method, nullptr, &err);

  if (err) {
    LOG_ERR("Hold: " << err->message);
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    g_error_free(err);
    return;
  }

  instance_->SendSuccessReply(msg);
}

void TelephonyBackend::ResumeCall(const picojson::value& msg) {
  std::string cid = msg.get("callId").to_str();
  TelephonyCall* call = FindCall(cid);

  if (!call) {
    LOG_ERR("Resume: invalid call id");
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  LOG_DBG("Resume " << call->id);
  if (call->state != kCallStateHeld) {
    LOG_ERR("Resume: invalid call state '" << call->state);
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  GError* err = nullptr;
  std::string path = IdToDbus(call->service->id);
  SyncDBusCall(path.c_str(), kDbusOfonoVoiceCallManager, "SwapCalls",
               nullptr, &err);

  if (err) {
    LOG_ERR("Resume: " << err->message);
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    g_error_free(err);
    return;
  }

  instance_->SendSuccessReply(msg);
}

void TelephonyBackend::TransferCall(const picojson::value& msg) {
  if (!default_service_) {
    LOG_ERR("Transfer: no default service");
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  LOG_DBG("Transfer: " << default_service_->id);
  GError* err = nullptr;
  std::string path = IdToDbus(default_service_->id);
  SyncDBusCall(path.c_str(), kDbusOfonoVoiceCallManager, "Transfer",
               nullptr, &err);

  if (err) {
    LOG_ERR("Transfer: " << err->message);
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    g_error_free(err);
    return;
  }

  instance_->SendSuccessReply(msg);
}

void TelephonyBackend::CreateConference(const picojson::value& msg) {
  static uint64_t conference_id_;
  TelephonyService* service = active_call_?
                              active_call_->service :
                              default_service_;

  if (!service) {
    LOG_ERR("CreateConference: unable to find the telephony service.");
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  LOG_DBG("CreateConference: " << service->id);
  GError* err = nullptr;
  std::string modem_path = IdToDbus(service->id);
  GVariant* res = SyncDBusCall(modem_path.c_str(), kDbusOfonoVoiceCallManager,
                               "CreateMultiparty", nullptr, &err);

  const time_t start_time = time(nullptr);
  if (err) {
    LOG_ERR("CreateConference: " << err->message);
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    g_error_free(err);
    return;
  }

  TelephonyCall* conf = new TelephonyCall(service);

  // oFono returns the new array with the participating call object paths.
  // Conference control calls should not be logged, but play safe for clients
  // and record start time for conference.
  struct tm tm_s = {0};
  char timebuf[40];
  strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%S%z",
      localtime_r(&start_time, &tm_s));
  conf->start_time = std::string(timebuf);

  conference_id_++;
  std::stringstream ss;
  ss << "conference-" << conference_id_;
  conf->id = ss.str();

  conf->conference = conf;
  conf->duration = 0;
  conf->state = kCallStateActive;

  GVariantIter* calls;
  g_variant_get(res, "(ao)", &calls);

  char* obj = nullptr;
  while (g_variant_iter_next(calls, "o", &obj)) {
    std::string call_id = IdFromDbus(obj);
    TelephonyCall* call = FindCall(call_id);

    if (!call) {  // unlikely
      LOG_ERR("CreateConference: could not find call with id" << call_id);
      instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
      g_variant_iter_free(calls);
      RemoveCall(conf);  // will not cause notification, just cleanup
      return;
    }

    call->conference = conf;
    call->saved_state = call->state;
    call->state = kCallStateConference;  // hangup/hold/split can change state
    conf->participants.push_back(call);
  }
  g_variant_iter_free(calls);

  calls_.push_back(conf);
  LOG_DBG("Conference call added: " << conf->id);

  picojson::object js;
  ToJson(conf, js);
  // No need to NotifyCallAdded(conf), since the Promise returns it.
  instance_->SendSuccessReply(msg, picojson::value(js));
  CheckActiveCall(conf);
  // no need to send notification about the changed participating calls
  // other than a state change event;
  // oFono will send CallChanged event with 'Multiparty' property updated
  // to each participating call object, handle that in UpdateCallProperty
  for (auto c : conf->participants)
    NotifyCallChanged("state", c);
}

void TelephonyBackend::GetConferenceParticipants(const picojson::value& msg) {
  // This can also be implemented purely in JavaScript.
  TelephonyCall* conf = FindCall(msg.get("conferenceId").to_str());

  if (!conf || !conf->conference) {
    LOG_ERR("GetParticipants: unable to find the conference call.");
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  LOG_DBG("GetParticipants: " << conf->id);
  picojson::value::array array;
  for (int i = 0; i < conf->participants.size(); i++) {
    TelephonyCall* call = conf->participants[i];
    if (call)
      array.push_back(picojson::value(call->id));
  }

  instance_->SendSuccessReply(msg, picojson::value(array));
}

void TelephonyBackend::SplitCall(const picojson::value& msg) {
  std::string cid = msg.get("callId").to_str();
  TelephonyCall* call = FindCall(cid);

  if (!call) {
    LOG_ERR("Split: invalid call id " << cid);
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  if (!call->conference) {
    LOG_ERR("Split: not in a multiparty call.");
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  TelephonyCall* conf = call->conference;
  if (!conf) {
    LOG_ERR("Split: invalid conference id.");
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  LOG_DBG("Split: " << call->id << " from: " << conf->id);
  GError* err = nullptr;
  std::string path = IdToDbus(call->service->id);
  GVariant* res = SyncDBusCall(path.c_str(), kDbusOfonoVoiceCallManager,
      "PrivateChat", g_variant_new("s", call->id.c_str()), &err);

  if (err) {
    LOG_ERR("Split: " << err->message);
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    g_error_free(err);
    return;
  }

  // oFono will notify the state changes to each (now held) participating call
  // oFono returns the new array with the rest of calls, ignore it
  RemoveParticipant(conf, call);
  instance_->SendSuccessReply(msg);
  g_variant_unref(res);
}

void TelephonyBackend::RemoveParticipant(TelephonyCall* conf,
                                         TelephonyCall* call) {
  if (!conf || !call)
    return;

  int size = conf->participants.size();
  for (auto iter = conf->participants.begin();
       iter != conf->participants.end();) {
    if ((*iter)->id == call->id) {  // don't delete, just erase
      iter = conf->participants.erase(iter);
      break;
    } else {
      ++iter;
    }
  }

  // If there have been only 2 calls in the conference before the split,
  // then remove the conf call object since we'll have two normal calls.
  if (size == 2) {
    RemoveCall(conf);
  }
}

void TelephonyBackend::SendTones(const picojson::value& msg) {
  // serviceId, tones
  std::string sid = msg.get("serviceId").to_str();
  TelephonyService* service = !sid.empty() ? FindService(sid) : nullptr;

  if (!service && !default_service_) {
    LOG_ERR("SendTones: unable to find telephony service.");
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  service = default_service_;

  std::string tones = msg.get("tones").to_str();
  if (tones.empty()) {
    LOG_ERR("SendTones: empty sequence.");
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  LOG_DBG("SendTones: " << tones << " to: " << sid);
  GError* err = nullptr;
  std::string path = IdToDbus(default_service_->id);
  SyncDBusCall(path.c_str(), kDbusOfonoVoiceCallManager, "SendTones",
               g_variant_new("(s)", tones.c_str()), &err);

  if (err) {
    LOG_ERR("SendTones: " << err->message);
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    g_error_free(err);
    return;
  }

  instance_->SendSuccessReply(msg);
}

void TelephonyBackend::StartTone(const picojson::value& msg) {
  instance_->SendErrorReply(msg, NOT_SUPPORTED_ERR);
}

void TelephonyBackend::StopTone(const picojson::value& msg) {
  instance_->SendErrorReply(msg, NOT_SUPPORTED_ERR);
}

void TelephonyBackend::GetEmergencyNumbers(const picojson::value& msg) {
  if (!default_service_ && (!QueryServices() || !default_service_)) {
    LOG_ERR("GetEmergencyNumbers: no service");
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    return;
  }

  GError* err = nullptr;
  std::string path = IdToDbus(default_service_->id);
  GVariant* res = SyncDBusCall(path.c_str(), kDbusOfonoVoiceCallManager,
                               "GetProperties", nullptr, &err);

  if (err) {
    LOG_ERR("GetEmergencyNumbers: " << err->message);
    instance_->SendErrorReply(msg, NO_MODIFICATION_ALLOWED_ERR);
    g_error_free(err);
    return;
  }

  if (!res) {
    LOG_ERR("Results from 'GetEmergencyNumbers' method is nullptr");
    instance_->SendSuccessReply(msg, picojson::value(""));
    return;
  }

  GVariantIter* prop_iter = nullptr;
  g_variant_get(res, "(a{sv})", &prop_iter);

  gchar* key = nullptr;
  GVariant* value = nullptr;
  picojson::value::array results;
  while (g_variant_iter_next(prop_iter, "{sv}", &key, &value)) {
    if (!strcmp(key, "EmergencyNumbers")) {
      gchar* number = nullptr;
      GVariantIter* num_iter = nullptr;
      g_variant_get(value, "as", &num_iter);
      while (g_variant_iter_loop(num_iter, "s", &number)) {
        results.push_back(picojson::value(number));
      }
      g_variant_iter_free(num_iter);
      g_variant_unref(value);
    }
  }

  instance_->SendSuccessReply(msg, picojson::value(results));
  g_variant_iter_free(prop_iter);
  g_variant_unref(res);
}

void TelephonyBackend::EmergencyDial(const picojson::value& msg) {
  // with ofono, need to make a regular call with the emergency number
  instance_->SendErrorReply(msg, NOT_SUPPORTED_ERR);
}

bool TelephonyBackend::CheckRemoteParty(const std::string& address) {
  // using the regexp proposed in oFono ./doc/voicecallmanager-api.txt
  // but it doesn't work with g++; leaving here for reference
  // std::regex phoneNumberRegex("[+]?[0-9*#]{1,80}");
  // return std::regex_match(address, phoneNumberRegex);
  char* p = const_cast<char*>(address.c_str());
  int len = 80;
  if (!p || !*p)
    return false;
  if (*p == '+')
    p++;
  for (; *p && --len; p++) {
    if ((*p < '0' || *p > '9') && *p != '*' && *p != '#')
      return false;
  }
  return (len > 0);
}

std::string TelephonyBackend::IdFromDbus(const char* id) {
  std::string res(id);
  for (int i = 0; i < res.size(); i++) {
    if (res[i] == '/')
      res[i] = '|';
  }
  return res;
}

std::string TelephonyBackend::IdToDbus(std::string& id) {
  std::string res = id;
  for (int i = 0; i < res.size(); i++) {
    if (res[i] == '|')
      res[i] = '/';
  }
  return res;
}
