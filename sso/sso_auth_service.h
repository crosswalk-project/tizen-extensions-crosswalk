// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SSO_SSO_AUTH_SERVICE_H_
#define SSO_SSO_AUTH_SERVICE_H_

#include <libgsignon-glib/signon-auth-service.h>

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "sso/sso_auth_session.h"
#include "sso/sso_identity.h"
#include "sso/sso_utils.h"

namespace common {

class Instance;

}  // namespace common

namespace picojson {

class value;

}  // namespace picojson

class MechanismQueryResult {
 public:
  MechanismQueryResult(const std::string& method, const gchar* const* mechs);
  ~MechanismQueryResult();

  void FromJSON(const picojson::value& value);
  picojson::value ToJSON() const;
  bool ContainsData() const;

 private:
  std::string method_;
  std::vector<std::string> mechanisms_;
};

class SSOAuthService {
 public:
  SSOAuthService(common::Instance* instance, const std::string& appcontext);
  virtual ~SSOAuthService();

  int jsid() const { return jsid_; }
  void set_jsid(int id) { jsid_ = id; }
  const std::string& appcontext() const { return appcontext_; }

  void HandleQueryMethods(const picojson::value& value);
  void HandleQueryMechanisms(const picojson::value& value);
  void HandleQueryIdentities(const picojson::value& value);
  void HandleGetIdentity(const picojson::value& value);
  void HandleCreateIdentity(const picojson::value& value);
  void HandleDestroyIdentity(const picojson::value& value);
  void HandleStartSession(const picojson::value& value);
  void HandleClear(const picojson::value& value);

  SSOIdentityPtr GetIdentityPtr(int jsid) const;
  SSOAuthSessionPtr GetAuthSessionPtr(int jsid) const;

 private:
  static void QueryMethodsCb(SignonAuthService* auth_service, gchar** methods,
      const GError* error, gpointer user_data);
  static void QueryMechanismsCb(SignonAuthService* auth_service,
      const gchar* method, gchar** mechanisms, const GError* error,
      gpointer user_data);
  static void QueryIdentitiesCb(SignonAuthService* auth_service,
      SignonIdentityList* identity_list, const GError* error,
      gpointer user_data);
  static void GetIdentityCb(SignonIdentity* identity, SignonIdentityInfo* info,
      const GError* error, gpointer user_data);
  static void ClearCb(SignonAuthService* auth_service, gboolean success,
      const GError* error, gpointer user_data);
  SSOIdentityPtr AddIdentity(SignonIdentity* identity, int jsid, int id);

 private:
  common::Instance* instance_;
  SignonAuthService* auth_service_;
  std::map<int, SSOIdentityPtr> identities_;
  int jsid_;
  std::string appcontext_;

  DISALLOW_COPY_AND_ASSIGN(SSOAuthService);
};

#endif  // SSO_SSO_AUTH_SERVICE_H_
