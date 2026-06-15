gautogui
========

gautogui is a GObject/C library for system-wide GUI input monitoring and
automation. It can emit signals for global mouse and keyboard input, read and
move the pointer, send clicks, scrolls, key presses, and type text through the
operating system's input APIs.

The library is designed for GUI test automation and language bindings. The C
API is synchronous by default, with optional GIO-style ``*_async()`` and
``*_finish()`` methods for applications that run a GLib main loop.

Platform support is intentionally explicit:

* Windows uses low-level hooks and ``SendInput``.
* Linux uses X11/Xorg through XInput2 and XTest.
* Wayland does not provide a generic unprivileged API for system-wide hooks or
  input injection, so gautogui reports a backend error there.

Contents
--------

.. toctree::
   :maxdepth: 2

   install
   platforms
   api
   examples
   development
