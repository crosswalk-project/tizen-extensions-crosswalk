// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "power/power_event_source_tizen.h"

#include <algorithm>
#include <functional>

PowerEventSource::PowerEventSource() {}

PowerEventSource::~PowerEventSource() {
  if (!listeners_.empty())
    power_unset_changed_cb();
}

void PowerEventSource::AddListener(PowerEventListener* listener) {
  listeners_.push_front(listener);
  if (listeners_.size() == 1)
    power_set_changed_cb(OnPowerChangedCallback, this);
}

void PowerEventSource::RemoveListener(PowerEventListener* listener) {
  listeners_.remove(listener);
  if (listeners_.empty())
    power_unset_changed_cb();
}

// static
void PowerEventSource::OnPowerChangedCallback(
    power_state_e state, void* user_data) {
  PowerEventSource* source = reinterpret_cast<PowerEventSource*>(user_data);
  source->DispatchEvent(state);
}

void PowerEventSource::DispatchEvent(power_state_e state) {
  auto changed_fun = std::mem_fun(&PowerEventListener::OnPowerStateChanged);
  std::for_each(listeners_.begin(), listeners_.end(),
                std::bind2nd(changed_fun, state));
}
