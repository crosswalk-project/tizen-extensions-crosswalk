{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_package',
      'type': 'loadable_module',
      'sources': [
        'package_api.js',
        'package_extension.cc',
        'package_extension.h',
        'package_extension_utils.h',
        'package_info.cc',
        'package_info.h',
        'package_instance.cc',
        'package_instance.h',
        'package_manager.cc',
        'package_manager.h',
      ],
      'conditions': [
        ['tizen == 1', {
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
    }
  ]
}
