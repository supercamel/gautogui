local GLib = import("GLib")
local Gautogui = import("Gautogui")

function sleep_ms(ms) {
    GLib.usleep(ms * 1000)
}

function say(message) {
    print(message + "\n")
    stdout.flush()
}

local controller = Gautogui.Controller.new()

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

    say("Opening the Windows Start menu...")
    controller.press_key(Gautogui.Key.super)
    sleep_ms(500)

    say("Typing notepad...")
    controller.type_text_with_delay("notepad", 45)
    sleep_ms(250)

    say("Pressing Enter...")
    controller.press_key(Gautogui.Key.enter)
    sleep_ms(1200)

    say("Typing a short line into Notepad...")
    controller.type_text_with_delay("Hello from gautogui via SqGI!\n", 35)

    say("Done.")
} catch (error) {
    print("Automation demo failed: " + error + "\n")
    stdout.flush()
    return 1
}

return 0
