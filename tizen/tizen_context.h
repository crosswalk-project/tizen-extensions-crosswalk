// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TIZEN_TIZEN_CONTEXT_H_
#define TIZEN_TIZEN_CONTEXT_H_

#include "common/extension_adapter.h"

class TizenContext {
 public:
  explicit TizenContext(ContextAPI* context_api);
  ~TizenContext();

  // ExtensionAdapter implementation.
  static const char name[];
  static const char* GetJavaScript();
  void HandleMessage(const char*) {}
  void HandleSyncMessage(const char*) {}

 private:
  ContextAPI* api_;
};

#endif  // TIZEN_TIZEN_CONTEXT_H_
