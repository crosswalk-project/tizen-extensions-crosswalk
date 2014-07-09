// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_RENDERER_CALLBACKS_H_
#define MEDIA_RENDERER_CALLBACKS_H_

struct CallbackData {
  CallbackData(void* data, double async_id)
      : data_(data), async_id_(async_id) {}
  void* data_;
  double async_id_;
};

#define CALLBACK_METHOD(METHOD, SENDER, ARG0, DATA_TYPE)                       \
  static void METHOD ## CallBack(SENDER sender, ARG0 res, gpointer userdata) { \
    reinterpret_cast<DATA_TYPE*>(userdata)->METHOD(sender, res);               \
  }                                                                            \
                                                                               \
  void METHOD(SENDER, ARG0);

#define CALLBACK_METHOD_WITH_ID(METHOD, SENDER, ARG0, DATA_TYPE)               \
  static void METHOD ## CallBack(SENDER sender, ARG0 res, gpointer userdata) { \
    CallbackData* d = static_cast<CallbackData*>(userdata);                    \
    DATA_TYPE* data = reinterpret_cast<DATA_TYPE*>(d->data_);                  \
    if (!data->IsCancelled())                                                  \
      data->METHOD(sender, res, d->async_id_);                                 \
    delete d;                                                                  \
  }                                                                            \
                                                                               \
  void METHOD(SENDER, ARG0, double async_call_id);

#endif  // MEDIA_RENDERER_CALLBACKS_H_
