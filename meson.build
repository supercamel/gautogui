project(
  'gautogui',
  'c',
  version: '0.1.0',
  meson_version: '>=0.59.0',
  default_options: [
    'c_std=c11',
    'warning_level=2',
  ],
)

cc = meson.get_compiler('c')
pkg = import('pkgconfig')
vala_available = add_languages('vala', required: false, native: false)

gautogui_inc = include_directories('include')

glib_dep = dependency('glib-2.0', version: '>=2.66')
gobject_dep = dependency('gobject-2.0', version: '>=2.66')
gio_dep = dependency('gio-2.0', version: '>=2.66')

subdir('src')
subdir('examples')
subdir('tests')
