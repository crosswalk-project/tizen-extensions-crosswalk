#!/usr/bin/env python

# Copyright (c) 2014 Intel Corporation. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Supported encodings (iconv), with aliases.
The spec seems to prefer asynchronous checks (more efficient)
but the TCT tests expect immediate checking of encoding support,
so we have to inject iconv supported encodings into the js source.
"""


import subprocess, json, sys

raw_encodings = subprocess.check_output(["iconv", "-l"])
encodings = dict((line.split('/')[0], 1) for line in raw_encodings.split() if line)
src = open(sys.argv[1]).readlines()
dest = open(sys.argv[2], 'w')
for line in src:
    if line.startswith("var encodings = "):
        dest.write("var encodings = %s;\n" % json.dumps(encodings))
    else:
        dest.write(line)
dest.close()
exit(0)
