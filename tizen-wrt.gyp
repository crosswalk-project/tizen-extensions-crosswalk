{
  'target_defaults': {
    'conditions': [
      ['type != "mobile"', { 'sources/': [['exclude', '_mobile\\.cc$']] } ],
      ['type != "desktop"', { 'sources/': [['exclude', '_desktop\\.cc$']] } ],
      ['type == "mobile"', { 'defines': ['TIZEN_MOBILE'] } ],
      ['type == "desktop"', { 'defines': ['GENERIC_DESKTOP'] } ],
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
  },
}
