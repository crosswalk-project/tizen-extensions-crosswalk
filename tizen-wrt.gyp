{
  'target_defaults': {
    'conditions': [
      ['type != "mobile"', { 'sources/': [['exclude', '_mobile\\.cc$']] } ],
      ['type != "desktop"', { 'sources/': [['exclude', '_desktop\\.cc$']] } ],
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

  # TODO: These modules still do not build properly with the mobile profile,
  #       so only enable them when building their desktop versions.
  'conditions': [
    ['type == "desktop"', {
      'includes': {
        'notification/notification.gypi',
        'power/power.gypi',
        'bluetooth/bluetooth.gypi'
      }
    }]
   ],
  'includes': {
    'time/time.gypi',
  },
}
