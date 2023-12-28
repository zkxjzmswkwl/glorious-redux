{
      'variables': {
            'driver%': 'hidraw',
      },
      'targets': [
            {
                  'target_name': 'GloriousMOISDK',
                  'include_dirs+': [
                        'src/',
                  ],
                  'sources': [ 
                        'src/GloriousMOISDK.cc', "src/GloriousMOISDK.h"
                  ],
                  'defines':[
                        'WIN32_LEAN_AND_MEAN',
                        'DEBUG'
                  ],
                  'dependencies': [
                        'hidapi'
                  ],
                  'msvs_settings': {
                        'VCLinkerTool': {
                              'AdditionalDependencies': [
                                    'setupapi.lib',
                                    'WtsApi32.lib',
                                    'psapi.lib',
                                    'ole32.lib',
                                    'shell32.lib',
                                    'Shlwapi.lib'
                              ]
                        },
                        'VCCLCompilerTool': {
                              'AdditionalOptions': [ 
                                    '/EHsc' 
                              ],
                        }
                  }
            },
            {
                  'target_name': 'hidapi',
                  'type': 'static_library',
                  'conditions': [
                        [ 'OS=="mac"', {                                   
                                    # 'sources': [ '../thridparty/hidapi/mac/hid.c'],
                                    'include_dirs+': [ '/usr/include/libusb-1.0/']
                              }
                        ],
                        ['OS=="linux"', {                                   
                                    'conditions': [
                                          [ 'driver=="libusb"', {
                                                      # 'sources': [ '../thridparty/hidapi/libusb/hid.c' ],
                                                      'include_dirs+': ['/usr/include/libusb-1.0/']
                                                }
                                          ],
                                          [ 'driver=="hidraw"', {'sources': [ '../thridparty/hidapi/linux/hid.c' ]}
                                          ]
                                    ]
                              }
                        ],
                        [ 'OS=="win"', {                              
                                    'sources': [ 
                                          # '../thridparty/hidapi/windows/hid.c', 
                                          # 'src/tools/WriteLog.cpp' ,   
                                          # 'src/DriverInit.cpp'     
                                          ],                                    
                                    'msvs_settings': {
                                          'VCLinkerTool': {
                                                'AdditionalDependencies': ['setupapi.lib',]
                                          }
                                    } 
                              }
                        ]
                  ],
                  # 'direct_dependent_settings': {
                  #       'include_dirs': ['../thridparty/hidapi/hidapi']
                  # },
                  # 'include_dirs': ['../thridparty/hidapi/hidapi'],
                  'defines': [
                        '_LARGEFILE_SOURCE',
                        '_FILE_OFFSET_BITS=64',
                  ],
                  'cflags': ['-g'],
                  'cflags!': ['-ansi']
            }
      ]
}
