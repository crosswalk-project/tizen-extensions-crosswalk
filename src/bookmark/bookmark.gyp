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
      ],

      # Evas.h is used in favorites.h.
      'conditions': [
        [ 'tizen == 1', {
            'variables': { 'packages': ['capi-web-favorites', 'evas'] },
        }],
      ],
    },
  ],
}
