// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sso/sso_extension.h"

#include <glib-object.h>

#include "sso/sso_instance.h"

common::Extension* CreateExtension() {
#if !GLIB_CHECK_VERSION (2, 36, 0)
  g_type_init();
#endif
  return new SSOExtension();
}

extern const char kSource_sso_api[];

SSOExtension::SSOExtension() {
  SetExtensionName("tizen.sso");
  SetJavaScriptAPI(kSource_sso_api);
}

SSOExtension::~SSOExtension() {
}

common::Instance* SSOExtension::CreateInstance() {
  return new SSOInstance();
}
