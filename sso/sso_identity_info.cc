// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sso/sso_identity_info.h"

#include <libgsignon-glib/signon-security-context.h>

SecurityContext::SecurityContext() {
}

SecurityContext::~SecurityContext() {
}

void SecurityContext::FromJSON(const picojson::value& value) {
  if (value.contains("sysContext"))
    sys_context_ = value.get("sysContext").to_str();
  if (value.contains("appContext"))
    app_context_ = value.get("appContext").to_str();
}

picojson::value SecurityContext::ToJSON() const {
  picojson::value::object object;
  object["sysContext"] = picojson::value(sys_context_);
  object["appContext"] = picojson::value(app_context_);
  return picojson::value(object);
}

bool SecurityContext::ContainsData() const {
  return (!sys_context_.empty() || !app_context_.empty());
}

ACLEntry::ACLEntry() {
}

ACLEntry::~ACLEntry() {
}

void ACLEntry::FromJSON(const picojson::value& value) {
  if (value.contains("securityContext"))
    security_context_.FromJSON(value.get("securityContext"));
  if (value.contains("method"))
    method_ = value.get("method").to_str();
  if (value.contains("mechanisms"))
    mechanisms_ = SSOUtils::FromJSONArrayToStringVector(
        value.get("mechanisms").get<picojson::array>());
}

picojson::value ACLEntry::ToJSON() const {
  picojson::value::object object;
  if (security_context_.ContainsData())
    object["securityContext"] = security_context_.ToJSON();
  if (!method_.empty())
    object["method"] = picojson::value(method_);
  if (!mechanisms_.empty())
    object["mechanisms"] = picojson::value(
        SSOUtils::FromStringVectorToJSONArray(
            (const std::vector<std::string>&) mechanisms_));
  return picojson::value(object);
}

std::vector<ACLEntry> ACLEntry::FromJSONValueArray(
    const picojson::array& array) {
  std::vector<ACLEntry> acl;
  picojson::array::const_iterator it;
  for (it = array.begin(); it != array.end(); ++it) {
    ACLEntry entry;
    entry.FromJSON(*it);
    acl.push_back(entry);
  }
  return acl;
}

picojson::value ACLEntry::ToJSONValueArray(
    const std::vector<ACLEntry>& entries) {
  picojson::array array;
  std::vector<ACLEntry>::const_iterator it;
  for (it = entries.begin(); it != entries.end(); ++it) {
    ACLEntry entry = *it;
    if (entry.ContainsData())
      array.push_back(entry.ToJSON());
  }
  return picojson::value(array);
}

bool ACLEntry::ContainsData() const {
  return (security_context_.ContainsData() ||
      !method_.empty() ||
      !mechanisms_.empty());
}

void SSOIdentityInfo::AddMethodMechanisms(gpointer method, gpointer mechanisms,
    gpointer user_data) {
  ACLEntry* entry = reinterpret_cast<ACLEntry*>(user_data);
  entry->set_method(std::string(reinterpret_cast<const gchar*>(method)));
  entry->set_mechanisms(SSOUtils::FromStringArrayToStringVector(
      reinterpret_cast<const gchar* const*>(mechanisms)));
}

std::vector<ACLEntry> SSOIdentityInfo::GetACL(SignonIdentityInfo* info) const {
  std::vector<ACLEntry> acl;
  SignonSecurityContextList* info_acl =
      signon_identity_info_get_access_control_list(info);
  if (!info_acl)
    return acl;

  for (info_acl = g_list_first(info_acl);
       info_acl != NULL;
       info_acl = g_list_next(info_acl)) {
    const SignonSecurityContext* context =
        reinterpret_cast<const SignonSecurityContext*>(info_acl->data);
    const gchar* sysctx = signon_security_context_get_system_context(context);
    SecurityContext ctx;
    if (sysctx)
      ctx.set_sys_context(sysctx);

    const gchar* appctx = signon_security_context_get_application_context(
        context);
    if (appctx)
      ctx.set_app_context(appctx);
    ACLEntry entry;
    entry.set_security_context(ctx);

    GHashTable* methods = signon_identity_info_get_methods(info);
    if (methods)
      g_hash_table_foreach(methods, &SSOIdentityInfo::AddMethodMechanisms,
          &entry);
    if (sysctx || appctx || methods)
      acl.push_back(entry);
  }
  return acl;
}

