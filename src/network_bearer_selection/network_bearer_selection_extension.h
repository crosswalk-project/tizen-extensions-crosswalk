// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NETWORK_BEARER_SELECTION_NETWORK_BEARER_SELECTION_EXTENSION_H_
#define NETWORK_BEARER_SELECTION_NETWORK_BEARER_SELECTION_EXTENSION_H_

#include "common/extension.h"

class NetworkBearerSelectionExtension : public common::Extension {
 public:
  NetworkBearerSelectionExtension();
  virtual ~NetworkBearerSelectionExtension();

 private:
  // common::Extension implementation.
  virtual common::Instance* CreateInstance();
};

#endif  // NETWORK_BEARER_SELECTION_NETWORK_BEARER_SELECTION_EXTENSION_H_
