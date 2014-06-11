{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_callhistory',
      'type': 'loadable_module',

      'conditions': [
        [ 'extension_host_os == "mobile"', {
          'variables': {
            'packages': ['contacts-service2', 'libpcrecpp',]
          },

         'includes': [
           '../common/pkg-config.gypi',
          ],

          'sources': [
            'callhistory_api.js',
            'callhistory.cc',
            'callhistory.h',
            'callhistory_mobile.cc',
            'callhistory_props.h',
          ],
        }],
      ],
    },
  ],
}
