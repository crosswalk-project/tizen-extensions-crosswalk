// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSION_ADAPTER_H_
#define EXTENSION_ADAPTER_H_

#include <cstdlib>
#include "xwalk_extension_public.h"

class ContextAPI {
 public:
  explicit ContextAPI(CXWalkExtensionContext* context) : context_(context) {}
  void PostMessage(const char* message) {
    xwalk_extension_context_post_message(context_, message);
  }

 private:
  CXWalkExtensionContext* context_;
};

template <class T>
class ExtensionAdapter {
 public:
  static CXWalkExtension* Initialize();

 private:
  struct Context {
    CXWalkExtensionContext context;
    T* cpp_context;
  };

  static const char* GetJavaScript(CXWalkExtension* extension);
  static void Shutdown(CXWalkExtension* extension);
  static CXWalkExtensionContext* ContextCreate(CXWalkExtension* extension);
  static void ContextHandleMessage(CXWalkExtensionContext* context,
                                   const char* message);
  static void ContextDestroy(CXWalkExtensionContext* context);
};

template <class T>
CXWalkExtension* ExtensionAdapter<T>::Initialize() {
  CXWalkExtension* xwalk_extension =
      static_cast<CXWalkExtension*>(calloc(1, sizeof(CXWalkExtension)));
  xwalk_extension->name = T::name;
  xwalk_extension->api_version = 1;
  xwalk_extension->get_javascript = GetJavaScript;
  xwalk_extension->shutdown = Shutdown;
  xwalk_extension->context_create = ContextCreate;
  return xwalk_extension;
}

template <class T>
const char* ExtensionAdapter<T>::GetJavaScript(CXWalkExtension* extension) {
  return T::GetJavaScript();
}

template <class T>
void ExtensionAdapter<T>::Shutdown(CXWalkExtension* extension) {
  free(extension);
}

template <class T>
CXWalkExtensionContext* ExtensionAdapter<T>::ContextCreate(
    CXWalkExtension* extension) {
  Context* adapter = static_cast<Context*>(calloc(1, sizeof(Context)));
  if (!adapter)
    return NULL;

  adapter->context.destroy = ContextDestroy;
  adapter->context.handle_message = ContextHandleMessage;
  adapter->cpp_context = new T(new ContextAPI(&adapter->context));

  return reinterpret_cast<CXWalkExtensionContext*>(adapter);
}

template <class T>
void ExtensionAdapter<T>::ContextHandleMessage(
    CXWalkExtensionContext* context, const char* message) {
  Context* adapter = reinterpret_cast<Context*>(context);
  adapter->cpp_context->HandleMessage(message);
}

template <class T>
void ExtensionAdapter<T>::ContextDestroy(CXWalkExtensionContext* context) {
  Context* adapter = reinterpret_cast<Context*>(context);
  delete adapter->cpp_context;
  free(adapter);
}

#define DEFINE_XWALK_EXTENSION(NAME)                            \
  CXWalkExtension* xwalk_extension_init(int32_t api_version) {  \
    return ExtensionAdapter<NAME>::Initialize();                \
  }

#endif  // EXTENSION_ADAPTER_H_
