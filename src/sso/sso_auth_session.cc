// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sso/sso_auth_session.h"

#include "sso/sso_async_op.h"
#include "sso/sso_instance.h"
#include "sso/sso_utils.h"

SessionData::SessionData()
    : data_(0) {
  data_ = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
      signon_gvalue_free);
}

SessionData::~SessionData() {
  if (data_) {
    g_hash_table_unref(data_);
    data_ = NULL;
  }
}

bool SessionData::IsSupportedType(GType type) const {
  return (signon_gtype_to_variant_type(type) != 0);
}

void SessionData::ToJSONType(picojson::object& object, const gchar* key,
    GValue* value) const {
  GType type = G_VALUE_TYPE(value);
  if (!IsSupportedType(type))
    return;

  switch (type) {
    case G_TYPE_STRING: {
      std::string str = g_value_get_string(value);
      if (!str.empty())
        object[std::string(key)] = picojson::value(str);
      break;
    }
    case G_TYPE_BOOLEAN: {
      object[std::string(key)] = picojson::value(
          static_cast<bool>(g_value_get_boolean(value)));
      break;
    }
    case G_TYPE_UCHAR: {
      object[std::string(key)] = picojson::value(
          static_cast<double>(g_value_get_uchar(value)));
      break;
    }
    case G_TYPE_INT: {
      object[std::string(key)] = picojson::value(
          static_cast<double>(g_value_get_int(value)));
      break;
    }
    case G_TYPE_UINT: {
      object[std::string(key)] = picojson::value(
          static_cast<double>(g_value_get_uint(value)));
      break;
    }
    case G_TYPE_INT64: {
      object[std::string(key)] = picojson::value(
          static_cast<double>(g_value_get_int64(value)));
      break;
    }
    case G_TYPE_UINT64: {
      object[std::string(key)] = picojson::value(
          static_cast<double>(g_value_get_uint64(value)));
      break;
    }
    case G_TYPE_DOUBLE: {
      object[std::string(key)] = picojson::value(g_value_get_double(value));
      break;
    }
    default:
      if (type == G_TYPE_STRV) {
        GVariant* var = g_value_get_variant(value);
        const gchar** strv = g_variant_get_strv(var, 0);
        if (strv) {
          object[std::string(key)] = picojson::value(
              SSOUtils::FromStringArrayToJSONArray(
                  reinterpret_cast<const gchar* const*>(strv)));
        }
      }
      break;
  }
}

void SessionData::InsertStringData(const std::string& key,
    const picojson::value& obj) {
  GValue* val = g_new0(GValue, 1);
  g_value_init(val, G_TYPE_STRING);
  g_value_set_string(val, obj.get(key).to_str().c_str());
  g_hash_table_insert(data_, g_strdup(key.c_str()), val);
}

void SessionData::InsertData(const std::string& key,
    const picojson::value& obj) {
  GValue* val = NULL;
  if (obj.is<bool>()) {
    val = g_new0(GValue, 1);
    g_value_init(val, G_TYPE_BOOLEAN);
    g_value_set_boolean(val, (gboolean) obj.get<bool>());
    g_hash_table_insert(data_, g_strdup(key.c_str()), val);
  } else if (obj.is<std::string>()) {
    val = g_new0(GValue, 1);
    g_value_init(val, G_TYPE_STRING);
    g_value_set_string(val, obj.to_str().c_str());
    g_hash_table_insert(data_, g_strdup(key.c_str()), val);
  } else if (obj.is<double>()) {
    val = g_new0(GValue, 1);
    g_value_init(val, G_TYPE_INT);
    g_value_set_int(val, (gint) obj.get<double>());
    g_hash_table_insert(data_, g_strdup(key.c_str()), val);
  } else if (obj.is<picojson::array>() && IsSupportedType(G_TYPE_STRV)) {
    val = g_new0(GValue, 1);
    g_value_init(val, G_TYPE_STRV);
    gchar** strv = SSOUtils::FromJSONArrayToStringArray(
        obj.get<picojson::array>());
    GVariant* var = g_variant_new_strv(strv, -1);
    g_strfreev(strv);
    g_value_set_variant(val, var);
    g_hash_table_insert(data_, g_strdup(key.c_str()), val);
  }
}

bool SessionData::InsertKnownData(const std::string& key,
    const picojson::value& obj) {
  bool inserted = true;
  GValue* val = NULL;
  if (key == "username" ||
      key == "secret" ||
      key == "realm" ||
      key == "networkProxy" ||
      key == "caption") {
    InsertStringData(key, obj);
  } else if (key == "networkTimeout") {
    val = g_new0(GValue, 1);
    g_value_init(val, G_TYPE_UINT);
    g_value_set_uint(val, (guint) obj.get("networkTimeout").get<double>());
    g_hash_table_insert(data_, g_strdup(key.c_str()), val);
  } else if (key == "renewToken") {
    val = g_new0(GValue, 1);
    g_value_init(val, G_TYPE_BOOLEAN);
    g_value_set_boolean(val, (gboolean) obj.get("renewToken").get<bool>());
    g_hash_table_insert(data_, g_strdup(key.c_str()), val);
  } else if (key == "userPromptPolicy") {
    val = g_new0(GValue, 1);
    g_value_init(val, G_TYPE_UINT);
    g_value_set_uint(val, (guint) obj.get("userPromptPolicy").get<double>());
    g_hash_table_insert(data_, g_strdup(key.c_str()), val);
  } else if (key == "windowId") {
    val = g_new0(GValue, 1);
    g_value_init(val, G_TYPE_INT);
    g_value_set_int(val, (gint) obj.get("windowId").get<double>());
    g_hash_table_insert(data_, g_strdup(key.c_str()), val);
  } else {
    inserted = false;
  }
  return inserted;
}

void SessionData::FromJSON(const picojson::value& value) {
  if (!value.is<picojson::object>())
    return;

  const picojson::object& obj = value.get<picojson::object>();
  picojson::object::const_iterator it;
  for (it = obj.begin(); it != obj.end(); ++it) {
    std::string key = it->first;
    picojson::value val = it->second;
    if (!InsertKnownData(key, val)) {
      InsertData(key, val);
    }
  }
}

picojson::value SessionData::ToJSON() const {
  GHashTableIter iter;
  g_hash_table_iter_init(&iter, data_);

  char* key = NULL;
  GValue* value = NULL;
  picojson::value::object object;
  while (g_hash_table_iter_next(&iter, reinterpret_cast<gpointer*>(&key),
      reinterpret_cast<gpointer*>(&value))) {
    ToJSONType(object, key, value);
  }
  return picojson::value(object);
}

GVariant* SessionData::ToVariant() const {
  return signon_hash_table_to_variant(data_);
}

void SessionData::FromVariant(GVariant* vdata) {
  if (data_) {
    g_hash_table_unref(data_);
    data_ = NULL;
  }
  data_ = signon_hash_table_from_variant(vdata);
}

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

  SessionData data;
  data.FromVariant(v_reply);
  g_variant_unref(v_reply);
  picojson::value::object resp;
  resp["sessionData"] = picojson::value(data.ToJSON());
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
  SessionData session_data;
  session_data.FromJSON(value.get("sessionData"));
  GVariant* vsess = session_data.ToVariant();
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
