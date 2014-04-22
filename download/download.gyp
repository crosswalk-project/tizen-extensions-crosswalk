{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_download',
      'type': 'loadable_module',
      'sources': [
        'download_api.js',
        'download_context.cc',
        'download_context.h',
        'download_context_desktop.cc',
        'download_context_tizen.cc',
        'download_utils.h',
      ],
      'conditions': [
        ['tizen == 1', {
          'includes': [
            '../common/pkg-config.gypi',
          ],
          'variables': {
            'packages': [
              'capi-appfw-application',
              'capi-web-url-download',
            ]
          },
        }],
      ],
    },
  ],
}
