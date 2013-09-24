// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BOOKMARK_BOOKMARK_INSTANCE_H_
#define BOOKMARK_BOOKMARK_INSTANCE_H_

#include "common/extension.h"

class BookmarkInstance : public common::Instance {
 public:
  BookmarkInstance();
  virtual ~BookmarkInstance();

 private:
  // common::Instance implementation.
  virtual void HandleMessage(const char* msg);
};

#endif  // BOOKMARK_BOOKMARK_INSTANCE_H_