SecurityContext SSOIdentityInfo::GetOwner(SignonIdentityInfo* info) const {
  const SignonSecurityContext* context = signon_identity_info_get_owner(info);
  SecurityContext ctx;
  if (context) {
    ctx.set_app_context(context->app_ctx);
    ctx.set_sys_context(context->sys_ctx);
  }
  return ctx;
}

SSOIdentityInfo::SSOIdentityInfo(SignonIdentityInfo* info)
    : info_(0) {
  if (info)
    info_ = signon_identity_info_copy(info);
}

SSOIdentityInfo::~SSOIdentityInfo() {
  if (info_) {
    signon_identity_info_free(info_);
    info_ = NULL;
  }
}

void SSOIdentityInfo::FromJSON(const picojson::value& value,
    const std::string& appcontext) {
  if (info_)
    signon_identity_info_free(info_);
  info_ = signon_identity_info_new();
  if (value.contains("type"))
    signon_identity_info_set_identity_type(info_,
        static_cast<SignonIdentityType>(value.get("type").get<double>()));

  if (value.contains("username"))
    signon_identity_info_set_username(info_,
        value.get("username").to_str().c_str());

  if (value.contains("secret"))
    signon_identity_info_set_secret(info_, value.get("secret").to_str().c_str(),
        value.get("storeSecret").get<bool>());

  if (value.contains("caption"))
    signon_identity_info_set_caption(info_,
        value.get("caption").to_str().c_str());

  if (value.contains("realms")) {
    gchar** strv = SSOUtils::FromJSONArrayToStringArray(
        value.get("realms").get<picojson::array>());
    if (strv) {
      signon_identity_info_set_realms(info_,
          reinterpret_cast<const gchar* const*>(strv));
      g_strfreev(strv);
    }
  }

  if (value.contains("owner")) {
    SecurityContext ctx;
    ctx.FromJSON(value.get("owner"));
    if (ctx.app_context().empty())
      ctx.set_app_context(appcontext);
    if (ctx.ContainsData()) {
      signon_identity_info_set_owner_from_values(info_,
          ctx.sys_context().c_str(), ctx.app_context().c_str());
    }
  }

  if (value.contains("accessControlList")) {
    std::vector<ACLEntry> acl = ACLEntry::FromJSONValueArray(
        value.get("accessControlList").get<picojson::array>());
    std::vector<ACLEntry>::const_iterator it;
    for (it = acl.begin(); it != acl.end(); ++it) {
      ACLEntry entry = *it;
      if (!entry.method().empty() && !entry.mechanisms().empty()) {
        gchar** mechs = SSOUtils::FromStringVectorToStringArray(
            entry.mechanisms());
        if (mechs) {
          signon_identity_info_set_method(info_, entry.method().c_str(),
              reinterpret_cast<const gchar* const*>(mechs));
          g_strfreev(mechs);
        }
      }
      if (entry.security_context().ContainsData()) {
        SignonSecurityContext* sc = signon_security_context_new_from_values(
            entry.security_context().sys_context().c_str(),
            entry.security_context().app_context().c_str());
        signon_identity_info_access_control_list_append(info_, sc);
      }
    }
  }
}

picojson::value SSOIdentityInfo::ToJSON() const {
  picojson::value::object object;
  if (!info_)
    return picojson::value(object);

  object["id"] = picojson::value(
      static_cast<double>(signon_identity_info_get_id(info_)));

  SignonIdentityType type = signon_identity_info_get_identity_type(info_);
  object["type"] = picojson::value(static_cast<double>(type));

  const gchar* str = signon_identity_info_get_username(info_);
  if (str)
    object["username"] = picojson::value(std::string(str));

  object["storeSecret"] = picojson::value(
      static_cast<bool>(signon_identity_info_get_storing_secret(info_)));

  str = signon_identity_info_get_caption(info_);
  if (str)
    object["caption"] = picojson::value(std::string(str));

  const gchar* const* strv = signon_identity_info_get_realms(info_);
  if (strv)
    object["realms"] = picojson::value(
        SSOUtils::FromStringArrayToJSONArray(strv));

  SecurityContext owner = GetOwner(info_);
  if (!owner.app_context().empty() || !owner.sys_context().empty())
    object["owner"] = owner.ToJSON();

  std::vector<ACLEntry> acl = GetACL(info_);
  if (!acl.empty())
    object["accessControlList"] = ACLEntry::ToJSONValueArray(acl);

  return picojson::value(object);
}
