// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SSO_SSO_IDENTITY_INFO_H_
#define SSO_SSO_IDENTITY_INFO_H_

#include <libgsignon-glib/signon-identity-info.h>

#include <memory>
#include <string>
#include <vector>

#include "common/picojson.h"
#include "common/utils.h"
#include "sso/sso_utils.h"

class SecurityContext {
 public:
  SecurityContext();
  ~SecurityContext();

  const std::string& sys_context() const { return sys_context_; }
  void set_sys_context(const std::string& sc) { sys_context_ = sc; }
  const std::string& app_context() const { return app_context_; }
  void set_app_context(const std::string& ac) { app_context_ = ac; }

  void FromJSON(const picojson::value& value);
  picojson::value ToJSON() const;

  bool ContainsData() const;

 private:
  std::string sys_context_;
  std::string app_context_;
};

class ACLEntry {
 public:
  ACLEntry();
  ~ACLEntry();

  const SecurityContext& security_context() const { return security_context_; }
  void set_security_context(const SecurityContext& ctx) {
    security_context_ = ctx;
  }
  const std::string& method() const { return method_; }
  void set_method(const std::string& method) { method_ = method; }
  const std::vector<std::string>& mechanisms() const { return mechanisms_; }
  void set_mechanisms(const std::vector<std::string>& mechs) {
    mechanisms_ = mechs;
  }

  static std::vector<ACLEntry> FromJSONValueArray(const picojson::array& array);
  static picojson::value ToJSONValueArray(const std::vector<ACLEntry>& entries);
  bool ContainsData() const;

 private:
  SecurityContext security_context_;
  std::string method_;
  std::vector<std::string> mechanisms_;

  void FromJSON(const picojson::value& value);
  picojson::value ToJSON() const;
};

class SSOIdentityInfo {
 public:
  explicit SSOIdentityInfo(SignonIdentityInfo* info);
  virtual ~SSOIdentityInfo();

  void FromJSON(const picojson::value& value, const std::string& appcontext);
  picojson::value ToJSON() const;

  SignonIdentityInfo* info() const { return info_; }

 private:
  SecurityContext GetOwner(SignonIdentityInfo* info) const;
  std::vector<ACLEntry> GetACL(SignonIdentityInfo* info) const;
  static void AddMethodMechanisms(gpointer method, gpointer mechanisms,
      gpointer user_data);

  SignonIdentityInfo* info_;

  DISALLOW_COPY_AND_ASSIGN(SSOIdentityInfo);
};
typedef std::shared_ptr<SSOIdentityInfo> SSOIdentityInfoPtr;

#endif  // SSO_SSO_IDENTITY_INFO_H_
