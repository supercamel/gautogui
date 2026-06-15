#!/usr/bin/env python3

import sys

import gi

gi.require_version("Gautogui", "1.0")

from gi.repository import GLib, Gautogui


def on_mouse_moved(controller, x, y):
    print(f"mouse moved: x={x} y={y}")


def on_mouse_button_event(controller, button, pressed, x, y):
    state = "down" if pressed else "up"
    print(f"mouse button: button={button} {state} x={x} y={y}")


def on_mouse_scrolled(controller, dx, dy, x, y):
    print(f"mouse scrolled: dx={dx} dy={dy} x={x} y={y}")


def on_key_event(controller, native_keycode, key, pressed):
    state = "down" if pressed else "up"
    print(f"key: native={native_keycode} key={key} {state}")


def main():
    controller = Gautogui.Controller.new()
    controller.connect("mouse-moved", on_mouse_moved)
    controller.connect("mouse-button-event", on_mouse_button_event)
    controller.connect("mouse-scrolled", on_mouse_scrolled)
    controller.connect("key-event", on_key_event)

    try:
        controller.start()
    except GLib.Error as exc:
        print(f"Unable to start gautogui monitor: {exc.message}", file=sys.stderr)
        return 1

    print("Monitoring system-wide input. Press Ctrl+C to quit.", flush=True)

    loop = GLib.MainLoop()
    try:
        loop.run()
    except KeyboardInterrupt:
        controller.stop()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
