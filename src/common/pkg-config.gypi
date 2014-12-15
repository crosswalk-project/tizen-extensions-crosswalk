{
  'variables': {
    'packages%': [],
  },

  'cflags': [
    '>!@(if [ -n ">@(packages)" ]; then pkg-config --cflags >@(packages); fi)'
  ],

  'link_settings': {
    'ldflags': [
      '>!@(if [ -n ">@(packages)" ]; then pkg-config --libs-only-L --libs-only-other >@(packages); fi)',
    ],
    'libraries': [
      '>!@(if [ -n ">@(packages)" ]; then pkg-config --libs-only-l >@(packages); fi)',
    ],
  },
}
