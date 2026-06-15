int main(string[] args) {
    var controller = new GAutogui.Controller();
    var loop = new GLib.MainLoop();

    controller.mouse_moved.connect((x, y) => {
        stdout.printf("mouse moved: x=%d y=%d\n", x, y);
    });

    controller.mouse_button_event.connect((button, pressed, x, y) => {
        stdout.printf("mouse button: button=%u %s x=%d y=%d\n",
                      button,
                      pressed ? "down" : "up",
                      x,
                      y);
    });

    controller.mouse_scrolled.connect((dx, dy, x, y) => {
        stdout.printf("mouse scrolled: dx=%d dy=%d x=%d y=%d\n", dx, dy, x, y);
    });

    controller.key_event.connect((native_keycode, key, pressed) => {
        stdout.printf("key: native=%u key=%u %s\n",
                      native_keycode,
                      key,
                      pressed ? "down" : "up");
    });

    try {
        controller.start();
    } catch (GLib.Error error) {
        stderr.printf("Unable to start gautogui monitor: %s\n", error.message);
        return 1;
    }

    stdout.printf("Monitoring system-wide input. Press Ctrl+C to quit.\n");
    stdout.flush();
    loop.run();

    return 0;
}
