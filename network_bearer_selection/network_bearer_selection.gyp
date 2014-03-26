{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_network_bearer_selection',
      'type': 'loadable_module',
      'sources': [
        'network_bearer_selection_api.js',
        'network_bearer_selection_connection_tizen.cc',
        'network_bearer_selection_connection_tizen.h',
        'network_bearer_selection_context.cc',
        'network_bearer_selection_context.h',
        'network_bearer_selection_context_desktop.cc',
        'network_bearer_selection_context_desktop.h',
        'network_bearer_selection_context_tizen.cc',
        'network_bearer_selection_context_tizen.h',
        'network_bearer_selection_request.cc',
        'network_bearer_selection_request.h',
      ],
      'conditions': [
        [ 'tizen == 1', {
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
