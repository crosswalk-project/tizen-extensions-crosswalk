// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TELEPHONY_TELEPHONY_BACKEND_OFONO_H_
#define TELEPHONY_TELEPHONY_BACKEND_OFONO_H_

#include <gio/gio.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "common/extension.h"
#include "common/picojson.h"
#include "common/utils.h"
#include "telephony/telephony_instance.h"
#include "tizen/tizen.h"

#define DECL_DBUS_SIGNAL_HANDLER(name) \
  static void name(GDBusConnection* connection, \
                   const gchar* sender_name, \
                   const gchar* object_path, \
                   const gchar* interface_name, \
                   const gchar* signal_name, \
                   GVariant* parameters, \
                   gpointer user_data)

#define DECL_SIGNAL_HANDLER(name) \
  void name(const gchar* object_path, GVariant* parameters)

#define _TEL_DBUS_ASYNC 0  // not much gain for async mode in a worker process

#if _TEL_DBUS_ASYNC
  #define DECL_API_METHOD(name) \
    void name(const picojson::value& msg); \
    static void name ## Finished(GObject* source_object, \
                   GAsyncResult* res, \
                   gpointer user_data)
#else
  #define DECL_API_METHOD(name) \
    void name(const picojson::value& msg)
#endif

/*
 * Implementation notes.
 *
 * Telephony services map to ofono Modem objects (plus subscriber identities).
 * The type of Modem objects is "hfp" for phones connected via Bluetooth HFP,
 * and "hardware" for internal modem.
 *
 * This extension identifies services/calls by modem/call DBUS object path.
 * The JS shim would need to generate an opaque id out of that and maintain 1:1
 * association, i.e. avoid having service objects with different serviceId but
 * referring to the same modem path. This implementation exposes the object
 * paths, which in turn exposes Bluetooth addresses to the
 * client in case of HFP connection. So telephony is in the same privacy class
 * as Bluetooth functionality, from fingerprinting point of view.
 *
 * Since oFono handles conference calls as arrays of call objects, with no
 * abstraction for a dedicated conference call object, this implementation
 * emulates it, and manages call id's for conference calls in a separate
 * namespace, i.e. they don't correspond to call object paths. Also,
 * oFono keeps all participating calls in 'active' state and updates the
 * 'Multiparty' property. When participating calls are disconnected until only
 * 2 calls remain, the conference call may or may not ne downgraded to normal
 * calls. This implementation will handle them as separate normal calls and
 * remove the conference call object in this case. When oFono changes one
 * participating call's state, the conf call object is checked/updated.
 * This is one issue complicating the implementation.
 * The other is managing "the active call", which from oFono perspective,
 * is not defined. The telephony protocols define which call is active, oFono
 * just follows up the states. The W3C spec requires the notion of active call,
 * as the one connected to audio resources.
 * oFono doesn't provide that information. In this implementation, the active
 * call is the last (normal or conference) call which became active.
 *
 * Since oFono does not handle multiple modems simultaneously, only one modem
 * is kept powered on at any given time, so API calls using a different service
 * than the default one will first try to power up the other, corresponding
 * modem, and then switch back to the default.
 */

class TelephonyBackend;
class TelephonyService;

struct TelephonyCall {
  explicit TelephonyCall(TelephonyService* s = NULL)
    : service(s), conference(NULL), duration(0) {}
  ~TelephonyCall() {}
  TelephonyService* service;  // the service owning the call
  std::string id;  // call object path or conference call id
  std::string remote_party;  // same as line_id in oFono (CLIP/COLP)
  std::string state;
  std::string state_reason;  // mainly for disconnect reason
  std::string saved_state;  // to maintain state during conf call
  std::string name;
  std::string start_time;
  double duration;  // computed, in milliseconds
  bool emergency;
  std::string protocol;  // duplicated from TelephonyService
  TelephonyCall* conference;  // the conference this call is part of
  std::vector<TelephonyCall*> participants;
  // other information from oFono is not handled in this implementation
};

struct TelephonyService {
  std::string id;  // modem object path
  std::string name;  // displayable name
  std::string type;  // "hfp" or "hw"
  std::string model;
  std::string revision;
  std::string serial;
  std::string protocol;  // "gsm" or "cdma"
  std::string provider;
  bool emergency;
  bool powered;
  bool online;
  bool lockdown;
  // for future: std::vector<TelephonyCall*> calls;
};

class TelephonyBackend {
 public:
  explicit TelephonyBackend(TelephonyInstance* instance);
  ~TelephonyBackend();

