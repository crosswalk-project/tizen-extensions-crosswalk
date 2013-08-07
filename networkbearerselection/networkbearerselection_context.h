// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NETWORKBEARERSELECTION_NETWORKBEARERSELECTION_CONTEXT_H_
#define NETWORKBEARERSELECTION_NETWORKBEARERSELECTION_CONTEXT_H_

#include <string>
#include "tizen/tizen.h"

class ContextAPI;
class NetworkBearerSelectionRequest;

enum NetworkType {
  CELLULAR = 0,
  UNKNOWN = 1,
};

class NetworkBearerSelectionContext {
 public:
  explicit NetworkBearerSelectionContext(ContextAPI* api);
  virtual ~NetworkBearerSelectionContext() {};

  // ExtensionAdapter implementation.
  static const char name[];
  static const char* GetJavaScript();
  void HandleMessage(const char* message);
  void HandleSyncMessage(const char* message) {}

  void PostMessage(const std::string& cmd, WebApiAPIErrors error,
                   bool disconnected, const std::string& reply_id);

 private:
  virtual void OnRequestRouteToHost(NetworkBearerSelectionRequest* request) = 0;
  virtual void OnReleaseRouteToHost(NetworkBearerSelectionRequest* request) = 0;

  ContextAPI* api_;
};

#endif  // NETWORKBEARERSELECTION_NETWORKBEARERSELECTION_CONTEXT_H_
