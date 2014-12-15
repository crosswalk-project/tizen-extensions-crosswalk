// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sso/sso_identity.h"

#include "sso/sso_async_op.h"
#include "sso/sso_instance.h"
#include "sso/sso_utils.h"

IdentityFilterItem::IdentityFilterItem()
  : value_(NULL) {
}

IdentityFilterItem::~IdentityFilterItem() {
  if (value_) g_variant_unref(value_);
  value_ = NULL;
}

void IdentityFilterItem::FromJSON(const std::string& key,
    const picojson::value& value) {
  key_ = key;
  if (key_ == "Type") {
    value_ = g_variant_new_int32(static_cast<gint32>(value.get<double>()));
  } else if (key_ == "Owner") {
    SecurityContext sc;
    sc.FromJSON(value);
    const std::string& sys = sc.sys_context();
    const std::string& app = sc.app_context();
    value_ = g_variant_new("(ss)", sys.empty() ? "" : sys.c_str(),
        app.empty() ? "" : app.c_str());
  } else if (key_ == "Caption") {
    value_ = g_variant_new_string(value.to_str().c_str());
  } else {
    if (value_) g_variant_unref(value_);
    value_ = NULL;
    key_ = "";
  }
  if (value_) g_variant_ref(value_);
}

GHashTable* IdentityFilterItem::FromJSONValueArray(
    const picojson::object& object) {
  GHashTable* filters = NULL;
  if (!object.empty())
    filters = g_hash_table_new_full((GHashFunc) g_str_hash,
        (GEqualFunc) g_str_equal, (GDestroyNotify) g_free,
        (GDestroyNotify) g_variant_unref);
  picojson::object::const_iterator it;
  for (it = object.begin(); it != object.end(); ++it) {
    IdentityFilterItem item;
    item.FromJSON(it->first, it->second);
    if (item.value_)
      g_hash_table_insert(filters, g_strdup(item.key_.c_str()),
          g_variant_ref(item.value_));
  }
  return filters;
}

void SSOIdentity::PostResultCb(SignonIdentity* self, const GError* error,
    gpointer user_data) {
  SSOAsyncOp* async_op = reinterpret_cast<SSOAsyncOp*>(user_data);
  SSOIdentity* identity = reinterpret_cast<SSOIdentity*>(
      async_op->user_data());
  if (error) {
    async_op->PostError(error->message, identity->jsid());
    delete async_op;
    return;
  }
  async_op->PostResult(identity->jsid());
  delete async_op;
}

void SSOIdentity::StoreCb(SignonIdentity* self, guint32 id, const GError* error,
    gpointer user_data) {
  SSOAsyncOp* async_op = reinterpret_cast<SSOAsyncOp*>(user_data);
  SSOIdentity* identity = reinterpret_cast<SSOIdentity*>(
      async_op->user_data());
  if (error) {
    async_op->PostError(error->message, identity->jsid());
    delete async_op;
    return;
  }
  identity->set_identity_id(id);
  picojson::value::object resp;
  resp["identityId"] = picojson::value(static_cast<double>(
      identity->identity_id()));
  async_op->PostResult(picojson::value(resp), identity->jsid());
  delete async_op;
}

void SSOIdentity::VerifyUserCb(SignonIdentity* self, gboolean valid,
    const GError* error, gpointer user_data) {
  SSOAsyncOp* async_op = reinterpret_cast<SSOAsyncOp*>(user_data);
  SSOIdentity* identity = reinterpret_cast<SSOIdentity*>(
      async_op->user_data());
  if (error) {
    async_op->PostError(error->message, identity->jsid());
    delete async_op;
    return;
  }
  picojson::value::object resp;
  resp["valid"] = picojson::value(static_cast<bool>(valid));
  async_op->PostResult(picojson::value(resp), identity->jsid());
  delete async_op;
}

void SSOIdentity::SignedoutSignalCb(gpointer self, gpointer user_data) {
  SSOIdentity* identity = reinterpret_cast<SSOIdentity*>(user_data);
  SSOAsyncOp op(identity->instance_, 0, identity);
  picojson::value::object info;
  info["info"] = picojson::value("signedout");
  op.PostInfo(picojson::value(info), identity->jsid());
}

void SSOIdentity::RemovedSignalCb(gpointer self, gpointer user_data) {
  SSOIdentity* identity = reinterpret_cast<SSOIdentity*>(user_data);
  SSOAsyncOp op(identity->instance_, 0, identity);
  picojson::value::object info;
  info["info"] = picojson::value("removed");
  op.PostInfo(picojson::value(info), identity->jsid());
}

SSOIdentity::SSOIdentity(common::Instance* instance, SignonIdentity* identity,
    int identity_id, int jsid)
    : instance_(instance), identity_(identity), identity_id_(identity_id),
      jsid_(jsid) {
  if (identity_) {
    g_signal_connect(identity_, "signout",
        G_CALLBACK(&SSOIdentity::SignedoutSignalCb), this);
    g_signal_connect(identity_, "removed",
        G_CALLBACK(&SSOIdentity::RemovedSignalCb), this);
  }
}

