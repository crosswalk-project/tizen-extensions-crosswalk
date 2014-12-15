{
  'includes':[
    '../common/common.gypi',
  ],
  'variables': {
    'gdbus_codegen_path': '<(SHARED_INTERMEDIATE_DIR)/speech',
  },
  'targets': [
    {
      'target_name': 'tizen_speech',
      'type': 'loadable_module',
      'variables': {
        'packages': [
          'glib-2.0',
          'gio-2.0',
          'gio-unix-2.0',
        ],
      },
      'includes': [
        '../common/pkg-config.gypi',
      ],
      'dependencies': [
        'tizen_speech_gen',
      ],
      'sources': [
        'speech_api.js',
        'speech_extension.cc',
        'speech_extension.h',
        'speech_instance.cc',
        'speech_instance.h',
      ],
    },
    {
      'target_name': 'tizen_speech_gen',
      'type': 'static_library',
      'variables': {
        'packages': [
          'gio-2.0',
          'gio-unix-2.0',
        ],
      },
      'include_dirs': [
        './',
      ],
      'actions': [
        {
          'variables': {
            'generate_args': [
              '--interface-prefix',
              'org.tizen',
              '--c-namespace',
              'Tizen',
              '--generate-c-code',
              '<(gdbus_codegen_path)/tizen_srs_gen',
            ],
          },
          'action_name': 'srs_gen',
          'inputs': [
            'org.tizen.srs.xml',
          ],
          'outputs': [
            '<(gdbus_codegen_path)/tizen_srs_gen.c',
            '<(gdbus_codegen_path)/tizen_srs_gen.h',
          ],
          'action': [
            'gdbus-codegen',
            '<@(generate_args)',
            '<@(_inputs)',
          ],
        },
      ],
      'cflags!': [ '-std=c++0x' ],
      'sources': [
        '<(gdbus_codegen_path)/tizen_srs_gen.c',
        '<(gdbus_codegen_path)/tizne_srs_gen.h',
      ],
      'includes': [
        '../common/pkg-config.gypi',
      ],
    },
  ],
}
