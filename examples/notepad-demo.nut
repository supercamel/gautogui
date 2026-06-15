local GLib = import("GLib")
local Gautogui = import("Gautogui")

function say(message) {
    print(message + "\n")
    stdout.flush()
}

local controller = Gautogui.Controller.new()
local loop = GLib.MainLoop.new(null, false)
local exit_code = 0

async function main() {
    try {
        local pos = controller.get_mouse_position()
        local x = pos[0]
        local y = pos[1]

        say("Moving the mouse in a small square...")
        controller.move_mouse(x + 40, y)
        await sqgi.sleep(120)
        controller.move_mouse(x + 40, y + 40)
        await sqgi.sleep(120)
        controller.move_mouse(x, y + 40)
        await sqgi.sleep(120)
        controller.move_mouse(x, y)
        await sqgi.sleep(250)

        say("Opening the Windows Start menu...")
        controller.press_key(Gautogui.Key.super)
        await sqgi.sleep(500)

        say("Typing notepad...")
        controller.type_text_with_delay("notepad", 45)
        await sqgi.sleep(250)

        say("Pressing Enter...")
        controller.press_key(Gautogui.Key.enter)
        await sqgi.sleep(1200)

        say("Typing a short line into Notepad...")
        controller.type_text_with_delay("Hello from gautogui via SqGI!\n", 35)

        say("Done.")
    } catch (error) {
        print("Automation demo failed: " + error + "\n")
        stdout.flush()
        exit_code = 1
    }

    loop.quit()
}

sqgi.timeout_add(0, function() {
    main().catch(function(error) {
        print("Automation demo failed: " + error + "\n")
        stdout.flush()
        exit_code = 1
        loop.quit()
    })

    return false
})

loop.run()
return exit_code
