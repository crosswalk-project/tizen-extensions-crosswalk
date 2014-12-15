// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#ifndef POWER_POWER_TYPES_H_
#define POWER_POWER_TYPES_H_

// These enums must be kept in sync with the JS object notation.
enum ResourceType {
  SCREEN = 0,
  CPU = 1,
  ResourceTypeValueCount
};

enum ResourceState {
  SCREEN_OFF = 0,
  SCREEN_DIM = 1,
  SCREEN_NORMAL = 2,
  SCREEN_BRIGHT = 3,
  CPU_AWAKE = 4,
  ResourceStateValueCount
};

#endif  // POWER_POWER_TYPES_H_
