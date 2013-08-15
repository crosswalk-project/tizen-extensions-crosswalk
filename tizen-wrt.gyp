{
  'targets': [
    {
      'target_name': 'build_all_tizen_extensions',
      'type': 'none',
      'dependencies': [
        'bluetooth/bluetooth.gyp:*',
        'network_bearer_selection/network_bearer_selection.gyp:*',
        'notification/notification.gyp:*',
        'power/power.gyp:*',
        'system_info/system_info.gyp:*',
        'system_setting/system_setting.gyp:*',
        'time/time.gyp:*',
        'tizen/tizen.gyp:*',
        'download/download.gyp:*',
      ],
    },
  ],
}
