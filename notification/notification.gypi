{
  'targets': [
    {
      'target_name': 'tizen_notification',
      'type': 'loadable_module',
      'sources': [
        'notification_api.js',
        'notification_context.cc',
        'notification_context.h',
        'notification_context_desktop.cc',
        'notification_context_mobile.cc',
	'mobile/notification_manager.cc',
	'mobile/notification_manager.h',
      ],

      'conditions': [
        [ 'type == "desktop"', {
            'variables': { 'packages': ['libnotify'] },
        }],
	[ 'type == "mobile"', {
	    'variables': { 'packages': ['notification'] },
	}],
      ],

      'includes': [ '../pkg-config.gypi' ],
    },
  ],
}
