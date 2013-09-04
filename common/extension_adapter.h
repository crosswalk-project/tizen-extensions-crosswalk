// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMMON_EXTENSION_ADAPTER_H_
#define COMMON_EXTENSION_ADAPTER_H_

#include <cstdlib>
#include <map>
#include "common/XW_Extension.h"
#include "common/XW_Extension_SyncMessage.h"

namespace internal {

int32_t InitializeExtension(XW_Extension extension,
                         XW_GetInterface get_interface,
                         const char* name,
                         const char* api,
                         XW_CreatedInstanceCallback created,
                         XW_DestroyedInstanceCallback destroyed,
                         XW_HandleMessageCallback handle_message,
                         XW_HandleSyncMessageCallback handle_sync_message);

void PostMessage(XW_Instance instance, const char* message);
void SetSyncReply(XW_Instance instance, const char* reply);

}  // namespace internal

class ContextAPI {
 public:
  explicit ContextAPI(XW_Instance instance) : instance_(instance) {}

  void PostMessage(const char* message) {
    internal::PostMessage(instance_, message);
  }
  void SetSyncReply(const char* reply) {
    internal::SetSyncReply(instance_, reply);
  }

 private:
  XW_Instance instance_;
};

template <class T>
class ExtensionAdapter {
 public:
  static int32_t Initialize(XW_Extension extension,
                            XW_GetInterface get_interface);

 private:
  static void DidCreateInstance(XW_Instance instance);
  static void DidDestroyInstance(XW_Instance instance);

  static void HandleMessage(XW_Instance instance, const char* message);
  static void HandleSyncMessage(XW_Instance instance, const char* message);

  typedef std::map<XW_Instance, T*> InstanceMap;
  static InstanceMap g_instances;
};

template <class T>
typename ExtensionAdapter<T>::InstanceMap ExtensionAdapter<T>::g_instances;

template <class T>
int32_t ExtensionAdapter<T>::Initialize(XW_Extension extension,
                                        XW_GetInterface get_interface) {
  return internal::InitializeExtension(
      extension, get_interface, T::name, T::GetJavaScript(),
      DidCreateInstance, DidDestroyInstance, HandleMessage, HandleSyncMessage);
}

template <class T>
void ExtensionAdapter<T>::DidCreateInstance(XW_Instance instance) {
  g_instances[instance] = new T(new ContextAPI(instance));
}

template <class T>
void ExtensionAdapter<T>::DidDestroyInstance(XW_Instance instance) {
  delete g_instances[instance];
  g_instances.erase(instance);
}

template <class T>
void ExtensionAdapter<T>::HandleMessage(XW_Instance instance,
                                        const char* message) {
  g_instances[instance]->HandleMessage(message);
}


template <class T>
void ExtensionAdapter<T>::HandleSyncMessage(XW_Instance instance,
                                            const char* message) {
  g_instances[instance]->HandleSyncMessage(message);
}

#define DEFINE_XWALK_EXTENSION(NAME)                                    \
  int32_t XW_Initialize(XW_Extension extension,                         \
                        XW_GetInterface get_interface) {                \
    return ExtensionAdapter<NAME>::Initialize(extension, get_interface); \
  }

#endif  // COMMON_EXTENSION_ADAPTER_H_
