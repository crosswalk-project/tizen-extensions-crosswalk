// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/extension_adapter.h"

#include <iostream>

namespace {

XW_Extension g_extension = 0;

const XW_CoreInterface* g_core = NULL;
const XW_MessagingInterface* g_messaging = NULL;
const XW_Internal_SyncMessagingInterface* g_sync_messaging = NULL;

}  // namespace

namespace internal {

int32_t InitializeExtension(XW_Extension extension,
                            XW_GetInterface get_interface,
                            const char* name,
                            const char* api,
                            XW_CreatedInstanceCallback created,
                            XW_DestroyedInstanceCallback destroyed,
                            XW_HandleMessageCallback handle_message,
                            XW_HandleSyncMessageCallback handle_sync_message) {
  if (g_extension != 0) {
    std::cerr << "Can't initialize same extension multiple times!\n";
    return XW_ERROR;
  }

  g_extension = extension;

  g_core = reinterpret_cast<const XW_CoreInterface*>(
      get_interface(XW_CORE_INTERFACE));
  if (!g_core) {
    std::cerr << "Can't initialize extension: error getting Core interface.\n";
    return XW_ERROR;
  }
  g_core->SetExtensionName(extension, name);
  g_core->SetJavaScriptAPI(extension, api);
  g_core->RegisterInstanceCallbacks(extension, created, destroyed);

  g_messaging = reinterpret_cast<const XW_MessagingInterface*>(
      get_interface(XW_MESSAGING_INTERFACE));
  if (!g_messaging) {
    std::cerr <<
        "Can't initialize extension: error getting Messaging interface.\n";
    return XW_ERROR;
  }
  g_messaging->Register(extension, handle_message);

  g_sync_messaging =
      reinterpret_cast<const XW_Internal_SyncMessagingInterface*>(
          get_interface(XW_INTERNAL_SYNC_MESSAGING_INTERFACE));
  if (!g_sync_messaging) {
    std::cerr <<
        "Can't initialize extension: error getting Messaging interface.\n";
    return XW_ERROR;
  }
  g_sync_messaging->Register(extension, handle_sync_message);

  return XW_OK;
}

void PostMessage(XW_Instance instance, const char* message) {
  g_messaging->PostMessage(instance, message);
}

void SetSyncReply(XW_Instance instance, const char* reply) {
  g_sync_messaging->SetSyncReply(instance, reply);
}

}  // namespace internal
