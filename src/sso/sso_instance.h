// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SSO_SSO_INSTANCE_H_
#define SSO_SSO_INSTANCE_H_

#include <string>
#include <thread>  // NOLINT

#include "common/extension.h"
#include "sso/sso_auth_session.h"
#include "sso/sso_identity.h"

class SSOAuthService;

class SSOInstance: public common::Instance {
 public:
  SSOInstance();
  virtual ~SSOInstance();

 private:
  // common::Instance implementation.
  virtual void HandleMessage(const char* msg);
  virtual void HandleSyncMessage(const char* msg);

  SSOAuthService* GetService(const picojson::value& v) const;
  SSOIdentityPtr GetIdentity(const picojson::value& v) const;
  SSOAuthSessionPtr GetAuthSession(const picojson::value& v) const;

  void HandleAuthServiceMessage(const std::string& cmd,
      const picojson::value& v);
  void HandleIdentityMessage(const std::string& cmd, const picojson::value& v);
  void HandleAuthSessionMessage(const std::string& cmd,
      const picojson::value& v);
  void PostError(const picojson::value& v, const std::string& msg);

  SSOAuthService* sso_auth_service_;
};

#endif  // SSO_SSO_INSTANCE_H_
