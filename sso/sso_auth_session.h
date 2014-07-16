// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SSO_SSO_AUTH_SESSION_H_
#define SSO_SSO_AUTH_SESSION_H_

#include <glib.h>
#include <glib-object.h>
#include <libgsignon-glib/signon-auth-session.h>
#include <libgsignon-glib/signon-utils.h>

#include <memory>
#include <string>

#include "common/utils.h"
#include "sso/sso_utils.h"

namespace common {

class Instance;

}  // namespace common

typedef enum {
  SESSION_STATE_NOT_STARTED,
  SESSION_STATE_RESOLVING_HOST,
  SESSION_STATE_CONNECTING,
  SESSION_STATE_SENDING_DATA,
  SESSION_STATE_WAITING_REPLY,
  SESSION_STATE_USER_PENDING,
  SESSION_STATE_UI_REFRESHING,
  SESSION_STATE_PROCESS_PENDING,
  SESSION_STATE_STARTED,
  SESSION_STATE_PROCESS_CANCELLING,
  SESSION_STATE_PROCESS_DONE,
  SESSION_STATE_CUSTOM
} SessionState;

class SessionData {
 public:
  SessionData();
  virtual ~SessionData();

  void FromJSON(const picojson::value& value);
  picojson::value ToJSON() const;

  GVariant* ToVariant() const;
  void FromVariant(GVariant* vdata);

 private:
  GHashTable* data_;

  bool IsSupportedType(GType type) const;
  void ToJSONType(picojson::object& object, const gchar* key,
      GValue* value) const;
  void InsertStringData(const std::string& key, const picojson::value& obj);
  void InsertData(const std::string& key, const picojson::value& obj);
  bool InsertKnownData(const std::string& key, const picojson::value& obj);
};

class SSOAuthSession {
 public:
  SSOAuthSession(common::Instance* instance, SignonAuthSession* session,
      const std::string& method, int jsid);
  virtual ~SSOAuthSession();

  int jsid() const { return jsid_; }
  picojson::value ToJSON() const;

  void HandleQueryAvailableMechanisms(const picojson::value& value);
  void HandleChallenge(const picojson::value& value);
  void HandleCancel(const picojson::value& value);

 private:
  static void QueryAvailableMechanismsCb(SignonAuthSession* self,
      gchar** mechanisms, const GError* error, gpointer user_data);
  static void ChallengeCb(GObject* source_object, GAsyncResult* res,
      gpointer user_data);
  static void StateChangeCb(SignonAuthSession* self, gint state, gchar* message,
      gpointer user_data);

  common::Instance* instance_;
  SignonAuthSession* session_;
  std::string method_;
  SessionState session_state_;
  int jsid_;

  DISALLOW_COPY_AND_ASSIGN(SSOAuthSession);
};
typedef std::shared_ptr<SSOAuthSession> SSOAuthSessionPtr;

#endif  // SSO_SSO_AUTH_SESSION_H_
