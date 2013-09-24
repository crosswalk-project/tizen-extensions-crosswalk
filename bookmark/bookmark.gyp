{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_bookmark',
      'type': 'loadable_module',
      'includes': [
        '../common/pkg-config.gypi',
      ],
      'sources': [
        'bookmark_api.js',
        'bookmark_extension.cc',
        'bookmark_extension.h',
        'bookmark_instance.cc',
        'bookmark_instance.h',
        '../common/extension.h',
        '../common/extension.cc',
      ],

      # Evas.h is used in favorites.h.
      'conditions': [
        [ 'extension_host_os == "mobile"', {
            'variables': { 'packages': ['capi-web-favorites', 'evas'] },
        }],
      ],
    },
  ],
}
