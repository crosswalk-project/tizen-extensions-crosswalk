{
  'targets': [
    {
      'target_name': 'tizen_networkbearerselection',
      'type': 'loadable_module',
      'sources': [
        'networkbearerselection_api.js',
        'networkbearerselection_context.cc',
        'networkbearerselection_context.h',
        'networkbearerselection_context_desktop.cc',
        'networkbearerselection_context_mobile.cc',
      ],
      'conditions': [
        ['type=="mobile"', {
          'cflags': [
            '<!@(pkg-config --cflags capi-network-connection)',
          ],
          'link_settings': {
            'ldflags': [
              '<!@(pkg-config --libs-only-L --libs-only-other capi-network-connection)',
            ],
            'libraries': [
              '<!@(pkg-config --libs-only-l capi-network-connection)',
            ],
          },
        }],
      ],
    },
  ],
}
