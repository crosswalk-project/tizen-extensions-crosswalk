{
  'targets': [
    {
      'target_name': 'tizen_power',
      'type': 'loadable_module',
      'sources': [
        'power_api.js',
        'power_context.cc',
        'power_context.h',
        'power_context_desktop.cc',
      ],
    },
  ],
}
