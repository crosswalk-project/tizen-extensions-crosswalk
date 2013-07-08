{
  'targets': [
    {
      'target_name': 'tizen_notification',
      'type': 'loadable_module',
      'sources': [
        'common/extension_adapter.h',
	'common/picojson.h',
        'notification/notification_api.js',
        'notification/notification_context.cc',
        'notification/notification_context.h',
      ],
      'includes': [
       'xwalk_js2c.gypi',
      ],
      'include_dirs': [
        '.',
	'<(xwalk_path)/extensions/public',
      ],
      'conditions': [
        ['type != "mobile"', {
	  'sources/': [['exclude', '_mobile\\.cc$']]
	}],
	['type != "desktop"', {
	  'sources/': [['exclude', '_desktop\\.cc$']]
	}],
	['type == "mobile"', {
	  'defines': ['TIZEN_MOBILE']
	}],
	['type == "desktop"', {
	  'defines': ['GENERIC_DESKTOP'],
	  'variables': { 'packages': 'libnotify' },
	  'cflags': [
	    '-fPIC',
	    '<!@(pkg-config --cflags <(packages))'
	  ],
          'link_settings': {
            'ldflags': [
	      '<!@(pkg-config --libs-only-L --libs-only-other <(packages))',
            ],
            'libraries': [
              '<!@(pkg-config --libs-only-l <(packages))',
            ],
	  },
	}],
      ],
    },
  ],
}
