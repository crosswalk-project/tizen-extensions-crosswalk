#!/bin/bash
# Copyright (c) 2013 Intel Corporation. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

if [ ! `which xwalk` ]; then
   echo -e "\nPlease make sure xwalk is in your PATH. It is usually found at XWALK_SOURCE_DIR/../out/Release/"
   exit 1
fi

exec xwalk "$@" --external-extensions-path=$PWD/out/Default $PWD/examples/index.html
