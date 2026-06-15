# gautogui

gautogui is a GObject/C library for system-wide GUI input monitoring and
automation.

Documentation: https://gautogui.readthedocs.io/

Companion SQGI GTK4 monitor app:
https://github.com/supercamel/sqgi_gautogui

It provides a `GAutoguiController` object that can:

- emit signals for global mouse movement, clicks, scrolls, and keyboard events;
- read and move the mouse pointer;
- inject mouse clicks, scrolls, key presses, and text.

Text injection is paced by default to avoid dropped keystrokes in real
applications. Use `gautogui_controller_type_text_with_delay()` or the
equivalent binding method to choose the inter-character delay explicitly.
Automation methods also have GIO-style `*_async()`/`*_finish()` variants for
callers that want to keep their main loop responsive while an operation runs.

## Platform support

- Windows: low-level mouse/keyboard hooks plus `SendInput`.
- Linux: X11/Xorg via XInput2 raw events plus XTest injection.

Wayland does not expose a generic client API for system-wide input hooks or
input injection. On Wayland/XWayland sessions, gautogui returns a backend error
instead of pretending it can observe input it cannot legally access. Set
`GAUTOGUI_FORCE_X11=1` only when XWayland-limited behavior is acceptable.

## Build

```sh
meson setup build
meson compile -C build
meson test -C build
```

## Documentation

The hosted documentation is available on Read the Docs:
https://gautogui.readthedocs.io/

Documentation sources live in `docs/`. To build them locally:

```sh
python3 -m venv /tmp/gautogui-docs-venv
. /tmp/gautogui-docs-venv/bin/activate
pip install -r docs/requirements.txt
sphinx-build -W -b html docs docs/_build/html
```

## Language Examples

From the build tree:

```sh
GI_TYPELIB_PATH=build/src LD_LIBRARY_PATH=build/src python3 examples/monitor.py
./build/examples/gautogui-monitor-vala
GI_TYPELIB_PATH=build/src LD_LIBRARY_PATH=build/src sqgi examples/monitor.nut
```

On Wayland/XWayland these examples report that true system-wide monitoring is
unavailable. For XWayland-limited testing, prefix the command with
`GAUTOGUI_FORCE_X11=1`.

There is also a Windows-focused SQGI automation demo that uses SQGI
`async`/`await` over gautogui's async automation API, moves the mouse, opens
the Start menu with the Super/Windows key, launches Notepad, and types a short
line with an explicit inter-character delay:

```sh
sqgi examples/notepad-demo.nut
```

## Minimal example

```c
#include <gautogui/gautogui.h>

static void
on_key_event(GAutoguiController *controller,
             guint native_keycode,
             guint key,
             gboolean pressed,
             gpointer user_data)
{
  g_print("key %u %s\n", key, pressed ? "down" : "up");
}

int
main(void)
{
  g_autoptr(GAutoguiController) controller = gautogui_controller_new();
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);
  g_autoptr(GError) error = NULL;

  g_signal_connect(controller, "key-event", G_CALLBACK(on_key_event), NULL);

  if (!gautogui_controller_start(controller, &error)) {
    g_printerr("%s\n", error->message);
    return 1;
  }

  g_main_loop_run(loop);
  return 0;
}
```
