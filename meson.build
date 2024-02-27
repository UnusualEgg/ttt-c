project('ttt', 'c',
  version : '0.1',
  default_options : ['warning_level=3'])

#cmake=import('cmake')
#TODO maybe linenoise(antirex)
#isocline = cmake.subproject('isocline')

linenoise=library('linenosie','linenoise/linenoise.c',install:false)


#deps =[isocline.dependency('isocline')]
deps=[]
exe = executable('ttt', 'ttt.c', 'input.c', 'terminal.c', 'socket.c',
  install : true,
  dependencies:deps,
  link_with:linenoise,
  include_directories: [include_directories('linenoise')])

test('basic', exe)
