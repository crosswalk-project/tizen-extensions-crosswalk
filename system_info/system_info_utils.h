// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_UTILS_H_
#define SYSTEM_INFO_SYSTEM_INFO_UTILS_H_

#include <libudev.h>
#include <string>

#include "common/picojson.h"

namespace system_info {

bool is_interface_on(const char* interface);
int read_one_byte(const char* path);
// free the returned value when not using
char* read_one_line(const char* path);
std::string get_udev_property(struct udev_device* dev,
                              const std::string& attr);
void SetPicoJsonObjectValue(picojson::value& obj,
                            const char* prop,
                            const picojson::value& val);

}  // system_info

#endif  // SYSTEM_INFO_SYSTEM_INFO_UTILS_H_
