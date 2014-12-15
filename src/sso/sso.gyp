{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_sso',
      'type': 'loadable_module',
      'variables': {
        'packages': [
          'libgsignon-glib',
        ],
      },
      'includes': [
        '../common/pkg-config.gypi',
      ],
      'sources': [
        'sso_api.js',
        'sso_async_op.cc',
        'sso_async_op.h',
        'sso_auth_service.cc',
        'sso_auth_service.h',
        'sso_auth_session.cc',
        'sso_auth_session.h',
        'sso_extension.cc',
        'sso_extension.h',
        'sso_identity.cc',
        'sso_identity.h',
        'sso_identity_info.cc',
        'sso_identity_info.h',
        'sso_instance.cc',
        'sso_instance.h',
        'sso_utils.cc',
        'sso_utils.h',
      ],
    },
  ],
}