SSOIdentity::~SSOIdentity() {
  if (identity_) {
    g_object_unref(identity_);
    identity_ = NULL;
  }
}

void SSOIdentity::HandleStartSession(const picojson::value& value) {
  GError* error = NULL;
  std::string method = value.get("method").to_str();
  SignonAuthSession* session = signon_identity_create_session(identity_,
      method.c_str(), &error);
  SSOAsyncOp op(instance_, new picojson::value(value), this);
  if (error) {
    op.PostError(error->message, jsid());
    g_clear_error(&error);
    return;
  }
  SSOAuthSessionPtr sess_ptr = AddAuthSession(session, method,
      static_cast<int>(value.get("sessionJSId").get<double>()));
  op.PostResult(sess_ptr->ToJSON(), jsid());
}

void SSOIdentity::HandleDestroySession(const picojson::value& value) {
  sessions_.erase(static_cast<int>(value.get("sessionJSId").get<double>()));
}

void SSOIdentity::HandleRequestCredentialsUpdate(const picojson::value& value) {
  signon_identity_request_credentials_update(identity_,
      reinterpret_cast<const gchar*>(value.get("message").to_str().c_str()),
      &SSOIdentity::PostResultCb,
      new SSOAsyncOp(instance_, new picojson::value(value), this));
}

void SSOIdentity::HandleStore(const picojson::value& value,
    const std::string& appcontext) {
  SSOIdentityInfo info(NULL);
  info.FromJSON(value.get("info"), appcontext);
  signon_identity_store_credentials_with_info(identity_,
      reinterpret_cast<const SignonIdentityInfo*>(info.info()),
      &SSOIdentity::StoreCb,
      new SSOAsyncOp(instance_, new picojson::value(value), this));
}

void SSOIdentity::HandleAddReference(const picojson::value& value) {
  signon_identity_add_reference(identity_,
      reinterpret_cast<const gchar*>(value.get("reference").to_str().c_str()),
      &SSOIdentity::PostResultCb,
      new SSOAsyncOp(instance_, new picojson::value(value), this));
}

void SSOIdentity::HandleRemoveReference(const picojson::value& value) {
  signon_identity_remove_reference(identity_,
      reinterpret_cast<const gchar*>(value.get("reference").to_str().c_str()),
      &SSOIdentity::PostResultCb,
      new SSOAsyncOp(instance_, new picojson::value(value), this));
}

void SSOIdentity::HandleVerifyUser(const picojson::value& value) {
  picojson::object obj;
  obj["message"] = value.get("message");
  GVariant* args = ArgsToVariant(obj);
  signon_identity_verify_user(identity_, args, &SSOIdentity::VerifyUserCb,
      new SSOAsyncOp(instance_, new picojson::value(value), this));
}

void SSOIdentity::HandleVerifyUserPrompt(const picojson::value& value) {
  GVariant* args = ArgsToVariant(value.get<picojson::object>());
  signon_identity_verify_user(identity_, args, &SSOIdentity::VerifyUserCb,
      new SSOAsyncOp(instance_, new picojson::value(value), this));
}

void SSOIdentity::HandleRemove(const picojson::value& value) {
  signon_identity_remove(identity_, &SSOIdentity::PostResultCb,
      new SSOAsyncOp(instance_, new picojson::value(value), this));
}

void SSOIdentity::HandleSignOut(const picojson::value& value) {
  signon_identity_signout(identity_, &SSOIdentity::PostResultCb,
      new SSOAsyncOp(instance_, new picojson::value(value), this));
}

SSOAuthSessionPtr SSOIdentity::AddAuthSession(SignonAuthSession* session,
    const std::string& method, int jsid) {
  SSOAuthSessionPtr authsession_ptr;
  std::map<int, SSOAuthSessionPtr>::const_iterator it = sessions_.find(jsid);
  if (it != sessions_.end()) {
    authsession_ptr = it->second;
  } else {
    authsession_ptr = std::make_shared<SSOAuthSession>(instance_, session,
        method, jsid);
    sessions_.insert(std::make_pair(jsid, authsession_ptr));
  }
  return authsession_ptr;
}

SSOAuthSessionPtr SSOIdentity::GetAuthSessionPtr(int jsid) const {
  SSOAuthSessionPtr authsession_ptr;
  std::map<int, SSOAuthSessionPtr>::const_iterator it = sessions_.find(jsid);
  if (it != sessions_.end())
    authsession_ptr = it->second;
  return authsession_ptr;
}

GVariant* SSOIdentity::ArgsToVariant(const picojson::object& obj) {
  GVariantBuilder builder;
  g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);
  picojson::object::const_iterator it;
  for (it = obj.begin(); it != obj.end(); ++it) {
    std::string key = it->first;
    picojson::value val = it->second;
    g_variant_builder_add(&builder, "{sv}", key.c_str(), val.to_str().c_str());
  }
  return g_variant_builder_end(&builder);
}