  // implementing the API methods
  DECL_API_METHOD(GetServices);
  DECL_API_METHOD(SetServiceEnabled);
  DECL_API_METHOD(SetDefaultService);
  DECL_API_METHOD(GetDefaultService);
  DECL_API_METHOD(GetCalls);
  DECL_API_METHOD(DialCall);
  DECL_API_METHOD(DeflectCall);
  DECL_API_METHOD(AcceptCall);
  DECL_API_METHOD(DisconnectCall);
  DECL_API_METHOD(HoldCall);
  DECL_API_METHOD(ResumeCall);
  DECL_API_METHOD(TransferCall);
  DECL_API_METHOD(CreateConference);
  DECL_API_METHOD(GetConferenceParticipants);
  DECL_API_METHOD(SplitCall);
  DECL_API_METHOD(SendTones);
  DECL_API_METHOD(StartTone);
  DECL_API_METHOD(StopTone);
  DECL_API_METHOD(GetEmergencyNumbers);
  DECL_API_METHOD(EmergencyDial);

  bool HangupCall(std::string& call_id);
  void HangupAllCalls(std::string& service_id);

  GVariant* SyncDBusCall(const gchar* object, const gchar* interface,
      const gchar* method, GVariant* parameters, GError **error);

  void AsyncDBusCall(const gchar* object, const gchar* interface,
    const gchar* method, GVariant* parameters, GAsyncReadyCallback callback,
    gpointer user_data);

  void SetDBusSignalHandler(const gchar* iface, const gchar* object,
    const gchar* name, GDBusSignalCallback cb, void* user_data);

  void EnableSignalHandlers();
  void DisableSignalHandlers();

 private:
  // handling DBUS signals from oFono
  DECL_DBUS_SIGNAL_HANDLER(SignalHandler);
  DECL_SIGNAL_HANDLER(OnModemAdded);
  DECL_SIGNAL_HANDLER(OnModemRemoved);
  DECL_SIGNAL_HANDLER(OnModemChanged);
  DECL_SIGNAL_HANDLER(OnCallAdded);
  DECL_SIGNAL_HANDLER(OnCallRemoved);
  DECL_SIGNAL_HANDLER(OnCallManagerChanged);
  DECL_SIGNAL_HANDLER(OnCallChanged);
  DECL_SIGNAL_HANDLER(OnCallDisconnectReason);

  void NotifyDefaultServiceChanged();
  void NotifyServiceAdded(TelephonyService* service);
  void NotifyServiceChanged(const char* key, TelephonyService* service);
  void NotifyServiceRemoved(TelephonyService* service);
  void NotifyCallAdded(TelephonyCall* call);
  void NotifyCallChanged(const char* key, TelephonyCall* call);
  void NotifyCallRemoved(TelephonyCall* call);

  bool QueryServices();
  void RemoveService(TelephonyService* service);
  bool SetModemEnabled(TelephonyService* service, bool on);
  void RemoveCall(TelephonyCall* call);
  bool CheckRemoteParty(const std::string& address);
  void RemoveParticipant(TelephonyCall* conf, TelephonyCall* call);

  std::string IdFromDbus(const char* id);
  std::string IdToDbus(std::string& id);

  TelephonyService* FindService(const std::string& id);
  TelephonyCall* FindCall(const std::string& id);

  picojson::object& ToJson(TelephonyCall* call, picojson::object& js);
  picojson::object& ToJson(TelephonyService* s, picojson::object& js);

  const char* UpdateCallProperty(TelephonyCall* call, const char* key,
      GVariant* value);
  const char* UpdateCallState(TelephonyCall* call, std::string& state);
  const char* UpdateServiceProperty(TelephonyService* s, const char* key,
      GVariant* value);
  bool InitCall(TelephonyCall* call, const std::string& path,
      GVariantIter* props);
  bool InitService(TelephonyService* s, const std::string& path,
      GVariantIter* props);
  void UpdateCall(TelephonyCall* call);
  void UpdateService(TelephonyService* s);
  void UpdateDuration(TelephonyCall* call);
  void LogCall(TelephonyCall* call);
  void LogService(TelephonyService* s);
  void LogCalls();
  void LogServices();

  void CheckActiveCall(TelephonyCall* call = NULL);

 private:
  TelephonyInstance* instance_;
  std::vector<guint> dbus_listeners_;
  std::vector<TelephonyService*> services_;
  TelephonyService* default_service_;
  TelephonyCall* active_call_;  // the one which has audio
  std::vector<TelephonyCall*> calls_;
  std::vector<TelephonyCall*> removed_calls_;
  GCancellable* cancellable_;

  DISALLOW_COPY_AND_ASSIGN(TelephonyBackend);
};

#endif  // TELEPHONY_TELEPHONY_BACKEND_OFONO_H_
