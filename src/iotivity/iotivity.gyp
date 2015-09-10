{
  'includes' :[ '../common/common.gypi', ],
              'targets' :[
                {
                  'target_name' : 'tizen_iotivity',
                  'type' : 'loadable_module',
                  'variables' : {
                    'packages' : [],
                  },
                  'sources' : [
                    'iotivity_api.js',
                    'iotivity_extension.cc',
                    'iotivity_extension.h',
                    'iotivity_instance.cc',
                    'iotivity_instance.h',
                    'iotivity_tools.cc',
                    'iotivity_tools.h',
                    'iotivity_device.cc',
                    'iotivity_device.h',
                    'iotivity_server.cc',
                    'iotivity_server.h',
                    'iotivity_client.cc',
                    'iotivity_client.h',
                    'iotivity_resource.cc',
                    'iotivity_resource.h',
                  ],
                  'includes' : [ '../common/pkg-config.gypi', ],
                  'link_settings' : {
                    'libraries' : [ '-loc', '-loc_logger', '-loctbstack', ],
                  },
                },
              ],
}
