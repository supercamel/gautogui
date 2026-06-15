# gautogui design

This document records the implementation plan and the current scope of the
library.

## Goals

gautogui is a small GObject library for GUI automation. It has two jobs:

- Monitor system-wide mouse and keyboard input and emit GObject signals.
- Inject system-level mouse and keyboard input for GUI tests.

The public API is intentionally centered on one object, `GAutoguiController`,
so applications do not need to know which native backend is in use.

## Public API

Create a controller with:

```c
GAutoguiController *controller = gautogui_controller_new();
```

or bind signal delivery to a specific main context:

```c
GAutoguiController *controller =
  gautogui_controller_new_for_context(context);
```

Signals are emitted in the controller's `GMainContext`:

- `mouse-moved (gint x, gint y)`
- `mouse-button-event (guint button, gboolean pressed, gint x, gint y)`
- `mouse-clicked (guint button, gint x, gint y)`
- `mouse-scrolled (gint dx, gint dy, gint x, gint y)`
- `key-event (guint native_keycode, guint key, gboolean pressed)`

`native_keycode` is deliberately exposed for tools that need platform-specific
diagnostics. `key` is a `GAutoguiKey`, or `GAUTOGUI_KEY_NONE` when the backend
cannot map the native key to the portable key enum.

Automation calls:

- `gautogui_controller_get_mouse_position()`
- `gautogui_controller_move_mouse()`
- `gautogui_controller_mouse_down()`
- `gautogui_controller_mouse_up()`
- `gautogui_controller_click()`
- `gautogui_controller_scroll()`
- `gautogui_controller_key_down()`
- `gautogui_controller_key_up()`
- `gautogui_controller_press_key()`
- `gautogui_controller_type_text()`
- `gautogui_controller_type_text_with_delay()`

The pointer/query and injection calls also expose GIO-style asynchronous
variants named `*_async()` and `*_finish()`. These wrappers run the synchronous
backend operation in a `GTask` worker thread, return results through the
controller's caller context, and are intended for bindings such as Vala, Python,
and SqGI that can map GLib async patterns onto `await`-style code. Cancellation
is checked before the worker starts; once native input injection has begun, the
operation runs to completion.

The portable key enum covers letters, digits, common punctuation, navigation
keys, modifiers, and F1-F24. `type_text()` is the preferred API for entering
text because it can handle layout-specific modifier needs. `type_text()` uses a
small default pause between characters so target applications do not drop
bursty synthetic input; `type_text_with_delay()` lets callers tune that pause or
set it to zero.

## Backend contract

The private backend interface lives in `src/gautogui-backend.h`. Each platform
backend implements the same functions:

- lifecycle: `new`, `free`, `start`, `stop`
- state query: `get_mouse_position`
- pointer injection: `move_mouse`, `mouse_button`, `scroll`
- keyboard injection: `key`, `type_text`

Native hook callbacks never emit signals directly. They call private
`_gautogui_controller_emit_*()` helpers, which marshal the signal emission back
to the controller's `GMainContext`.

## Windows implementation

The Windows backend uses:

- `SetWindowsHookExW(WH_MOUSE_LL)` for global mouse monitoring.
- `SetWindowsHookExW(WH_KEYBOARD_LL)` for global keyboard monitoring.
- A dedicated GLib thread with a Win32 message loop, because low-level hooks
  are delivered on the installing thread.
- `GetCursorPos()` and `SetCursorPos()` for pointer position.
- `SendInput()` for mouse buttons, wheel input, virtual-key events, and Unicode
  text input.

Only one active hook instance is allowed per process. That matches the static
callback shape of low-level Win32 hooks and keeps event routing deterministic.

## Linux implementation

The Linux backend currently targets X11/Xorg:

- XInput2 raw events on the root window monitor global pointer and keyboard
  events even when another X11 application has focus.
- XTest injects pointer movement, buttons, wheel events, and key events.
- `XQueryPointer()` reads the current pointer position.
- A GLib thread polls the X connection fd plus an internal wake pipe for clean
  shutdown.

This backend requires `x11`, `xi`, and `xtst`.

### Wayland note

Generic Wayland clients are not allowed to observe or inject system-wide input.
That restriction is intentional in the Wayland security model. XWayland may set
`DISPLAY`, but it still cannot promise coverage of native Wayland applications.
For that reason, the Linux backend reports `GAUTOGUI_ERROR_BACKEND_UNAVAILABLE`
when the session advertises itself as Wayland. Set `GAUTOGUI_FORCE_X11=1` only
when the caller explicitly accepts XWayland-limited behavior for testing.

Future Linux work can add a privileged evdev/uinput backend for test rigs that
explicitly grant access to `/dev/input` and `/dev/uinput`, but that is a
different operational model from an ordinary desktop client library.

## Build

```sh
meson setup build
meson compile -C build
meson test -C build
```

Run the monitor example:

```sh
./build/examples/gautogui-monitor
```
