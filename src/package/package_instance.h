// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PACKAGE_PACKAGE_INSTANCE_H_
#define PACKAGE_PACKAGE_INSTANCE_H_

#include <memory>
#include <string>

#include "common/extension.h"
#include "common/picojson.h"
#include "tizen/tizen.h"

namespace picojson {

class value;

}  // namespace picojson

class PackageManager;
class PackageExtension;

class PackageInstance : public common::Instance {
 public:
  explicit PackageInstance(PackageExtension* extension);
  virtual ~PackageInstance();

  void PostPackageInfoEventMessage(picojson::object& events);
  void PostPackageRequestMessage(picojson::object& events, double callback_id);

  PackageManager* package_manager() { return package_manager_.get(); }

 private:
  // common::Instance implementation
  virtual void HandleMessage(const char* msg);
  virtual void HandleSyncMessage(const char* msg);

  // Synchronous message handlers.
  void HandleGetPackageInfoRequest(const picojson::value& json);
  void HandleRegisterPackageInfoEvent();
  void HandleUnregisterPackageInfoEvent();

  // Asynchronous message handlers.
  void HandleInstallRequest(const picojson::value& json);
  void HandleUninstallRequest(const picojson::value& json);
  void HandleGetPackagesInfoRequest(const picojson::value& json);

  void ReturnMessageAsync(double callback_id, picojson::value& value);

  std::unique_ptr<PackageManager> package_manager_;
  PackageExtension* extension_;
};

#endif  // PACKAGE_PACKAGE_INSTANCE_H_
