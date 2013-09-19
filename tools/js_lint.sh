#!/bin/bash
# Copyright (c) 2013 Intel Corporation. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

if [ ! `which gjslint` ]; then
   echo -e "\nPlease make sure gjslint (Google Closure Lint) is in your PATH."
   echo -e "You can install it directly by \"sudo easy_install-2.7 http://closure-linter.googlecode.com/files/closure_linter-latest.tar.gz\"."
   echo -e "Or visit https://developers.google.com/closure/utilities/docs/linter_howto for more information.\n"
   exit 1
fi

if [ $# -eq 0 ]; then
   echo -e "\nUsage: $0 path_to_js_files_to_check\n"
   exit 1
fi

gjslint --strict --nojsdoc --max_line_length 100 $1
