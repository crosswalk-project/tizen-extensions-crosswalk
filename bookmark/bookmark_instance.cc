// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bookmark/bookmark_instance.h"

#include <stdio.h>

#if defined(TIZEN_MOBILE)
#include <favorites.h>
#endif

BookmarkInstance::BookmarkInstance() {
#if defined(TIZEN_MOBILE)
  int count;
  favorites_bookmark_get_count(&count);
  printf("Bookmark count = %d\n", count);
#endif
}

BookmarkInstance::~BookmarkInstance() {}

void BookmarkInstance::HandleMessage(const char* message) {}
