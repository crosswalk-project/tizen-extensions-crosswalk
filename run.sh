#!/bin/bash
# Copyright (c) 2013 Intel Corporation. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

if [ ! -e $XWALK_SOURCE_DIR/xwalk.gyp ]; then
   echo "Please set XWALK_SOURCE_DIR to the directory containing xwalk.gyp."
   exit 1
fi

exec $XWALK_SOURCE_DIR/../out/Release/xwalk --external-extensions-path=$PWD/out/Default $PWD/index.html
