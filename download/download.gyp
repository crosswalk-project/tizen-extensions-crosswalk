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
        'download_extension.cc',
        'download_extension.h',
        'download_instance.cc',
        'download_instance.h',
        'download_instance_desktop.cc',
        'download_instance_tizen.cc',
        'download_utils.h',
        '../common/virtual_fs.cc',
        '../common/virtual_fs.h',
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
