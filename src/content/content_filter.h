// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CONTENT_FILTER_H_
#define CONTENT_CONTENT_FILTER_H_

#include <string>
#include "common/picojson.h"

// TODO(spoussa): Once more filters are impllemnted this class will enhance.
class ContentFilter {
 public:
  static ContentFilter& instance();
  std::string convert(const picojson::value &jsonFilter);

 private:
  ContentFilter() {}
};

#endif  // CONTENT_CONTENT_FILTER_H_
