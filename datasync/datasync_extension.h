// Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DATASYNC_DATASYNC_EXTENSION_H_
#define DATASYNC_DATASYNC_EXTENSION_H_

#include "common/extension.h"
#include "datasync/datasync_manager.h"

namespace datasync {

class DatasyncExtension : public common::Extension {
 public:
  DatasyncExtension();
  virtual ~DatasyncExtension();

  DataSyncManager& manager();

 private:
  // common::Extension implementation.
  virtual common::Instance* CreateInstance();
};

}  // namespace datasync

#endif  // DATASYNC_DATASYNC_EXTENSION_H_
