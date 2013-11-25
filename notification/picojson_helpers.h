// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NOTIFICATION_PICOJSON_HELPERS_H_
#define NOTIFICATION_PICOJSON_HELPERS_H_

#include <string>
#include "common/picojson.h"

picojson::value ParseJSONMessage(const char* message);

picojson::value JSONValueFromInt(int value);

bool GetIntFromJSONValue(const picojson::value& v, int* result);
void GetStringFromJSONValue(const picojson::value& v, std::string* result);
bool GetULongFromJSONValue(const picojson::value& v, uint64_t* result);
bool GetLongFromJSONValue(const picojson::value& v, int64_t* result);
void GetBoolFromJSONValue(const picojson::value& v, bool* result);

#endif  // NOTIFICATION_PICOJSON_HELPERS_H_
