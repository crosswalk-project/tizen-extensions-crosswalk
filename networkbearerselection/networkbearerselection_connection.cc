// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "networkbearerselection/networkbearerselection_connection.h"

#include <map>
#include <stdlib.h>
#include <vector>
#include "networkbearerselection/networkbearerselection_request.h"

typedef std::vector<NetworkBearerSelectionRequest*> RequestList;
typedef std::map<std::string, RequestList> ProfileMap;

static ProfileMap g_pending_requests;

static void add_pending_request(connection_profile_h profile,
                                NetworkBearerSelectionRequest* request) {
  char* profile_name;
  connection_profile_get_name(profile, &profile_name);
  ProfileMap::iterator iter = g_pending_requests.find(profile_name);

  if (iter == g_pending_requests.end()) {
    RequestList list;
    list.push_back(request);
    g_pending_requests[profile_name] = list;
  } else {
    iter->second.push_back(request);
  }

  free(profile_name);
}

static void connection_opened_callback(connection_error_e result, void* data) {
  if (result == CONNECTION_ERROR_NONE)
    return;

  connection_profile_h profile = static_cast<connection_profile_h>(data);

  char* profile_name;
  connection_profile_get_name(profile, &profile_name);
  ProfileMap::iterator map_iter = g_pending_requests.find(profile_name);
  free(profile_name);

  if (map_iter == g_pending_requests.end())
    return;

  RequestList* pending = &map_iter->second;
  RequestList::iterator list_iter;

  for (list_iter = pending->begin(); list_iter != pending->end(); ++list_iter)
    (*list_iter)->Failure();

  pending->clear();
  g_pending_requests.erase(map_iter);
}

static void profile_state_changed_callback(connection_profile_state_e state,
                                           void* data) {
  connection_profile_h profile = static_cast<connection_profile_h>(data);

  char* profile_name;
  connection_profile_get_name(profile, &profile_name);
  ProfileMap::iterator map_iter = g_pending_requests.find(profile_name);
  free(profile_name);

  if (map_iter == g_pending_requests.end())
    return;

  RequestList* pending = &map_iter->second;
  RequestList::iterator list_iter;

  if (state == CONNECTION_PROFILE_STATE_CONNECTED) {
    for (list_iter = pending->begin(); list_iter != pending->end(); ++list_iter)
      (*list_iter)->Success();
    // TODO(tmpsantos): Add route to host here.
  } else if (state == CONNECTION_PROFILE_STATE_DISCONNECTED) {
    for (list_iter = pending->begin(); list_iter != pending->end(); ++list_iter)
      (*list_iter)->Disconnected();

    pending->clear();
    g_pending_requests.erase(map_iter);
  }
}

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
  connection_profile_h profile =
      GetProfileForNetworkType(request->network_type());
  if (!profile || !is_valid()) {
    request->Failure();
    delete request;
    return;
  }

  connection_profile_state_e state;
  int ret = connection_profile_get_state(profile, &state);
  if (ret != CONNECTION_ERROR_NONE) {
    request->Failure();
    delete request;
    return;
  }

  connection_profile_set_state_changed_cb(profile,
                                          profile_state_changed_callback,
                                          profile);

  if (state == CONNECTION_PROFILE_STATE_CONNECTED) {
    // TODO(tmpsantos): Add route to host here.
    add_pending_request(profile, request);
    request->Success();
    return;
  }

  if (state == CONNECTION_PROFILE_STATE_DISCONNECTED) {
    ret = connection_open_profile(connection_, profile,
                                  connection_opened_callback, profile);
    if (ret != CONNECTION_ERROR_NONE) {
      request->Failure();
      delete request;
      return;
    }
  }

  add_pending_request(profile, request);
  request->Success();
}

connection_profile_h NetworkBearerSelectionConnection::GetProfileForNetworkType(
    NetworkType network_type) {
  // FIXME(tmpsantos): I'm not really sure if we should really map the UNKNOWN
  // type to the wireless. Looks to me that it should have its own category,
  // although the spec doesn't say anything about it.
  connection_profile_type_e expected_profile_type;
  switch(network_type) {
    case CELLULAR:
      expected_profile_type = CONNECTION_PROFILE_TYPE_CELLULAR;
      break;
    case UNKNOWN:
      expected_profile_type = CONNECTION_PROFILE_TYPE_WIFI;
      break;
  }

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
    if (profile_type == expected_profile_type)
      return profile;
  }

  connection_destroy_profile_iterator(profile_iter);
  return NULL;
}
