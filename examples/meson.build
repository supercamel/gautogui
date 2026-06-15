monitor_example = executable(
  'gautogui-monitor',
  'monitor.c',
  dependencies: gautogui_dep,
  install: false,
)

if vala_available and gir_dep.found()
  vala_monitor_example = executable(
    'gautogui-monitor-vala',
    'monitor.vala',
    dependencies: [gautogui_dep, gautogui_vapi_dep],
    install: false,
  )
endif
