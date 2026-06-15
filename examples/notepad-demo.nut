local GLib = import("GLib")
local Gautogui = import("Gautogui")
local controller = null

function sleep_ms(ms) {
    GLib.usleep(ms * 1000)
}

function say(message) {
    print(message + "\n")
    stdout.flush()
}

function tap_key(key) {
    controller.key_down(key)
    sleep_ms(60)
    controller.key_up(key)
    sleep_ms(90)
}

function type_slow(text, delay_ms) {
    for (local i = 0; i < text.len(); i++) {
        controller.type_text(text.slice(i, i + 1))
        sleep_ms(delay_ms)
    }
}

function release_modifiers() {
    controller.key_up(Gautogui.Key.super)
    controller.key_up(Gautogui.Key.shift)
    controller.key_up(Gautogui.Key.control)
    controller.key_up(Gautogui.Key.alt)
    sleep_ms(120)
}

controller = Gautogui.Controller.new()

try {
    local pos = controller.get_mouse_position()
    local x = pos[0]
    local y = pos[1]

    say("Moving the mouse in a small square...")
    controller.move_mouse(x + 40, y)
    sleep_ms(120)
    controller.move_mouse(x + 40, y + 40)
    sleep_ms(120)
    controller.move_mouse(x, y + 40)
    sleep_ms(120)
    controller.move_mouse(x, y)
    sleep_ms(250)

    release_modifiers()

    say("Opening the Windows Run dialog...")
    controller.key_down(Gautogui.Key.super)
    sleep_ms(80)
    tap_key(Gautogui.Key.r)
    controller.key_up(Gautogui.Key.super)
    sleep_ms(500)

    say("Typing notepad...")
    type_slow("notepad", 45)
    sleep_ms(250)

    say("Pressing Enter...")
    tap_key(Gautogui.Key.enter)
    sleep_ms(1800)

    release_modifiers()

    say("Typing a short line into Notepad...")
    type_slow("Hello from gautogui via SqGI!\n", 35)

    say("Done.")
} catch (error) {
    print("Automation demo failed: " + error + "\n")
    stdout.flush()
    return 1
}

return 0
