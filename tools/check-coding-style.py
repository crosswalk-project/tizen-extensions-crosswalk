#!/usr/bin/env python
# Copyright (c) 2013 Intel Corporation. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file

import os.path
import subprocess
import sys

d = os.path.dirname(os.path.realpath(__file__))
p = subprocess.Popen([os.path.join(d, 'check-coding-style')],
                     stdout=subprocess.PIPE)
stdout = p.communicate()[0]
sys.exit(p.returncode)
