{
  'includes': [
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_device_capabilities',
      'type': 'loadable_module',
      'conditions': [
        [ 'extension_host_os == "mobile"', {
          'variables': {
            'packages': [
              'capi-system-device',
              'capi-system-info',
              'vconf',
            ]
          },
        }],
      ],
      'variables': {
        'packages': [
          'dbus-glib-1',
          'glib-2.0',
          'libudev',
        ]
      },
      'ldflags': [
        '-lX11',
      ],
      'includes': [
        '../common/pkg-config.gypi',
      ],
      'sources': [
        'device_capabilities_api.js',
        'device_capabilities_cpu_desktop.cc',
        'device_capabilities_cpu_mobile.cc',
        'device_capabilities_cpu.h',
        'device_capabilities_extension.cc',
        'device_capabilities_extension.h',
        'device_capabilities_instance.cc',
        'device_capabilities_instance.h',
        'device_capabilities_memory_desktop.cc',
        'device_capabilities_memory_mobile.cc',
        'device_capabilities_memory.h',        
        'device_capabilities_storage_desktop.cc',
        'device_capabilities_storage_mobile.cc',
        'device_capabilities_storage.h',        
        'device_capabilities_utils.h',
        '../common/extension.cc',
        '../common/extension.h',
      ],
    },
  ],
}
