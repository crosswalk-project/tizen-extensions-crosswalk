{
  'includes':[
    '../common/common.gypi',
  ],
  'variables': {
    'gen_dbus_proxy_path': '<(SHARED_INTERMEDIATE_DIR)/mediaserver',
  },
  'targets': [
    {
      'target_name': 'tizen_mediaserver',
      'type': 'loadable_module',
      'variables': {
        'packages': [
          'gio-2.0',
          'gio-unix-2.0',
        ],
      },
      'dependencies': [
        'tizen_mediaserver_gen',
      ],
      'sources': [
        'mediaserver_api.js',
        'mediaserver.cc',
        'mediaserver.h',
        'mediaserver_extension.cc',
        'mediaserver_extension.h',
        'mediaserver_instance.cc',
        'mediaserver_instance.h',
        'mediaserver_manager.cc',
        'mediaserver_manager.h',
      ],
      'includes': [
        '../common/pkg-config.gypi',
      ],
    },
    {
      'target_name': 'tizen_mediaserver_gen',
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
              'com.intel.dLeynaServer.',
              '--c-namespace',
              'dleyna',
              '--generate-c-code',
              '<(gen_dbus_proxy_path)/dleyna_manager_gen',
            ],
          },
          'action_name': 'dleyna_manager_gen',
          'inputs': [
            'dbus_interfaces/com.intel.dLeynaServer.Manager.xml',
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
              'com.intel.dLeynaServer.',
              '--c-namespace',
              'dleyna',
              '--generate-c-code',
              '<(gen_dbus_proxy_path)/dleyna_media_device_gen',
            ],
          },
          'action_name': 'dleyna_media_device_gen',
          'inputs': [
            'dbus_interfaces/com.intel.dLeynaServer.MediaDevice.xml',
          ],
          'outputs': [
            '<(gen_dbus_proxy_path)/dleyna_media_device_gen.c',
            '<(gen_dbus_proxy_path)/dleyna_media_device_gen.h',
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
              'org.gnome.UPnP.',
              '--c-namespace',
              'upnp',
              '--generate-c-code',
              '<(gen_dbus_proxy_path)/upnp_media_container_gen',
            ],
          },
          'action_name': 'upnp_media_container_gen',
          'inputs': [
            'dbus_interfaces/org.gnome.UPnP.MediaContainer2.xml',
          ],
          'outputs': [
            '<(gen_dbus_proxy_path)/upnp_media_container_gen.c',
            '<(gen_dbus_proxy_path)/upnp_media_container_gen.h',
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
              'org.gnome.UPnP.',
              '--c-namespace',
              'upnp',
              '--generate-c-code',
              '<(gen_dbus_proxy_path)/upnp_media_object_gen',
            ],
          },
          'action_name': 'upnp_media_object_gen',
          'inputs': [
            'dbus_interfaces/org.gnome.UPnP.MediaObject2.xml',
          ],
          'outputs': [
            '<(gen_dbus_proxy_path)/upnp_media_object_gen.c',
            '<(gen_dbus_proxy_path)/upnp_media_object_gen.h',
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
        '<(gen_dbus_proxy_path)/dleyna_media_device_gen.c',
        '<(gen_dbus_proxy_path)/dleyna_media_device_gen.h',
        '<(gen_dbus_proxy_path)/upnp_media_container_gen.c',
        '<(gen_dbus_proxy_path)/upnp_media_container_gen.h',
        '<(gen_dbus_proxy_path)/upnp_media_object_gen.c',
        '<(gen_dbus_proxy_path)/upnp_media_object_gen.h',
      ],
      'includes': [
        '../common/pkg-config.gypi',
      ],
    },
  ],
}
