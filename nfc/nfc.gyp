{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_nfc',
      'type': 'loadable_module',
      'variables': {
        'packages': [
          'capi-network-nfc',
        ],
      },
      'sources': [
        'nfc_api.js',
        'nfc_extension.cc',
        'nfc_extension.h',
        'nfc_instance.cc',
        'nfc_instance.h',
      ],
      'includes': [
        '../common/pkg-config.gypi',
      ],
    },
  ],
}
