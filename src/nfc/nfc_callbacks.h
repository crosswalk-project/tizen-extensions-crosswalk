// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NFC_NFC_CALLBACKS_H_
#define NFC_NFC_CALLBACKS_H_

#include <string>

typedef struct _ndef_message_s *nfc_ndef_message_h;

struct CallbackData {
  CallbackData(void* data, double async_id,
      const std::string& event_name, nfc_ndef_message_h message = 0)
      : data_(data),
        async_id_(async_id),
        event_name_(event_name),
        message_(message) {}
  void* data_;
  double async_id_;
  std::string event_name_;
  nfc_ndef_message_h message_;
};

#define CALLBACK_METHOD(METHOD, ARG0, DATA_TYPE)                               \
  static void METHOD ## CallBack(ARG0 res, void* userdata) {                   \
    reinterpret_cast<DATA_TYPE*>(userdata)->METHOD(res);                       \
  }                                                                            \
                                                                               \
  void METHOD(ARG0);

#define CALLBACK_METHOD_2(METHOD, ARG0, ARG1, DATA_TYPE)                       \
  static void METHOD ## CallBack(ARG0 arg0, ARG1 arg1, void* userdata) {       \
    reinterpret_cast<DATA_TYPE*>(userdata)->METHOD(arg0, arg1);                \
  }                                                                            \
                                                                               \
  void METHOD(ARG0, ARG1);

#define CALLBACK_METHOD_WITH_DATA(METHOD, ARG0, DATA_TYPE)                     \
  static void METHOD ## CallBack(ARG0 res, void* userdata) {                   \
    CallbackData* d = static_cast<CallbackData*>(userdata);                    \
    DATA_TYPE* data = reinterpret_cast<DATA_TYPE*>(d->data_);                  \
    data->METHOD(res, d);                                                      \
    delete d;                                                                  \
  }                                                                            \
                                                                               \
  void METHOD(ARG0, CallbackData* cd);

#define CALLBACK_METHOD_WITH_DATA_2(METHOD, ARG0, ARG1, DATA_TYPE)             \
  static void METHOD ## CallBack(ARG0 arg0, ARG1 arg1, void* userdata) {       \
    CallbackData* d = static_cast<CallbackData*>(userdata);                    \
    DATA_TYPE* data = reinterpret_cast<DATA_TYPE*>(d->data_);                  \
    data->METHOD(arg0, arg1, d);                                               \
    delete d;                                                                  \
  }                                                                            \
                                                                               \
  void METHOD(ARG0, ARG1, CallbackData* cd);

#endif  // NFC_NFC_CALLBACKS_H_
