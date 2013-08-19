# Copyright (c) 2013 Intel Corporation. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys, os, re, errno

spec = sys.argv[1]
origin = sys.argv[2]

if len(sys.argv) != 8:
    print 'Invalid arguments'
    sys.exit(errno.EINVAL)

if not os.path.isfile(spec) or not os.path.isfile(origin):
    print 'No such spec(%s) or input xml(%s) file' % (spec, origin)
    sys.exit(errno.ENOENT)

spec = file(spec).read()
version = re.search('(?<=Version:).*', spec).group(0).strip()
package_name = re.search('(?<=%define '+sys.argv[3]+').*', spec).group(0).strip()

template = file(origin).read().replace('@PACKAGE_VERSION@', version)
template = template.replace('@PACKAGE_NAME@', package_name)
template = template.replace('@APP_ID@', sys.argv[4])
template = template.replace('@PACKAGE_EXEC@', sys.argv[5])
template = template.replace('@PACKAGE_LABEL@', sys.argv[6])

output = open(sys.argv[7], 'w')
output.write(template)
output.close()
