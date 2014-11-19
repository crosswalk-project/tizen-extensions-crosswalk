{
  'variables': {
    'extension_host_os%': 'desktop',
    'tizen%': '0',
    'extension_build_type%': '<(extension_build_type)',
    'extension_build_type%': 'Debug',
    'display_type%': 'x11',
  },
  'target_defaults': {
    'conditions': [
      ['extension_host_os != "mobile"', {
        'sources/': [['exclude', '_mobile\\.cc$|mobile/']],
        'includes/': [['exclude', '_mobile\\.gypi$|mobile/']],
      }],
      ['extension_host_os != "ivi"', {
        'sources/': [['exclude', '_ivi\\.cc$|ivi/']],
        'includes/': [['exclude', '_ivi\\.gypi$|ivi/']],
      }],
      ['extension_host_os != "desktop"', {
        'sources/': [['exclude', '_desktop\\.cc$|desktop/']],
        'includes/': [['exclude', '_desktop\\.gypi$|desktop/']],
        'variables': {
          'packages': [
            'libtzplatform-config',
          ],
        },
      }],
      ['tizen == 1', {
        'defines': ['TIZEN']
      }, {
        'sources/': [['exclude', '_tizen\\.cc$|tizen/']],
        'includes/': [['exclude', '_tizen\\.gypi$|tizen/']],
      }],
      ['extension_host_os == "mobile"', { 'defines': ['TIZEN_MOBILE'] } ],
      ['extension_host_os == "ivi"', { 'defines': ['TIZEN_IVI'] } ],
      ['extension_host_os == "desktop"', { 'defines': ['GENERIC_DESKTOP'] } ],
      ['extension_build_type== "Debug"', {
        'defines': ['_DEBUG', ],
        'cflags': [ '-O0', '-g', ],
      }],
      ['extension_build_type == "Release"', {
        'defines': ['NDEBUG', ],
        'cflags': [
          '-O2',
          # Don't emit the GCC version ident directives, they just end up
          # in the .comment section taking up binary size.
          '-fno-ident',
          # Put data and code in their own sections, so that unused symbols
          # can be removed at link time with --gc-sections.
          '-fdata-sections',
          '-ffunction-sections',
        ],
      }],
      [ 'display_type != "wayland"', {
        'sources/': [['exclude', '_wayland\\.cc$|wayland/']],
      }],
      [ 'display_type != "x11"', {
        'sources/': [['exclude', '_x11\\.cc$|x11/']],
      }],
    ],
    'includes': [
      'xwalk_js2c.gypi',
    ],
    'include_dirs': [
      '../',
      '<(SHARED_INTERMEDIATE_DIR)',
    ],
    'sources': [
      'extension.cc',
      'extension.h',
      'picojson.h',
      'scope_exit.h',
      'utils.h',
      'XW_Extension.h',
      'XW_Extension_EntryPoints.h',
      'XW_Extension_Permissions.h',
      'XW_Extension_Runtime.h',
      'XW_Extension_SyncMessage.h',
    ],
    'cflags': [
      '-std=c++0x',
      '-fPIC',
      '-fvisibility=hidden',
    ],
  },
}
