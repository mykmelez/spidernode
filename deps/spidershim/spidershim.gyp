{
  'variables': {
    'spidermonkey_dir%': 'spidermonkey',
  },

  'targets': [

    # TODO: The dependencies for the tests are backwards, we need to fix this.
    # This is currently required to get the tests to build without building the
    # whole tree.
    {
      'target_name': 'test-spidershim-hello-world',
      'type': 'executable',
      'include_dirs': [ '.' ],
      'dependencies': [
        #'spidershim', # XXX correct
        '../gtest/gtest.gyp:gtest',
      ],
      'sources': [ 'test/hello-world.cpp' ],
    },

    {
      'target_name': 'spidershim',
      'type': '<(library)',
      'dependencies': [
        'test-spidershim-hello-world', # XXX incorrect
      ],

      'include_dirs': [
        'include',
        '<(spidermonkey_dir)/../build/dist/include',
      ],
      'conditions': [
        [ 'target_arch=="ia32"', { 'defines': [ '__i386__=1' ] } ],
        [ 'target_arch=="x64"', { 'defines': [ '__x86_64__=1' ] } ],
        [ 'target_arch=="arm"', { 'defines': [ '__arm__=1' ] } ],
        ['node_engine=="spidermonkey"', {
          'dependencies': [
            'spidermonkey.gyp:spidermonkey',
          ],
          'export_dependent_settings': [
            'spidermonkey.gyp:spidermonkey',
          ],
        }],
      ],

      'direct_dependent_settings': {
        'include_dirs': [
          'include',
        ],
        'libraries': [
          '-lspidershim',
          '<@(node_engine_libs)',
        ],
        'conditions': [
          [ 'target_arch=="arm"', {
            'defines': [ '__arm__=1' ]
          }],
        ],
      },

      'sources': [
        'src/v8isolate.cc',
        'src/v8v8.cc',
      ],
    },
  ],
}
