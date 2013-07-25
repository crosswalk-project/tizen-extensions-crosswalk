#!/bin/bash
# Copyright (c) 2013 Intel Corporation. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

exec xwalk "$@" --external-extensions-path=$PWD/out/Default $PWD/examples/index.html
