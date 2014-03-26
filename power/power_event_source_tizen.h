// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef POWER_POWER_EVENT_SOURCE_TIZEN_H_
#define POWER_POWER_EVENT_SOURCE_TIZEN_H_

#include <power.h>
#include <list>

class PowerEventListener {
 public:
  virtual void OnPowerStateChanged(power_state_e state) = 0;
 protected:
  // We don't use listener interface to destroy them, so protected and
  // non-virtual destructor is fine.
  ~PowerEventListener() {}
};

// Track power changed events from Tizen native library. The library supports
// only one callback, so this class multiplexes to the different instances that
// maybe listening for event changes.
class PowerEventSource {
 public:
  PowerEventSource();
  ~PowerEventSource();

  void AddListener(PowerEventListener* listener);
  void RemoveListener(PowerEventListener* listener);

 private:
  // Handler for callback in power.h library.
  static void OnPowerChangedCallback(power_state_e state, void* user_data);

  void DispatchEvent(power_state_e state);

  std::list<PowerEventListener*> listeners_;
};

#endif  // POWER_POWER_EVENT_SOURCE_TIZEN_H_
