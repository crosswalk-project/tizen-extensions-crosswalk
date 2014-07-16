// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sso/sso_auth_service.h"

#include <libgsignon-glib/signon-identity.h>
#include <stdlib.h>
#include <string>

#include "common/picojson.h"
#include "sso/sso_async_op.h"
#include "sso/sso_instance.h"
#include "sso/sso_utils.h"

MechanismQueryResult::MechanismQueryResult(const std::string& method,
    const gchar* const* mechs)
    : method_(method) {
  if (mechs) {
    while (*mechs)
      mechanisms_.push_back(*mechs++);
  }
}

MechanismQueryResult::~MechanismQueryResult() {
}

void MechanismQueryResult::FromJSON(const picojson::value& value) {
  if (value.contains("method"))
    method_ = value.get("method").to_str();
  if (value.contains("mechanisms"))
    mechanisms_ = SSOUtils::FromJSONArrayToStringVector(
        value.get("mechanisms").get<picojson::array>());
}

picojson::value MechanismQueryResult::ToJSON() const {
  picojson::value::object object;
  if (!method_.empty())
    object["method"] = picojson::value(method_);
  if (!mechanisms_.empty())
    object["mechanisms"] = picojson::value(
        SSOUtils::FromStringVectorToJSONArray(mechanisms_));
  return picojson::value(object);
}

bool MechanismQueryResult::ContainsData() const {
  return (!method_.empty() || !mechanisms_.empty());
}

void SSOAuthService::QueryMethodsCb(SignonAuthService* auth_service,
    gchar** methods, const GError* error, gpointer user_data) {
  SSOAsyncOp* async_op = reinterpret_cast<SSOAsyncOp*>(user_data);
  SSOAuthService* service = reinterpret_cast<SSOAuthService*>(
      async_op->user_data());
  if (error) {
    async_op->PostError(error->message, service->jsid());
    delete async_op;
    return;
  }
  picojson::value::object resp;
  resp["methods"] = picojson::value(SSOUtils::FromStringArrayToJSONArray(
      reinterpret_cast<const gchar* const*>(methods)));
  g_strfreev(methods);
  async_op->PostResult(picojson::value(resp), service->jsid());
  delete async_op;
}

void SSOAuthService::QueryMechanismsCb(SignonAuthService* auth_service,
    const gchar* method, gchar** mechanisms, const GError* error,
    gpointer user_data) {
  SSOAsyncOp* async_op = reinterpret_cast<SSOAsyncOp*>(user_data);
  SSOAuthService* service = reinterpret_cast<SSOAuthService*>(
      async_op->user_data());
  if (error) {
    async_op->PostError(error->message, service->jsid());
    delete async_op;
    return;
  }
  MechanismQueryResult query_result(
      async_op->request_data()->get("method").get<std::string>(),
      reinterpret_cast<const gchar* const*>(mechanisms));
  if (mechanisms)
    g_strfreev(mechanisms);

  async_op->PostResult(query_result.ToJSON(), service->jsid());
  delete async_op;
}

void SSOAuthService::QueryIdentitiesCb(SignonAuthService* auth_service,
    SignonIdentityList* identity_list, const GError* error,
    gpointer user_data) {
  GList* iter = identity_list;
  SSOAsyncOp* async_op = reinterpret_cast<SSOAsyncOp*>(user_data);
  SSOAuthService* service = reinterpret_cast<SSOAuthService*>(
      async_op->user_data());
  if (error) {
    async_op->PostError(error->message, service->jsid());
    delete async_op;
    return;
  }
  picojson::array infos;
  while (iter) {
    SignonIdentityInfo* info = static_cast<SignonIdentityInfo*>(iter->data);
    SSOIdentityInfo identity_info(info);
    infos.push_back(identity_info.ToJSON());
    iter = g_list_next(iter);
  }
  g_list_free_full(identity_list, (GDestroyNotify) signon_identity_info_free);

  async_op->PostResult(picojson::value(infos), service->jsid());
  delete async_op;
}

void SSOAuthService::GetIdentityCb(SignonIdentity* identity,
    SignonIdentityInfo* info, const GError* error, gpointer user_data) {
  SSOAsyncOp* async_op = reinterpret_cast<SSOAsyncOp*>(user_data);
  SSOAuthService* service = reinterpret_cast<SSOAuthService*>(
      async_op->user_data());
  if (error) {
    picojson::value::object resp;
    resp["identityJSId"] = async_op->request_data()->get("identityJSId");
    async_op->PostError(error->message, picojson::value(resp), service->jsid());
    delete async_op;
    return;
  }
  SSOIdentityPtr id_ptr = service->AddIdentity(identity,
      static_cast<int>(async_op->request_data()->get(
          "identityJSId").get<double>()), signon_identity_info_get_id(info));
  SSOIdentityInfo id_info(info);
  picojson::value::object resp;
  resp["identityJSId"] = async_op->request_data()->get("identityJSId");
  resp["info"] = id_info.ToJSON();
  async_op->PostResult(picojson::value(resp), service->jsid());
  delete async_op;
}

