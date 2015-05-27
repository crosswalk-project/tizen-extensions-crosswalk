// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bookmark/bookmark_extension.h"

#include "bookmark/bookmark_instance.h"

common::Extension* CreateExtension() {
  return new BookmarkExtension;
}

// This will be generated from bookmark_api.js.
extern const char kSource_bookmark_api[];

BookmarkExtension::BookmarkExtension() {
  SetExtensionName("tizen.bookmark");
  SetJavaScriptAPI(kSource_bookmark_api);

  const char* entry_points[] = {
    "tizen.BookmarkItem",
    "tizen.BookmarkFolder",
    NULL
  };
  SetExtraJSEntryPoints(entry_points);
}

BookmarkExtension::~BookmarkExtension() {}

common::Instance* BookmarkExtension::CreateInstance() {
  return new BookmarkInstance;
}
