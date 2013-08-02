{
  'targets': [
    {
      'target_name': 'vconf',
      'type': 'none',
      'conditions': [
        ['type=="mobile"', {
          'direct_dependent_settings': {
            'cflags': [
              '<!@(pkg-config --cflags vconf)',
            ],
          },
          'link_settings': {
            'ldflags': [
              '<!@(pkg-config --libs-only-L --libs-only-other vconf)',
            ],
            'libraries': [
              '<!@(pkg-config --libs-only-l vconf)',
            ],
          }
        }],
      ],
    },

    {
      'target_name': 'capi-system-power',
      'type': 'none',
      'conditions': [
        ['type=="mobile"', {
          'direct_dependent_settings': {
            'cflags': [
              '<!@(pkg-config --cflags capi-system-power)',
            ],
          },
          'link_settings': {
            'ldflags': [
              '<!@(pkg-config --libs-only-L --libs-only-other capi-system-power)',
            ],
            'libraries': [
              '<!@(pkg-config --libs-only-l capi-system-power)',
            ],
          }
        }],
      ],
    },

    {
      'target_name': 'capi-system-device',
      'type': 'none',
      'conditions': [
        ['type=="mobile"', {
          'direct_dependent_settings': {
            'cflags': [
              '<!@(pkg-config --cflags capi-system-device)',
            ],
          },
          'link_settings': {
            'ldflags': [
              '<!@(pkg-config --libs-only-L --libs-only-other capi-system-device)',
            ],
            'libraries': [
              '<!@(pkg-config --libs-only-l capi-system-device)',
            ],
          }
        }],
      ],
    },

    {
      'target_name': 'pmapi',
      'type': 'none',
      'conditions': [
        ['type=="mobile"', {
          'direct_dependent_settings': {
            'cflags': [
              '<!@(pkg-config --cflags pmapi)',
            ],
          },
          'link_settings': {
            'ldflags': [
              '<!@(pkg-config --libs-only-L --libs-only-other pmapi)',
            ],
            'libraries': [
              '<!@(pkg-config --libs-only-l pmapi)',
            ],
          }
        }],
      ],
    },
  ],
}
