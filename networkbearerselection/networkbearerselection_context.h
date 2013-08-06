// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NETWORKBEARERSELECTION_NETWORKBEARERSELECTION_CONTEXT_H_
#define NETWORKBEARERSELECTION_NETWORKBEARERSELECTION_CONTEXT_H_

#include <string>
#include "common/extension_adapter.h"

class NetworkBearerSelectionContext {
 public:
  NetworkBearerSelectionContext(ContextAPI* api);

  // ExtensionAdapter implementation.
  static const char name[];
  static const char* GetJavaScript();
  void HandleMessage(const char* message);
  void HandleSyncMessage(const char* message) {}

 private:
  enum NetworkType {
      CELLULAR = 0,
      UNKNOWN = 1,
  };

  // FIXME(tmpsantos): Move this to a common place and write a full
  // list. We should have an equivalent on
  enum WebApiAPIErrors {
    UnknownError = 0,
    NotSupportedError = 9,
  };

  void OnRequestRouteToHost(const std::string& cmd,
                            const std::string& domain_name,
                            NetworkType network_type,
                            const std::string& reply_id);
  void OnReleaseRouteToHost(const std::string& cmd,
                            const std::string& domain_name,
                            NetworkType network_type,
                            const std::string& reply_id);

  ContextAPI* api_;
};

#endif  // NETWORKBEARERSELECTION_NETWORKBEARERSELECTION_CONTEXT_H_
