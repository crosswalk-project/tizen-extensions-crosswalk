{
  'targets': [
    {
      'target_name': 'build_all_tizen_extensions',
      'type': 'none',
      'dependencies': [
        'bluetooth/bluetooth.gyp:*',
        'networkbearerselection/networkbearerselection.gyp:*',
        'notification/notification.gyp:*',
        'power/power.gyp:*',
        'system_info/system_info.gyp:*',
        'time/time.gyp:*',
        'tizen/tizen.gyp:*',
      ],
    },
  ],

}
