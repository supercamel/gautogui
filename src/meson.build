public_headers = files(
  '../include/gautogui/gautogui.h',
  '../include/gautogui/gautogui-controller.h',
)

core_sources = files(
  'gautogui-controller.c',
)

platform_sources = []
platform_deps = []
platform_pkg_deps = []
platform_c_args = []

host_system = host_machine.system()

if host_system == 'windows'
  platform_sources += files('gautogui-backend-win32.c')
  platform_deps += cc.find_library('user32')
  platform_c_args += ['-DWIN32_LEAN_AND_MEAN']
elif host_system == 'linux'
  x11_dep = dependency('x11')
  xi_dep = dependency('xi')
  xtst_dep = dependency('xtst')

  platform_sources += files('gautogui-backend-x11.c')
  platform_deps += [x11_dep, xi_dep, xtst_dep]
  platform_pkg_deps += ['x11', 'xi', 'xtst']
else
  platform_sources += files('gautogui-backend-dummy.c')
endif

all_deps = [
  glib_dep,
  gobject_dep,
] + platform_deps

libgautogui = library(
  'gautogui-1.0',
  core_sources + platform_sources,
  include_directories: gautogui_inc,
  dependencies: all_deps,
  c_args: ['-DGAUTOGUI_COMPILATION'] + platform_c_args,
  version: '0.0.0',
  soversion: '0',
  install: true,
)

install_headers(public_headers, subdir: 'gautogui-1.0/gautogui')

gautogui_dep = declare_dependency(
  link_with: libgautogui,
  include_directories: gautogui_inc,
  dependencies: [glib_dep, gobject_dep],
)

pkg.generate(
  libgautogui,
  filebase: 'gautogui-1.0',
  name: 'gautogui',
  description: 'Cross-platform system-wide GUI input monitoring and automation',
  subdirs: 'gautogui-1.0',
  requires: ['glib-2.0', 'gobject-2.0'],
  requires_private: platform_pkg_deps,
)

gautogui_vapi_dep = []

gir_dep = dependency('gobject-introspection-1.0', required: get_option('introspection'))
if gir_dep.found()
  gnome = import('gnome')
  gautogui_gir = gnome.generate_gir(
    libgautogui,
    sources: public_headers + core_sources,
    namespace: 'Gautogui',
    nsversion: '1.0',
    identifier_prefix: 'GAutogui',
    symbol_prefix: 'gautogui',
    header: 'gautogui/gautogui.h',
    includes: ['GObject-2.0'],
    export_packages: 'gautogui-1.0',
    install: true,
  )

  if vala_available
    gautogui_vapi_dep = gnome.generate_vapi(
      'gautogui-1.0',
      sources: gautogui_gir[0],
      packages: ['gobject-2.0'],
      install: true,
    )
  endif
endif
