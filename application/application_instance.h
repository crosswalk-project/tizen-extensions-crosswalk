// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPLICATION_APPLICATION_INSTANCE_H_
#define APPLICATION_APPLICATION_INSTANCE_H_

#include "common/extension.h"
#include "common/picojson.h"

class ApplicationExtension;

class ApplicationInstance : public common::Instance {
 public:
  explicit ApplicationInstance(ApplicationExtension* extension);
  ~ApplicationInstance();

 private:
  // common::Instance implementation.
  virtual void HandleMessage(const char* msg);
  virtual void HandleSyncMessage(const char* msg);

  void HandleGetAppInfo(picojson::value& msg);
  void HandleGetAppsInfo(picojson::value& msg);

  void ReturnMessageAsync(picojson::value& msg, const picojson::object& obj);

  ApplicationExtension* extension_;
};

#endif  // APPLICATION_APPLICATION_INSTANCE_H_
