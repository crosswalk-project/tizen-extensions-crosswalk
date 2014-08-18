// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SSO_SSO_IDENTITY_H_
#define SSO_SSO_IDENTITY_H_

#include <libgsignon-glib/signon-identity.h>
#include <libgsignon-glib/signon-identity-info.h>

#include <map>
#include <string>
#include <utility>

#include "common/picojson.h"
#include "common/utils.h"
#include "sso/sso_auth_session.h"
#include "sso/sso_identity_info.h"

namespace common {

class Instance;

}  // namespace common

typedef enum {
  USER_PROMPT_POLICY_DEFAULT,
  USER_PROMPT_POLICY_REQUEST_PASSWORD,
  USER_PROMPT_POLICY_NO_USER_INTERACTION,
  USER_PROMPT_POLICY_VALIDATION
} UserPromptPolicy;

class IdentityFilterItem {
 public:
  IdentityFilterItem();
  ~IdentityFilterItem();

  static GHashTable* FromJSONValueArray(const picojson::object& object);

 private:
  std::string key_;
  GVariant* value_;

  void FromJSON(const std::string& key, const picojson::value& value);
};

class SSOIdentity {
 public:
  SSOIdentity(common::Instance* instance, SignonIdentity* identity,
      int identity_id, int jsid);
  virtual ~SSOIdentity();

  int jsid() const { return jsid_; }
  int identity_id() const { return identity_id_; }
  void set_identity_id(int id) { identity_id_ = id; }

  SSOAuthSessionPtr GetAuthSessionPtr(int jsid) const;

  void HandleStartSession(const picojson::value& value);
  void HandleDestroySession(const picojson::value& value);
  void HandleRequestCredentialsUpdate(const picojson::value& value);
  void HandleStore(const picojson::value& value, const std::string& appcontext);
  void HandleAddReference(const picojson::value& value);
  void HandleRemoveReference(const picojson::value& value);
  void HandleVerifyUser(const picojson::value& value);
  void HandleVerifyUserPrompt(const picojson::value& value);
  void HandleRemove(const picojson::value& value);
  void HandleSignOut(const picojson::value& value);

 private:
  static void PostResultCb(SignonIdentity* self, const GError* error,
      gpointer user_data);
  static void StoreCb(SignonIdentity* self, guint32 id, const GError* error,
      gpointer user_data);
  static void VerifyUserCb(SignonIdentity* self, gboolean valid,
      const GError* error, gpointer user_data);
  static void SignedoutSignalCb(gpointer self, gpointer user_data);
  static void RemovedSignalCb(gpointer self, gpointer user_data);
  GVariant* ArgsToVariant(const picojson::object& obj);
  SSOAuthSessionPtr AddAuthSession(SignonAuthSession* session,
      const std::string& method, int jsid);

  common::Instance* instance_;
  SignonIdentity* identity_;
  std::map<int, SSOAuthSessionPtr> sessions_;
  int jsid_;
  int identity_id_;

  DISALLOW_COPY_AND_ASSIGN(SSOIdentity);
};
typedef std::shared_ptr<SSOIdentity> SSOIdentityPtr;

#endif  // SSO_SSO_IDENTITY_H_
