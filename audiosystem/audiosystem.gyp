{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_audiosystem',
      'type': 'loadable_module',
      'variables': {
        'packages': [
          'libpulse',
        ],
      },
      'includes': [
        '../common/pkg-config.gypi',
      ],
      'sources': [
        'audiosystem_api.js',
        'audiosystem_audio_group.cc',
        'audiosystem_audio_group.h',
        'audiosystem_context.cc',
        'audiosystem_context.h',
        'audiosystem_device.cc',
        'audiosystem_device.h',
        'audiosystem_extension.cc',
        'audiosystem_extension.h',
        'audiosystem_instance.cc',
        'audiosystem_instance.h',
        'audiosystem_logs.h',
        'audiosystem_mute_control.cc',
        'audiosystem_mute_control.h',
        'audiosystem_stream.cc',
        'audiosystem_stream.h',
        'audiosystem_volume_control.cc',
        'audiosystem_volume_control.h',
      ],
    },
  ],
}
