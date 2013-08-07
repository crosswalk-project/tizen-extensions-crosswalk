{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_networkbearerselection',
      'type': 'loadable_module',
      'sources': [
        'networkbearerselection_api.js',
        'networkbearerselection_connection.cc',
        'networkbearerselection_connection.h',
        'networkbearerselection_context.cc',
        'networkbearerselection_context.h',
        'networkbearerselection_context_desktop.cc',
        'networkbearerselection_context_desktop.h',
        'networkbearerselection_context_mobile.cc',
        'networkbearerselection_context_mobile.h',
        'networkbearerselection_request.cc',
        'networkbearerselection_request.h',
      ],
      'conditions': [
        [ 'extension_host_os=="mobile"', {
          'includes': [
            '../common/pkg-config.gypi',
          ],
          'variables': {
            'packages': [
              'capi-network-connection',
            ],
          },
        }],
      ],
    },
  ],
}
