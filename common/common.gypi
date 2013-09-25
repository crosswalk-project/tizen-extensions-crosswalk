{
  'variables': {
    # If capi-system-power package exists, the host is considered to be Tizen Mobile.
    # Note, the spec file requires this package: BuildRequires: pkgconfig(capi-system-power).
    'extension_host_os%': '<!(pkg-config --exists capi-system-power; if [ $? = 0 ]; then echo mobile; else echo desktop; fi)',
    'extension_build_type%': '<(extension_build_type)',
    'extension_build_type%': 'Debug',
  },
  'target_defaults': {
    'conditions': [
      ['extension_host_os != "mobile"', {
        'sources/': [['exclude', '_mobile\\.cc$|mobile/']],
        'includes/': [['exclude', '_mobile\\.gypi$|mobile/']],
      }],
      ['extension_host_os != "desktop"', {
        'sources/': [['exclude', '_desktop\\.cc$|desktop/']],
        'includes/': [['exclude', '_desktop\\.gypi$|desktop/']],
      }],
      ['extension_host_os == "mobile"', { 'defines': ['TIZEN_MOBILE'] } ],
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
    ],
    'includes': [
      'xwalk_js2c.gypi',
    ],
    'include_dirs': [
      '../',
      '<(SHARED_INTERMEDIATE_DIR)',
    ],
    'sources': [
      'extension_adapter.cc',
      'extension_adapter.h',
      'XW_Extension.h',
      'XW_Extension_SyncMessage.h',
      'picojson.h',
      'utils.h',
    ],
    'cflags': [
      '-std=c++0x',
      '-fPIC',
      '-fvisibility=hidden',
    ],
  },
}
