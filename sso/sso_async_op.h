// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SSO_SSO_ASYNC_OP_H_
#define SSO_SSO_ASYNC_OP_H_

#include <glib.h>

#include <string>

#include "common/picojson.h"
#include "common/utils.h"

namespace common {

class Instance;

}  // namespace common

class SSOAsyncOp {
 public:
  SSOAsyncOp(common::Instance* instance, picojson::value* request_data,
      gpointer user_data);
  virtual ~SSOAsyncOp();

  void PostInfo(const picojson::value& info, int jsid);
  void PostResult(const picojson::value& response, int jsid);
  void PostResult(int jsid);
  void PostError(const gchar* error, int jsid);
  void PostError(const gchar* error, const picojson::value& response, int jsid);

  picojson::value* request_data() const { return request_data_; }
  gpointer user_data() const { return user_data_; }

 private:
  picojson::value* request_data_;
  common::Instance* instance_;
  gpointer user_data_;

  double GetId() const;
  std::string GetCommand() const;

  DISALLOW_COPY_AND_ASSIGN(SSOAsyncOp);
};

#endif  // SSO_SSO_ASYNC_OP_H_
