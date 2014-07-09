// Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DATASYNC_DATASYNC_SCOPED_EXIT_H_
#define DATASYNC_DATASYNC_SCOPED_EXIT_H_

#include <functional>

namespace datasync {

class ScopedExit {
 public:
  explicit ScopedExit(std::function<void()> func) : func_(func) { }
  ~ScopedExit() { func_(); }
 private:
  std::function<void()> func_;
};

}  // namespace datasync

#endif  // DATASYNC_DATASYNC_SCOPED_EXIT_H_
