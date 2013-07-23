{
  'target_defaults': {
    'conditions': [
      ['type != "mobile"', { 'sources/': [['exclude', '_mobile\\.cc$|mobile/']] } ],
      ['type != "desktop"', { 'sources/': [['exclude', '_desktop\\.cc$|desktop/']] } ],
      ['type == "mobile"', { 'defines': ['TIZEN_MOBILE'] } ],
      ['type == "desktop"', { 'defines': ['GENERIC_DESKTOP'] } ],
      ['build == "Debug"', {
        'defines': ['_DEBUG', ],
        'cflags': [ '-O0', '-g', ],
      }],
      ['build == "Release"', {
        'defines': ['NDEBUG', ],
        'cflags': [
          '-O2'
          # Don't emit the GCC version ident directives, they just end up
          # in the .comment section taking up binary size.
          '-fno-ident',
          # Put data and code in their own sections, so that unused symbols
          # can be removed at link time with --gc-sections.
          '-fdata-sections',
          '-ffunction-sections',
        ],
      }],
    ],
    'includes': [
      'xwalk_js2c.gypi',
    ],
    'include_dirs': [
      '.',
    ],
    'sources': [
      '../common/extension_adapter.h',
      '../common/picojson.h',
    ],
    'cflags': [
      '-fPIC',
      '-fvisibility=hidden',
    ],
  },
  'includes': {
    'bluetooth/bluetooth.gypi',
    'notification/notification.gypi',
    'power/power.gypi',
    'system_info/system_info.gypi',
    'tizen/tizen.gypi',
    'time/time.gypi',
  },
}
