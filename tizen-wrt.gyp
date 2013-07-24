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
      '<(xwalk_path)/extensions/public',
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
    'notification/notification.gypi',
    'time/time.gypi',
  },
}
