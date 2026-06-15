Platform Notes
==============

Windows
-------

The Windows backend uses:

* ``SetWindowsHookExW(WH_MOUSE_LL)`` for global mouse monitoring.
* ``SetWindowsHookExW(WH_KEYBOARD_LL)`` for global keyboard monitoring.
* A GLib thread with a Win32 message loop for hook delivery.
* ``GetCursorPos()`` and ``SetCursorPos()`` for pointer state and movement.
* ``SendInput()`` for mouse buttons, scrolling, virtual-key events, and Unicode
  text input.

Windows security boundaries still apply. An unelevated process should not
expect to automate elevated applications, UAC prompts, or secure desktops.

Linux/X11
---------

The Linux backend targets X11/Xorg:

* XInput2 raw events monitor mouse movement, buttons, scrolls, and keyboard
  events even when another X11 application has focus.
* XTest injects pointer movement, clicks, scroll events, and key events.
* ``XQueryPointer()`` reads the current pointer position.

Text injection on X11 currently supports ASCII text through keyboard symbols.
Windows text injection uses Unicode input events.

Wayland
-------

Wayland intentionally does not expose a generic unprivileged API for
system-wide input hooks or synthetic input. This prevents ordinary desktop
clients from becoming keyloggers or silently driving other applications.

When gautogui detects a Wayland session, it returns
``GAUTOGUI_ERROR_BACKEND_UNAVAILABLE`` instead of pretending it can monitor or
inject input system-wide.

If you are testing an XWayland-only target and accept that native Wayland
applications will not be covered, you can opt into the X11 backend:

.. code-block:: sh

   GAUTOGUI_FORCE_X11=1 sqgi examples/monitor.nut

For reliable Linux GUI automation on modern Wayland desktops, use an X11/Xorg
test session, a nested X server, a VM, or a future privileged backend designed
around explicit evdev/uinput access.
