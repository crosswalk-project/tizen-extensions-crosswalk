// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sso/sso_async_op.h"

#include "common/extension.h"

SSOAsyncOp::SSOAsyncOp(common::Instance* instance,
    picojson::value* request_data, gpointer user_data)
    : instance_(instance), request_data_(request_data), user_data_(user_data) {
}

SSOAsyncOp::~SSOAsyncOp() {
  if (request_data_)
    delete request_data_;
  request_data_ = NULL;
}

void SSOAsyncOp::PostInfo(const picojson::value& info, int jsid) {
  picojson::value::object object;
  object["info"] = info;
  object["objectJSId"] = picojson::value(static_cast<double>(jsid));
  picojson::value value(object);
  instance_->PostMessage(value.serialize().c_str());
}

void SSOAsyncOp::PostResult(const picojson::value& response, int jsid) {
  picojson::value::object object;
  object["asyncOpCmd"] = picojson::value(GetCommand());
  object["asyncOpId"] = picojson::value(GetId());
  object["objectJSId"] = picojson::value(static_cast<double>(jsid));
  object["responseData"] = response;
  picojson::value value(object);
  instance_->PostMessage(value.serialize().c_str());
}

void SSOAsyncOp::PostResult(int jsid) {
  picojson::value::object object;
  object["asyncOpCmd"] = picojson::value(GetCommand());
  object["asyncOpId"] = picojson::value(GetId());
  object["objectJSId"] = picojson::value(static_cast<double>(jsid));
  picojson::value value(object);
  instance_->PostMessage(value.serialize().c_str());
}

void SSOAsyncOp::PostError(const gchar* error, int jsid) {
  picojson::value::object object;
  object["asyncOpCmd"] = picojson::value(GetCommand());
  object["asyncOpId"] = picojson::value(GetId());
  object["asyncOpErrorMsg"] = picojson::value(error);
  object["objectJSId"] = picojson::value(static_cast<double>(jsid));
  picojson::value value(object);
  instance_->PostMessage(value.serialize().c_str());
}

void SSOAsyncOp::PostError(const gchar* error, const picojson::value& response,
    int jsid) {
  picojson::value::object object;
  object["asyncOpCmd"] = picojson::value(GetCommand());
  object["asyncOpId"] = picojson::value(GetId());
  object["asyncOpErrorMsg"] = picojson::value(error);
  object["objectJSId"] = picojson::value(static_cast<double>(jsid));
  object["responseData"] = response;
  picojson::value value(object);
  instance_->PostMessage(value.serialize().c_str());
}

double SSOAsyncOp::GetId() const {
  if (!request_data_)
    return -1;
  return request_data_->get("asyncOpId").get<double>();
}

std::string SSOAsyncOp::GetCommand() const {
  if (!request_data_)
    return std::string("Unknown");
  return request_data_->get("asyncOpCmd").get<std::string>();
}
