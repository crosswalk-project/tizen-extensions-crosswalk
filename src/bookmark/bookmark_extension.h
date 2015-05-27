// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BOOKMARK_BOOKMARK_EXTENSION_H_
#define BOOKMARK_BOOKMARK_EXTENSION_H_

#include "common/extension.h"

class BookmarkExtension : public common::Extension {
 public:
  BookmarkExtension();
  virtual ~BookmarkExtension();

 private:
  // common::Extension implementation.
  virtual common::Instance* CreateInstance();
};

#endif  // BOOKMARK_BOOKMARK_EXTENSION_H_
