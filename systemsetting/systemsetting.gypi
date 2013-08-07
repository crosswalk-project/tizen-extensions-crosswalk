{
  'targets': [
    {
      'target_name': 'tizen_systemsetting',
      'type': 'loadable_module',
      'sources': [
        'systemsetting_api.js',
        'systemsetting_context.cc',
        'systemsetting_context.h',
        'systemsetting_context_desktop.cc',
        'systemsetting_context_mobile.cc',
      ],
      'conditions': [
        ['type=="mobile"', {
          'cflags': [
            '<!@(pkg-config --cflags capi-system-system-settings)',
          ],
          'link_settings': {
            'ldflags': [
              '<!@(pkg-config --libs-only-L --libs-only-other capi-system-system-settings)',
            ],
            'libraries': [
              '<!@(pkg-config --libs-only-l capi-system-system-settings)',
            ],
          },
        }],
      ],
    },
  ],
}