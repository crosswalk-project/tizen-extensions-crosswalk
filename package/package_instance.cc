// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "package/package_instance.h"

#include "package/package_extension.h"
#include "package/package_info.h"
#include "package/package_manager.h"

namespace {

const char kJSCallbackKey[] = "_callback";
const char kPkgInfoEventCallback[] = "_pkgInfoEventCallback";

}  // namespace

PackageInstance::PackageInstance(PackageExtension* extension)
    : extension_(extension) {
  package_manager_.reset(new PackageManager(this));
}

PackageInstance::~PackageInstance() {
  package_manager_->PackageManagerDestroy();
}

void PackageInstance::HandleMessage(const char* message) {
  picojson::value v;
  picojson::value::object o;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cerr << "Ignoring message. \n";
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "PackageManager.install") {
    HandleInstallRequest(v);
  } else if (cmd == "PackageManager.uninstall") {
    HandleUninstallRequest(v);
  } else if (cmd == "PackageManager.getPackagesInfo") {
    HandleGetPackagesInfoRequest(v);
  } else {
    std::cerr << "Message " + cmd + " is not supported.\n";
  }
}

void PackageInstance::HandleSyncMessage(const char* message) {
  picojson::value v;
  picojson::value::object o;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cerr << "Ignoring message.\n";
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "PackageManager.setPackageInfoEventListener") {
    HandleRegisterPackageInfoEvent();
  } else if (cmd == "PackageManager.unsetPackageInfoEventListener") {
    HandleUnregisterPackageInfoEvent();
  } else if (cmd == "PackageManager.getPackageInfo") {
    HandleGetPackageInfoRequest(v);
  } else {
    std::cerr << "Message " + cmd + " is not supported.\n";
  }
}

void PackageInstance::HandleInstallRequest(const picojson::value& msg) {
  const char* pkg_path = msg.get("id").to_str().c_str();
  double callback_id = msg.get(kJSCallbackKey).get<double>();
  std::unique_ptr<picojson::value> result(
      package_manager()->InstallPackage(pkg_path, callback_id));
  if (result)
    ReturnMessageAsync(callback_id, *result);
}

void PackageInstance::HandleUninstallRequest(const picojson::value& msg) {
  const char* pkg_id = msg.get("id").to_str().c_str();
  double callback_id = msg.get(kJSCallbackKey).get<double>();
  std::unique_ptr<picojson::value> result(
      package_manager()->UnInstallPackage(pkg_id, callback_id));
  if (result)
    ReturnMessageAsync(callback_id, *result);
}

void PackageInstance::HandleGetPackageInfoRequest(const picojson::value& msg) {
  std::string id = (msg.contains("id") && msg.get("id").is<std::string>()) ?
      msg.get("id").to_str() :
      extension_->current_app_id();

  PackageInformation pkg_info(id, false);
  SendSyncReply(pkg_info.Serialize().c_str());
}

void PackageInstance::HandleGetPackagesInfoRequest(const picojson::value& msg) {
  std::unique_ptr<picojson::value> result(
      PackageInformation::GetAllInstalled());
  double callback_id = msg.get(kJSCallbackKey).get<double>();
  ReturnMessageAsync(callback_id, *result);
}

void PackageInstance::HandleRegisterPackageInfoEvent() {
  std::unique_ptr<picojson::value> result(
      package_manager()->RegisterPackageInfoEvent());
  SendSyncReply(result->serialize().c_str());
}

void PackageInstance::HandleUnregisterPackageInfoEvent() {
  std::unique_ptr<picojson::value> result(
      package_manager()->UnregisterPackageInfoEvent());
  SendSyncReply(result->serialize().c_str());
}

void PackageInstance::PostPackageInfoEventMessage(picojson::object& events) {
  events[kJSCallbackKey] = picojson::value(kPkgInfoEventCallback);
  PostMessage(picojson::value(events).serialize().c_str());
}

void PackageInstance::PostPackageRequestMessage(
    picojson::object& reply,
    double callback_id) {
  reply[kJSCallbackKey] = picojson::value(callback_id);
  PostMessage(picojson::value(reply).serialize().c_str());
}

void PackageInstance::ReturnMessageAsync(
    double callback_id,
    picojson::value& value) {
  value.get<picojson::object>()[kJSCallbackKey] = picojson::value(callback_id);
  PostMessage(value.serialize().c_str());
}
