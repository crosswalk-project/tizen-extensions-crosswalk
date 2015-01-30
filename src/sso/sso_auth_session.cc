// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sso/sso_auth_session.h"

#include <json-glib/json-glib.h>

#include "sso/sso_async_op.h"
#include "sso/sso_instance.h"
#include "sso/sso_utils.h"

void SSOAuthSession::QueryAvailableMechanismsCb(SignonAuthSession* self,
    gchar** mechanisms, const GError* error, gpointer user_data) {
  SSOAsyncOp* async_op = reinterpret_cast<SSOAsyncOp*>(user_data);
  SSOAuthSession* session = reinterpret_cast<SSOAuthSession*>(
      async_op->user_data());
  if (error) {
    async_op->PostError(error->message, session->jsid_);
    delete async_op;
    return;
  }

  picojson::value::object resp;
  resp["mechanisms"] = picojson::value(SSOUtils::FromStringArrayToJSONArray(
      reinterpret_cast<const gchar* const*>(mechanisms)));
  g_strfreev(mechanisms);

  async_op->PostResult(picojson::value(resp), session->jsid_);
  delete async_op;
}

void SSOAuthSession::ChallengeCb(GObject* source_object, GAsyncResult* res,
    gpointer user_data) {
  SSOAsyncOp* async_op = reinterpret_cast<SSOAsyncOp*>(user_data);
  SSOAuthSession* session = reinterpret_cast<SSOAuthSession*>(
      async_op->user_data());
  GError* error = NULL;
  SignonAuthSession* auth_session = SIGNON_AUTH_SESSION(source_object);
  GVariant* v_reply = signon_auth_session_process_finish(auth_session, res,
      &error);
  if (error) {
    async_op->PostError(error->message, session->jsid_);
    delete async_op;
    g_error_free(error);
    return;
  }

  gchar* data = json_gvariant_serialize_data(v_reply, NULL);
  g_variant_unref(v_reply);
  picojson::value::object resp;
  resp["sessionData"] = picojson::value(std::string(data));
  g_free(data);
  async_op->PostResult(picojson::value(resp), session->jsid_);
  delete async_op;
}

void SSOAuthSession::StateChangeCb(SignonAuthSession* self, gint state,
    gchar* message, gpointer user_data) {
  SSOAuthSession* session = reinterpret_cast<SSOAuthSession*>(user_data);
  SSOAsyncOp op(session->instance_, 0, session);
  session->session_state_ = static_cast<SessionState>(state);
  picojson::value::object info;
  info["info"] = picojson::value("sessionStateChanged");
  if (message)
    info["message"] = picojson::value(std::string(message));
  info["object"] = session->ToJSON();
  op.PostInfo(picojson::value(info), session->jsid_);
}

SSOAuthSession::SSOAuthSession(common::Instance* instance,
    SignonAuthSession* session, const std::string& method, int jsid)
    : instance_(instance), session_(session), method_(method),
      session_state_(SESSION_STATE_NOT_STARTED), jsid_(jsid) {
  if (session_) {
    g_signal_connect(session_, "state-changed",
        G_CALLBACK(&SSOAuthSession::StateChangeCb), this);
  }
}

SSOAuthSession::~SSOAuthSession() {
  if (session_) {
    g_object_unref(session_);
    session_ = NULL;
  }
}

void SSOAuthSession::HandleQueryAvailableMechanisms(
    const picojson::value& value) {
  gchar** mechs = SSOUtils::FromJSONArrayToStringArray(
      value.get("wantedMechanisms").get<picojson::array>());
  signon_auth_session_query_available_mechanisms(session_,
      const_cast<const gchar**>(mechs),
      &SSOAuthSession::QueryAvailableMechanismsCb,
      new SSOAsyncOp(instance_, new picojson::value(value), this));
  g_strfreev(mechs);
}

void SSOAuthSession::HandleChallenge(const picojson::value& value) {
  GVariant* vsess = json_gvariant_deserialize_data(
      value.get("sessionData").serialize().c_str(), -1, NULL, NULL);
  vsess = g_variant_ref_sink(vsess);
  signon_auth_session_process_async(session_, vsess,
      value.get("mechanism").to_str().c_str(), 0, &SSOAuthSession::ChallengeCb,
      new SSOAsyncOp(instance_, new picojson::value(value), this));
}

void SSOAuthSession::HandleCancel(const picojson::value& value) {
  SSOAsyncOp op(instance_, new picojson::value(value), this);
  signon_auth_session_cancel(session_);
  op.PostResult(jsid());
}

picojson::value SSOAuthSession::ToJSON() const {
  picojson::value::object object;
  object["method"] = picojson::value(method_);
  object["sessionJSId"] = picojson::value(static_cast<double>(jsid_));
  object["sessionState"] = picojson::value(static_cast<double>(session_state_));
  return picojson::value(object);
}
