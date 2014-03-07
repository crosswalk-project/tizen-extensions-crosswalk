// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BOOKMARK_BOOKMARK_INSTANCE_H_
#define BOOKMARK_BOOKMARK_INSTANCE_H_

#if defined(TIZEN)
#include <favorites.h>
#endif

#include "common/extension.h"
#include "common/picojson.h"

class BookmarkInstance : public common::Instance {
 public:
  BookmarkInstance();
  virtual ~BookmarkInstance();

  void HandleSyncMessage(const char* message);

 private:
  // common::Instance implementation.
  virtual void HandleMessage(const char* msg);

  void SetSyncReply(picojson::value v);

  const picojson::value::object
     HandleAddBookmark(const picojson::value& msg);
  const picojson::value::object
     HandleGetFolder(const picojson::value& msg, bool getParent);
  const picojson::value::object
     HandleRemoveBookmark(const picojson::value& msg);
  const picojson::value::object
     HandleRemoveAll(const picojson::value& msg);
  const picojson::value::object
     HandleGetRootID(const picojson::value& msg);
};

#endif  // BOOKMARK_BOOKMARK_INSTANCE_H_
