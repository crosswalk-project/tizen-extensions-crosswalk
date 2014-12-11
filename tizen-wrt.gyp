{
  'includes':[
    'src/common/common.gypi',
  ],

  'targets': [
    {
      'target_name': 'build_all_tizen_extensions',
      'type': 'none',
      'dependencies': [
        'src/bluetooth/bluetooth.gyp:*',
        'src/media_renderer/media_renderer.gyp:*',
        'src/mediaserver/mediaserver.gyp:*',
        'src/network_bearer_selection/network_bearer_selection.gyp:*',
        'src/notification/notification.gyp:*',
        'src/phone/phone.gyp:*',
        'src/power/power.gyp:*',
        'src/speech/speech.gyp:*',
        'src/system_info/system_info.gyp:*',
        'src/system_setting/system_setting.gyp:*',
        'src/time/time.gyp:*',
        'src/tizen/tizen.gyp:*',
        'src/utils/utils.gyp:*',
        'src/web_setting/web_setting.gyp:*',
      ],
      'conditions': [
        [ 'tizen == 1', {
          'dependencies': [
            'src/alarm/alarm.gyp:*',
            'src/application/application.gyp:*',
            'src/bookmark/bookmark.gyp:*',
            'src/content/content.gyp:*',
            'src/download/download.gyp:*',
            'src/filesystem/filesystem.gyp:*',
            'src/messageport/messageport.gyp:*',
            'src/nfc/nfc.gyp:*',
            'src/package/package.gyp:*',
          ],
        }],
        [ 'extension_host_os == "mobile"', {
          'dependencies': [
            'src/callhistory/callhistory.gyp:*',
            'src/datasync/datasync.gyp:*',
          ],
        }],
        [ 'extension_host_os == "ivi"', {
          'dependencies': [
            'src/audiosystem/audiosystem.gyp:*',
            'src/sso/sso.gyp:*',
            'src/telephony/telephony.gyp:*',
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
