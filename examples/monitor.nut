local GLib = import("GLib")
local Gautogui = import("Gautogui")

local controller = Gautogui.Controller.new()
local loop = GLib.MainLoop.new(null, false)

controller.connect("mouse-moved", function(x, y) {
    printf("mouse moved: x=%d y=%d\n", x, y)
    stdout.flush()
})

controller.connect("mouse-button-event", function(button, pressed, x, y) {
    printf("mouse button: button=%u %s x=%d y=%d\n",
           button,
           pressed ? "down" : "up",
           x,
           y)
    stdout.flush()
})

controller.connect("mouse-scrolled", function(dx, dy, x, y) {
    printf("mouse scrolled: dx=%d dy=%d x=%d y=%d\n", dx, dy, x, y)
    stdout.flush()
})

controller.connect("key-event", function(native_keycode, key, pressed) {
    printf("key: native=%u key=%u %s\n",
           native_keycode,
           key,
           pressed ? "down" : "up")
    stdout.flush()
})

try {
    controller.start()
} catch (error) {
    print("Unable to start gautogui monitor: " + error + "\n")
    return 1
}

print("Monitoring system-wide input. Press Ctrl+C to quit.\n")
stdout.flush()
loop.run()

return 0
