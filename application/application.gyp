{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_application',
      'type': 'loadable_module',
      'sources': [
        '../common/extension.cc',
        '../common/extension.h',
        'application_api.js',
        'application_extension.cc',
        'application_extension.h',
        'application_information.cc',
        'application_information.h',
        'application_instance.cc',
        'application_instance.h',
      ],
      'conditions': [
        ['extension_host_os == "mobile"', {
          'includes': [
            '../common/pkg-config.gypi',
          ],
          'variables': {
            'packages': [
              'capi-appfw-package-manager',
              'pkgmgr',
              'pkgmgr-info',
            ]
          },
        }],
      ],
    },
  ],
}
