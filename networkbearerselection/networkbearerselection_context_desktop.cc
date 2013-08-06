// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "networkbearerselection/networkbearerselection_context.h"

#include "common/picojson.h"

void NetworkBearerSelectionContext::OnRequestRouteToHost(
    const std::string& cmd, const std::string& domain_name,
    NetworkType network_type, const std::string& reply_id) {
  picojson::value::object o;

  o["cmd"] = picojson::value(cmd);
  o["error"] = picojson::value(static_cast<double>(NotSupportedError));
  o["disconnected"] = picojson::value(false);
  o["reply_id"] = picojson::value(reply_id);

  picojson::value v(o);
  api_->PostMessage(v.serialize().c_str());
}

void NetworkBearerSelectionContext::OnReleaseRouteToHost(
    const std::string& cmd, const std::string& domain_name,
    NetworkType network_type, const std::string& reply_id) {
  picojson::value::object o;

  o["cmd"] = picojson::value(cmd);
  o["error"] = picojson::value(static_cast<double>(NotSupportedError));
  o["disconnected"] = picojson::value(false);
  o["reply_id"] = picojson::value(reply_id);

  picojson::value v(o);
  api_->PostMessage(v.serialize().c_str());
}
