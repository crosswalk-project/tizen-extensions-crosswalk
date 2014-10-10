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
        'media_renderer/media_renderer.gyp:*',
        'mediaserver/mediaserver.gyp:*',
        'network_bearer_selection/network_bearer_selection.gyp:*',
        'notification/notification.gyp:*',
        'phone/phone.gyp:*',
        'power/power.gyp:*',
        'speech/speech.gyp:*',
        'system_info/system_info.gyp:*',
        'system_setting/system_setting.gyp:*',
        'time/time.gyp:*',
        'tizen/tizen.gyp:*',
        'utils/utils.gyp:*',
        'web_setting/web_setting.gyp:*',
      ],
      'conditions': [
        [ 'tizen == 1', {
          'dependencies': [
            'alarm/alarm.gyp:*',
            'application/application.gyp:*',
            'bookmark/bookmark.gyp:*',
            'content/content.gyp:*',
            'datasync/datasync.gyp:*',
            'download/download.gyp:*',
            'filesystem/filesystem.gyp:*',
            'messageport/messageport.gyp:*',
            'nfc/nfc.gyp:*',
            'package/package.gyp:*',
          ],
        }],
        [ 'extension_host_os == "mobile"', {
          'dependencies': [
            'callhistory/callhistory.gyp:*',
          ],
        }],
        [ 'extension_host_os == "ivi"', {
          'dependencies': [
            'audiosystem/audiosystem.gyp:*',
            'sso/sso.gyp:*',
            'telephony/telephony.gyp:*',
            'vehicle/vehicle.gyp:*',
          ],
        }],
      ],
    },
    {
      'target_name': 'generate_manifest',
      'type': 'none',

      'conditions': [
        [ 'tizen == 1', {
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
            {
              'variables': {
                'generate_args': [
                  '_audiosystem_demo_package',
                  'crosswalk-audiosystem-demo',
                  '/usr/bin/tizen-extensions-crosswalk-audiosystem-demo',
                  'Crosswalk Tizen Volume API Demo',
                ],
              },
              'action_name': 'audiosystem_demo',
              'inputs': [
                'tools/generate_manifest.py',
                'packaging/tizen-extensions-crosswalk.spec',
                'tizen-extensions-crosswalk.xml.in',
              ],
              'outputs': [
                'tizen-extensions-crosswalk-audiosystem-demo.xml',
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
