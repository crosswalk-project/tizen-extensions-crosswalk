{
  'includes':[
    'common/common.gypi',
  ],

  'targets': [
    {
      'target_name': 'build_all_tizen_extensions',
      'type': 'none',
      'dependencies': [
        'bluetooth/bluetooth.gyp:*',
        'bookmark/bookmark.gyp:*',
        'filesystem/filesystem.gyp:*',
        'network_bearer_selection/network_bearer_selection.gyp:*',
        'notification/notification.gyp:*',
        'power/power.gyp:*',
        'system_info/system_info.gyp:*',
        'system_setting/system_setting.gyp:*',
        'time/time.gyp:*',
        'tizen/tizen.gyp:*',
      ],
      'conditions': [
        [ 'extension_host_os == "mobile"', {
          'dependencies': [
            'download/download.gyp:*',
          ],
        }],
      ],
    },
    {
      'target_name': 'generate_manifest',
      'type': 'none',

      'conditions': [
        [ 'extension_host_os == "mobile"', {
          'actions': [
            {
              'variables': {
                'generate_args': [
                  '_examples_package',
                  'crosswalk-examples',
                  '/usr/bin/tizen-extensions-crosswalk-examples',
                  'Crosswalk Examples',
                ],
              },
              'action_name': 'examples',
              'inputs': [
                'tools/generate_manifest.py',
                'packaging/tizen-extensions-crosswalk.spec',
                'tizen-extensions-crosswalk.xml.in',
              ],
              'outputs': [
                'tizen-extensions-crosswalk-examples.xml',
              ],
              'action': [
                'python',
                '<@(_inputs)',
                '<@(generate_args)',
                '<@(_outputs)',
              ],
            },
            {
              'variables': {
                'generate_args': [
                  '_bluetooth_demo_package',
                  'crosswalk-bluetooth-demo',
                  '/usr/bin/tizen-extensions-crosswalk-bluetooth-demo',
                  'Crosswalk Bluetooth Demo',
                ],
              },
              'action_name': 'demo',
              'inputs': [
                'tools/generate_manifest.py',
                'packaging/tizen-extensions-crosswalk.spec',
                'tizen-extensions-crosswalk.xml.in',
              ],
              'outputs': [
                'tizen-extensions-crosswalk-bluetooth-demo.xml',
              ],
              'action': [
                'python',
                '<@(_inputs)',
                '<@(generate_args)',
                '<@(_outputs)',
              ],
            },
            {
              'variables': {
                'generate_args': [
                  '_system_info_demo_package',
                  'crosswalk-system-info-demo',
                  '/usr/bin/tizen-extensions-crosswalk-system-info-demo',
                  'Crosswalk System Info Demo',
                ],
              },
              'action_name': 'system_info_demo',
              'inputs': [
                'tools/generate_manifest.py',
                'packaging/tizen-extensions-crosswalk.spec',
                'tizen-extensions-crosswalk.xml.in',
              ],
              'outputs': [
                'tizen-extensions-crosswalk-system-info-demo.xml',
              ],
              'action': [
                'python',
                '<@(_inputs)',
                '<@(generate_args)',
                '<@(_outputs)',
              ],
            },
          ],
        }],
      ],
    },
  ],
}
