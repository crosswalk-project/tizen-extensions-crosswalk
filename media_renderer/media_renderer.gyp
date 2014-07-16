{
  'includes':[
    '../common/common.gypi',
  ],
  'variables': {
    'gen_dbus_proxy_path': '<(SHARED_INTERMEDIATE_DIR)/media_renderer',
  },
  'targets': [
    {
      'target_name': 'tizen_media_renderer',
      'type': 'loadable_module',
      'variables': {
        'packages': [
          'gio-2.0',
          'gio-unix-2.0',
        ],
      },
      'dependencies': [
        'tizen_media_renderer_gen',
      ],
      'sources': [
        'media_renderer_api.js',
        'media_renderer.cc',
        'media_renderer.h',
        'media_renderer_extension.cc',
        'media_renderer_extension.h',
        'media_renderer_instance.cc',
        'media_renderer_instance.h',
        'media_renderer_manager.cc',
        'media_renderer_manager.h',
      ],
      'includes': [
        '../common/pkg-config.gypi',
      ],
    },
    {
      'target_name': 'tizen_media_renderer_gen',
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
              'com.intel.dLeynaRenderer.',
              '--c-namespace',
              'dleyna',
              '--generate-c-code',
              '<(gen_dbus_proxy_path)/dleyna_manager_gen',
            ],
          },
          'action_name': 'dleyna_manager_gen',
          'inputs': [
            'dbus_interfaces/com.intel.dLeynaRenderer.Manager.xml',
          ],
          'outputs': [
            '<(gen_dbus_proxy_path)/dleyna_manager_gen.c',
            '<(gen_dbus_proxy_path)/dleyna_manager_gen.h',
          ],
          'action': [
            'gdbus-codegen',
            '<@(generate_args)',
            '<@(_inputs)',
          ],
        },
        {
          'variables': {
            'generate_args': [
              '--interface-prefix',
              'com.intel.dLeynaRenderer.',
              '--c-namespace',
              'dleyna',
              '--generate-c-code',
              '<(gen_dbus_proxy_path)/dleyna_renderer_device_gen',
            ],
          },
          'action_name': 'dleyna_media_device_gen',
          'inputs': [
            'dbus_interfaces/com.intel.dLeynaRenderer.RendererDevice.xml',
          ],
          'outputs': [
            '<(gen_dbus_proxy_path)/dleyna_renderer_device_gen.c',
            '<(gen_dbus_proxy_path)/dleyna_renderer_device_gen.h',
          ],
          'action': [
            'gdbus-codegen',
            '<@(generate_args)',
            '<@(_inputs)',
          ],
        },
        {
          'variables': {
            'generate_args': [
              '--interface-prefix',
              'com.intel.dLeynaRenderer',
              '--c-namespace',
              'dleyna',
              '--generate-c-code',
              '<(gen_dbus_proxy_path)/dleyna_pushhost_gen',
            ],
          },
          'action_name': 'dleyna_pushhost_gen',
          'inputs': [
            'dbus_interfaces/com.intel.dLeynaRenderer.PushHost.xml',
          ],
          'outputs': [
            '<(gen_dbus_proxy_path)/dleyna_pushhost_gen.c',
            '<(gen_dbus_proxy_path)/dleyna_pushhost_gen.h',
          ],
          'action': [
            'gdbus-codegen',
            '<@(generate_args)',
            '<@(_inputs)',
          ],
        },
        {
          'variables': {
            'generate_args': [
              '--interface-prefix',
              'org.mpris.',
              '--c-namespace',
              'mpris',
              '--generate-c-code',
              '<(gen_dbus_proxy_path)/mpris_mediaplayer2_gen',
            ],
          },
          'action_name': 'mpris_mediaplayer2_gen',
          'inputs': [
            'dbus_interfaces/org.mpris.MediaPlayer2.xml',
          ],
          'outputs': [
            '<(gen_dbus_proxy_path)/mpris_mediaplayer2_gen.c',
            '<(gen_dbus_proxy_path)/mpris_mediaplayer2_gen.h',
          ],
          'action': [
            'gdbus-codegen',
            '<@(generate_args)',
            '<@(_inputs)',
          ],
        },
        {
          'variables': {
            'generate_args': [
              '--interface-prefix',
              'org.mpris.MediaPlayer2.',
              '--c-namespace',
              'mprismediaplayer2',
              '--generate-c-code',
              '<(gen_dbus_proxy_path)/mpris_mediaplayer2_player_gen',
            ],
          },
          'action_name': 'mpris_mediaplayer2_player_gen',
          'inputs': [
            'dbus_interfaces/org.mpris.MediaPlayer2.Player.xml',
          ],
          'outputs': [
            '<(gen_dbus_proxy_path)/mpris_mediaplayer2_player_gen.c',
            '<(gen_dbus_proxy_path)/mpris_mediaplayer2_player_gen.h',
          ],
          'action': [
            'gdbus-codegen',
            '<@(generate_args)',
            '<@(_inputs)',
          ],
        },
      ],
      # Compile generated dbus proxies without C++11 flag
      'cflags!': [ '-std=c++0x' ],
      'sources': [
        '<(gen_dbus_proxy_path)/dleyna_manager_gen.c',
        '<(gen_dbus_proxy_path)/dleyna_manager_gen.h',
        '<(gen_dbus_proxy_path)/dleyna_renderer_device_gen.c',
        '<(gen_dbus_proxy_path)/dleyna_renderer_device_gen.h',
        '<(gen_dbus_proxy_path)/dleyna_pushhost_gen.c',
        '<(gen_dbus_proxy_path)/dleyna_pushhost_gen.h',
        '<(gen_dbus_proxy_path)/mpris_mediaplayer2_player_gen.c',
        '<(gen_dbus_proxy_path)/mpris_mediaplayer2_player_gen.h',
        '<(gen_dbus_proxy_path)/mpris_mediaplayer2_gen.c',
        '<(gen_dbus_proxy_path)/mpris_mediaplayer2_gen.h',
      ],
      'includes': [
        '../common/pkg-config.gypi',
      ],
    },
  ],
}