void SSOAuthService::ClearCb(SignonAuthService* auth_service, gboolean success,
    const GError* error, gpointer user_data) {
  SSOAsyncOp* async_op = reinterpret_cast<SSOAsyncOp*>(user_data);
  SSOAuthService* service = reinterpret_cast<SSOAuthService*>(
      async_op->user_data());
  if (error) {
    async_op->PostError(error->message, service->jsid());
    delete async_op;
    return;
  }
  picojson::value::object resp;
  resp["success"] = picojson::value(static_cast<bool>(success));
  async_op->PostResult(picojson::value(resp), service->jsid());
  delete async_op;
}

SSOAuthService::SSOAuthService(common::Instance* instance,
    const std::string& appcontext)
    : instance_(instance), auth_service_(0), jsid_(-1),
      appcontext_(appcontext) {
  auth_service_ = signon_auth_service_new();
}

SSOAuthService::~SSOAuthService() {
  if (auth_service_) {
    g_object_unref(auth_service_);
    auth_service_ = 0;
  }
}

void SSOAuthService::HandleQueryMethods(const picojson::value& value) {
  signon_auth_service_query_methods(auth_service_,
      &SSOAuthService::QueryMethodsCb,
      new SSOAsyncOp(instance_, new picojson::value(value), this));
}

void SSOAuthService::HandleQueryMechanisms(const picojson::value& value) {
  signon_auth_service_query_mechanisms(auth_service_,
      reinterpret_cast<const gchar*>(value.get("method").to_str().c_str()),
      &SSOAuthService::QueryMechanismsCb,
      new SSOAsyncOp(instance_, new picojson::value(value), this));
}

void SSOAuthService::HandleQueryIdentities(const picojson::value& value) {
  GHashTable* filters = IdentityFilterItem::FromJSONValueArray(
      value.get("filter").get<picojson::object>());
  signon_auth_service_query_identities(auth_service_, filters,
      reinterpret_cast<const gchar*>(appcontext_.c_str()),
      &SSOAuthService::QueryIdentitiesCb,
      new SSOAsyncOp(instance_, new picojson::value(value), this));
  if (filters)
    g_hash_table_unref(filters);
}

void SSOAuthService::HandleGetIdentity(const picojson::value& value) {
  double id = value.get("identityId").get<double>();
  SignonIdentity* identity = signon_identity_new_with_context_from_db(
      static_cast<gint>(id),
      reinterpret_cast<const gchar*>(appcontext_.c_str()));
  signon_identity_query_info(identity, &SSOAuthService::GetIdentityCb,
      new SSOAsyncOp(instance_, new picojson::value(value), this));
}

void SSOAuthService::HandleCreateIdentity(const picojson::value& value) {
  SignonIdentity* identity = signon_identity_new_with_context(
      reinterpret_cast<const gchar*>(appcontext_.c_str()));
  AddIdentity(identity,
      static_cast<int>(value.get("identityJSId").get<double>()), 0);
}

void SSOAuthService::HandleDestroyIdentity(const picojson::value& value) {
  identities_.erase(static_cast<int>(value.get("identityJSId").get<double>()));
}

void SSOAuthService::HandleClear(const picojson::value& value) {
  signon_auth_service_clear(auth_service_, &SSOAuthService::ClearCb,
      new SSOAsyncOp(instance_, new picojson::value(value), this));
}

SSOIdentityPtr SSOAuthService::AddIdentity(SignonIdentity* identity, int jsid,
    int identity_id) {
  std::map<int, SSOIdentityPtr>::const_iterator it = identities_.find(jsid);
  if (it != identities_.end())
    return it->second;

  SSOIdentityPtr identity_ptr;
  if (identity_id != 0) {
    for (it = identities_.begin(); it != identities_.end(); ++it) {
      SSOIdentityPtr ptr = it->second;
      if (identity_id == ptr->identity_id()) {
        identity_ptr = ptr;
        break;
      }
    }
  }
  if (!identity_ptr) {
    identity_ptr = std::make_shared<SSOIdentity>(instance_, identity,
        identity_id, jsid);
  }
  identities_.insert(std::make_pair(jsid, identity_ptr));
  return identity_ptr;
}

SSOIdentityPtr SSOAuthService::GetIdentityPtr(int jsid) const {
  SSOIdentityPtr identity_ptr;
  std::map<int, SSOIdentityPtr>::const_iterator it = identities_.find(jsid);
  if (it != identities_.end())
    identity_ptr = it->second;
  return identity_ptr;
}

SSOAuthSessionPtr SSOAuthService::GetAuthSessionPtr(int jsid) const {
  std::map<int, SSOIdentityPtr>::const_iterator it;
  for (it = identities_.begin(); it != identities_.end(); ++it) {
    SSOAuthSessionPtr ptr = it->second->GetAuthSessionPtr(jsid);
    if (ptr)
      return ptr;
  }
  return SSOAuthSessionPtr();
}
