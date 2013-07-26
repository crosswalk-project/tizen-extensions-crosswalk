// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

namespace system_info {

bool is_interface_on(const char* interface);
int read_one_byte(const char* path);
// free the returned value when not using
char* read_one_line(const char* path);

}  // system_info
