{
  'variables': {
    'packages%': [],
  },

  'cflags': [
    '>!@(pkg-config --cflags >@(packages))'
  ],

  'link_settings': {
    'ldflags': [
      '>!@(pkg-config --libs-only-L --libs-only-other >@(packages))',
    ],
    'libraries': [
      '>!@(pkg-config --libs-only-l >@(packages))',
    ],
  },
}
