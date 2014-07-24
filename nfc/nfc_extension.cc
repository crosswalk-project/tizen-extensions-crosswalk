// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "nfc/nfc_extension.h"

#include "nfc/nfc_instance.h"

common::Extension* CreateExtension() {
  return new NfcExtension;
}

extern const char kSource_nfc_api[];

NfcExtension::NfcExtension() {
  SetExtensionName("navigator.nfc");
  SetJavaScriptAPI(kSource_nfc_api);
  const char* entry_points[] = {
    "window.NDEFMessage",
    "window.NDEFRecord",
    "window.NDEFRecordText",
    "window.NDEFRecordURI",
    "window.NDEFRecordMedia",
    "window.NDEFRecordSmartPoster",
    NULL
  };
  SetExtraJSEntryPoints(entry_points);
}

NfcExtension::~NfcExtension() {}

common::Instance* NfcExtension::CreateInstance() {
  return new NfcInstance;
}
