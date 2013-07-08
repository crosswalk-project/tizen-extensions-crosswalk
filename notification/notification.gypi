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
      ],

      'conditions': [
        [ 'type == "desktop"', {
            'variables': { 'packages': ['libnotify'] },
            'includes': [ '../pkg-config.gypi' ],
          }
        ],
      ],
    },
  ],
}
