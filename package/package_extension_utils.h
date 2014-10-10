// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PACKAGE_PACKAGE_EXTENSION_UTILS_H_
#define PACKAGE_PACKAGE_EXTENSION_UTILS_H_

#include "common/picojson.h"
#include "tizen/tizen.h"

inline picojson::value* CreateResultMessage() {
  return new picojson::value(picojson::object());
}

inline picojson::value* CreateResultMessage(WebApiAPIErrors error) {
  picojson::object obj;
  obj["error"] = picojson::value(static_cast<double>(error));
  return new picojson::value(obj);
}

inline picojson::value* CreateResultMessage(const picojson::array& data) {
  picojson::object obj;
  obj["data"] = picojson::value(data);
  return new picojson::value(obj);
}

inline picojson::value* CreateResultMessage(const picojson::value& data) {
  picojson::object obj;
  obj["data"] = data;
  return new picojson::value(obj);
}

#endif  // PACKAGE_PACKAGE_EXTENSION_UTILS_H_
