// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "network_bearer_selection/network_bearer_selection_connection_tizen.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "network_bearer_selection/network_bearer_selection_request.h"

typedef std::vector<NetworkBearerSelectionRequest*> RequestList;
typedef std::map<std::string, RequestList> ProfileMap;

namespace {

static ProfileMap g_pending_requests;

bool add_route(connection_h connection, connection_profile_h profile,
    const std::string& domain_name) {
  // FIXME(tmpsantos): This is wrong, but it is exactly what the Tizen
  // runtime is doing, so it was kept for compatibility.
  // - Routes should not be programmatically defined by the app, it has
  //   a strong chance of messing with the environment as a whole.
  // - Allowing apps to do this is probably not safe.
  // - We add a route just for a single IP, but a hostname might resolve
  //   to several IPs.
  struct hostent *host_entry = gethostbyname(domain_name.c_str());
  if (!host_entry)
    return false;

  char* ip_address = inet_ntoa(
      *reinterpret_cast<struct in_addr*>(host_entry->h_addr));
  if (!ip_address)
    return false;

  char *interface_name;
  connection_profile_get_network_interface_name(profile, &interface_name);
  if (!interface_name)
    return false;

  // This is issuing a GLib-CRITICAL on Tizen 2.1 for not apparent reason.
  connection_add_route(connection, interface_name, ip_address);

  free(interface_name);

  return true;
}

void dummy_callback(connection_error_e result, void* data) {}

void profile_state_changed_callback(connection_profile_state_e state,
                                    void* data) {
  if (state != CONNECTION_PROFILE_STATE_DISCONNECTED)
    return;

  std::pair<connection_profile_h, char*>* data_pair =
      static_cast<std::pair<connection_profile_h, char*>*>(data);

  connection_profile_h profile = data_pair->first;
  char* profile_name = data_pair->second;

  ProfileMap::iterator map_iter = g_pending_requests.find(profile_name);
  if (map_iter == g_pending_requests.end())
    return;

  RequestList* pending = &map_iter->second;
  RequestList::iterator list_iter;

  list_iter = pending->begin();
  while (list_iter != pending->end()) {
    (*list_iter)->Disconnected();
    delete (*list_iter);
    list_iter = pending->erase(list_iter);
  }

  pending->clear();
  g_pending_requests.erase(map_iter);

  // FIXME(tmpsantos): This is sad, but the callback gets called sometimes even
  // after calling the "unset" callback method. If we free the resources, we end
  // up by accessing corrupted memory. Yep, this is an intentional memory leak.
  //
  //  free(profile_name);
  //  delete data_pair;

  connection_profile_unset_state_changed_cb(profile);
}

void add_pending_request(connection_h connection,
                         connection_profile_h profile,
                         NetworkBearerSelectionRequest* request) {
  char* profile_name;
  connection_profile_get_name(profile, &profile_name);
  ProfileMap::iterator iter = g_pending_requests.find(profile_name);

  if (iter == g_pending_requests.end()) {
    RequestList list;
    list.push_back(request);
    g_pending_requests[profile_name] = list;

    // FIXME(tmpsantos): It might sound redundant passing the "profile" and
    // the "profile_name" because you can get the "profile_name" from the
    // "profile". For some unknown reason, you might get different profile names
    // at each call.
    std::pair<connection_profile_h, char*>* data = new
        std::pair<connection_profile_h, char*>(profile, profile_name);

    connection_profile_set_state_changed_cb(profile,
                                            profile_state_changed_callback,
                                            data);
  } else {
    iter->second.push_back(request);
    free(profile_name);
  }
}

}  // namespace

NetworkBearerSelectionConnection::NetworkBearerSelectionConnection()
    : is_valid_(false) {
  int ret = connection_create(&connection_);
  if (ret != CONNECTION_ERROR_NONE)
    return;

  is_valid_ = true;
}

NetworkBearerSelectionConnection::~NetworkBearerSelectionConnection() {
  if (!is_valid())
    return;
  connection_destroy(connection_);
}

void NetworkBearerSelectionConnection::RequestRouteToHost(
    NetworkBearerSelectionRequest* request) {
  std::unique_ptr<NetworkBearerSelectionRequest> request_ptr(request);
  if (!is_valid()) {
    request_ptr->Failure();
    return;
  }

  connection_profile_h profile =
      GetProfileForNetworkType(request_ptr->network_type());
  if (!profile) {
    request_ptr->Failure();
    return;
  }

  connection_profile_state_e state;
  int ret = connection_profile_get_state(profile, &state);
  if (ret != CONNECTION_ERROR_NONE) {
    request_ptr->Failure();
    return;
  }

  // If you don't call connection_open_profile (even though the connection
  // state is already "CONNECTED") you don't get the disconnect event. The API
  // documentation is really vague about this.
  connection_open_profile(connection_, profile, dummy_callback, NULL);

  if (state == CONNECTION_PROFILE_STATE_CONNECTED) {
    if (add_route(connection_, profile, request_ptr->domain_name())) {
      request_ptr->Success();
      add_pending_request(connection_, profile, request_ptr.release());
      return;
    }
  }

  request_ptr->Failure();
}

void NetworkBearerSelectionConnection::ReleaseRouteToHost(
    NetworkBearerSelectionRequest* request) {
  std::unique_ptr<NetworkBearerSelectionRequest> request_ptr(request);

  if (!is_valid()) {
    request_ptr->Failure();
    return;
  }

  connection_profile_h profile =
      GetProfileForNetworkType(request_ptr->network_type());
  if (!profile) {
    request_ptr->Failure();
    return;
  }

  // FIXME(tmpsantos): Tizen runtime implements the "release route" by simply
  // disconnecting the network interface. We are doing the same here for
  // compatibility, but it is obviously wrong.
  if (connection_close_profile(
          connection_, profile, dummy_callback, NULL) == CONNECTION_ERROR_NONE)
    request_ptr->Success();
  else
    request_ptr->Failure();
}

connection_profile_h NetworkBearerSelectionConnection::GetProfileForNetworkType(
    NetworkType network_type) {
  connection_profile_iterator_h profile_iter;
  int ret = connection_get_profile_iterator(connection_,
                                            CONNECTION_ITERATOR_TYPE_REGISTERED,
                                            &profile_iter);
  if (ret != CONNECTION_ERROR_NONE)
    return NULL;

  connection_profile_h profile;
  while (connection_profile_iterator_has_next(profile_iter)) {
    ret = connection_profile_iterator_next(profile_iter, &profile);
    if (ret != CONNECTION_ERROR_NONE)
      break;

    connection_profile_type_e profile_type;
    connection_profile_get_type(profile, &profile_type);

    // FIXME(tmpsantos): I'm not really sure if we should really map the UNKNOWN
    // type to the wireless. Looks to me that it should have its own category,
    // although the spec doesn't say anything about it.
    connection_profile_type_e expected_type = CONNECTION_PROFILE_TYPE_WIFI;
    if (network_type == CELLULAR)
      expected_type = CONNECTION_PROFILE_TYPE_CELLULAR;

    if (profile_type == expected_type)
      return profile;
  }

  connection_destroy_profile_iterator(profile_iter);
  return NULL;
}
