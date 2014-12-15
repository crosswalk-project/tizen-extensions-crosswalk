// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sso/sso_instance.h"

#include <sstream>
#include <string>

#include "common/picojson.h"
#include "sso/sso_async_op.h"
#include "sso/sso_auth_service.h"

SSOInstance::SSOInstance() {
  std::string id_str = common::Extension::GetRuntimeVariable("app_id", 64);
  std::istringstream buf(id_str);
  picojson::value id_val;
  picojson::parse(id_val, buf);
  sso_auth_service_ = new SSOAuthService(this, id_val.get<std::string>());
}

SSOInstance::~SSOInstance() {
  delete sso_auth_service_;
}

SSOAuthService* SSOInstance::GetService(const picojson::value& v) const {
  SSOAuthService* service = 0;
  int jsid = static_cast<int>(v.get("serviceJSId").get<double>());
  if (sso_auth_service_->jsid() == -1) {
    sso_auth_service_->set_jsid(jsid);
    service = sso_auth_service_;
  } else if (jsid == sso_auth_service_->jsid()) {
    service = sso_auth_service_;
  }
  return service;
}

SSOIdentityPtr SSOInstance::GetIdentity(const picojson::value& v) const {
  int jsid = static_cast<int>(v.get("identityJSId").get<double>());
  return sso_auth_service_->GetIdentityPtr(jsid);
}

SSOAuthSessionPtr SSOInstance::GetAuthSession(const picojson::value& v) const {
  int jsid = static_cast<int>(v.get("sessionJSId").get<double>());
  return sso_auth_service_->GetAuthSessionPtr(jsid);
}

void SSOInstance::PostError(const picojson::value& v, const std::string& msg) {
  SSOAsyncOp op(this, new picojson::value(v), 0);
  op.PostError(msg.c_str(), -1);
}

void SSOInstance::HandleAuthServiceMessage(const std::string& cmd,
    const picojson::value& v) {
  SSOAuthService* service = GetService(v);
  if (!service) {
    PostError(v, "Invalid Service Object");
    return;
  }

  if (cmd == "queryMethods") {
    service->HandleQueryMethods(v);
  } else if (cmd == "queryMechanisms") {
    service->HandleQueryMechanisms(v);
  } else if (cmd == "queryIdentities") {
    service->HandleQueryIdentities(v);
  } else if (cmd == "getIdentity") {
    service->HandleGetIdentity(v);
  } else if (cmd == "destroyIdentity") {
    service->HandleDestroyIdentity(v);
  } else if (cmd == "clear") {
    service->HandleClear(v);
  } else {
    std::cerr << "Received unknown message: " << cmd << "\n";
  }
}

void SSOInstance::HandleIdentityMessage(const std::string& cmd,
    const picojson::value& v) {
  SSOIdentityPtr identity = GetIdentity(v);
  if (!identity) {
    PostError(v, "Invalid Identity Object");
    return;
  }

  if (cmd == "startSession") {
    identity->HandleStartSession(v);
  } else if (cmd == "destroySession") {
    identity->HandleDestroySession(v);
  } else if (cmd == "requestCredentialsUpdate") {
    identity->HandleRequestCredentialsUpdate(v);
  } else if (cmd == "store") {
    identity->HandleStore(v, sso_auth_service_->appcontext());
  } else if (cmd == "addReference") {
    identity->HandleAddReference(v);
  } else if (cmd == "removeReference") {
    identity->HandleRemoveReference(v);
  } else if (cmd == "verifyUser") {
    identity->HandleVerifyUser(v);
  } else if (cmd == "verifyUserPrompt") {
    identity->HandleVerifyUserPrompt(v);
  } else if (cmd == "remove") {
    identity->HandleRemove(v);
  } else if (cmd == "signout") {
    identity->HandleSignOut(v);
  } else {
    std::cerr << "Received unknown message: " << cmd << "\n";
  }
}

void SSOInstance::HandleAuthSessionMessage(const std::string& cmd,
    const picojson::value& v) {
  SSOAuthSessionPtr session = GetAuthSession(v);
  if (!session) {
    PostError(v, "Invalid AuthSession Object");
    return;
  }

  if (cmd == "queryAvailableMechanisms") {
    session->HandleQueryAvailableMechanisms(v);
  } else if (cmd == "challenge") {
    session->HandleChallenge(v);
  } else if (cmd == "cancel") {
    session->HandleCancel(v);
  } else {
    std::cerr << "Received unknown message: " << cmd << "\n";
  }
}

void SSOInstance::HandleMessage(const char* message) {
  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    return;
  }
  std::string cmd = v.get("asyncOpCmd").to_str();
  if (cmd == "queryMethods" ||
      cmd == "queryMechanisms" ||
      cmd == "queryIdentities" ||
      cmd == "getIdentity" ||
      cmd == "destroyIdentity" ||
      cmd == "clear") {
    HandleAuthServiceMessage(cmd, v);
  } else if (cmd == "startSession" ||
      cmd == "requestCredentialsUpdate" ||
      cmd == "store" ||
      cmd == "addReference" ||
      cmd == "removeReference" ||
      cmd == "verifyUser" ||
      cmd == "verifyUserPrompt" ||
      cmd == "remove" ||
      cmd == "signout") {
    HandleIdentityMessage(cmd, v);
  } else if (cmd == "queryAvailableMechanisms" ||
      cmd == "challenge" ||
      cmd == "cancel") {
    HandleAuthSessionMessage(cmd, v);
  } else {
    std::cerr << "Received unknown message: " << cmd << "\n";
  }
}

void SSOInstance::HandleSyncMessage(const char* message) {
  picojson::value::object obj;
  picojson::value v;
  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    obj["synOpErrorMsg"] = picojson::value("Unable to parse message");
    SendSyncReply(picojson::value(obj).serialize().c_str());
    return;
  }

  std::string cmd = v.get("syncOpCmd").to_str();
  SSOAuthService* service = 0;
  if (cmd == "createIdentity") {
    service = GetService(v);
    if (service)
      service->HandleCreateIdentity(v);
    else
      obj["synOpErrorMsg"] = picojson::value("Invalid Service Object");
  } else {
    obj["synOpErrorMsg"] = picojson::value("Unknown sync command");
  }
  SendSyncReply(picojson::value(obj).serialize().c_str());
}
