Examples
========

Build-tree environment
----------------------

When examples are run before installation, expose the build-tree library and
typelib:

.. code-block:: sh

   export GI_TYPELIB_PATH=$PWD/build/src
   export LD_LIBRARY_PATH=$PWD/build/src

Vala monitor
------------

Build the Vala example:

.. code-block:: sh

   meson compile -C build gautogui-monitor-vala
   ./build/examples/gautogui-monitor-vala

Source:

.. code-block:: vala

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
           stdout.printf("mouse scrolled: dx=%d dy=%d x=%d y=%d\n",
                         dx, dy, x, y);
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

Vala async automation
---------------------

The generated Vala binding exposes the C ``*_async()``/``*_finish()`` pairs as
Vala ``async`` methods. This example moves the mouse, opens the Windows Start
menu, launches Notepad, and types text.

.. code-block:: vala

   async int automate() {
       var controller = new GAutogui.Controller();

       try {
           int x;
           int y;

           yield controller.get_mouse_position_async(null, out x, out y);

           yield controller.move_mouse_async(x + 40, y, null);
           yield controller.move_mouse_async(x + 40, y + 40, null);
           yield controller.move_mouse_async(x, y + 40, null);
           yield controller.move_mouse_async(x, y, null);

           yield controller.press_key_async(GAutogui.Key.SUPER, null);
           yield controller.type_text_with_delay_async("notepad", 45, null);
           yield controller.press_key_async(GAutogui.Key.ENTER, null);
           yield controller.type_text_with_delay_async(
               "Hello from gautogui via Vala!\n",
               35,
               null);

           return 0;
       } catch (GLib.Error error) {
           stderr.printf("Automation failed: %s\n", error.message);
           return 1;
       }
   }

   int main(string[] args) {
       var loop = new GLib.MainLoop();
       int status = 0;

       automate.begin((obj, result) => {
           status = automate.end(result);
           loop.quit();
       });

       loop.run();
       return status;
   }

SQGI monitor
------------

Run the SQGI monitor from the source tree:

.. code-block:: sh

   sqgi examples/monitor.nut

Source:

.. code-block:: text

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

SQGI async automation
---------------------

The SQGI binding exposes awaitable convenience methods for the GLib async API.
This example is the same Windows-focused Notepad demo installed in
``examples/notepad-demo.nut``.

.. code-block:: sh

   sqgi examples/notepad-demo.nut

Source:

.. code-block:: text

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
           await controller.move_mouse_async(x + 40, y)
           await sqgi.sleep(120)
           await controller.move_mouse_async(x + 40, y + 40)
           await sqgi.sleep(120)
           await controller.move_mouse_async(x, y + 40)
           await sqgi.sleep(120)
           await controller.move_mouse_async(x, y)
           await sqgi.sleep(250)

           say("Opening the Windows Start menu...")
           await controller.press_key_async(Gautogui.Key.super)
           await sqgi.sleep(500)

           say("Typing notepad...")
           await controller.type_text_with_delay_async("notepad", 45)
           await sqgi.sleep(250)

           say("Pressing Enter...")
           await controller.press_key_async(Gautogui.Key.enter)
           await sqgi.sleep(1200)

           say("Typing a short line into Notepad...")
           await controller.type_text_with_delay_async(
               "Hello from gautogui via SQGI!\n",
               35)

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

On Wayland, the automation examples report that no true system-wide backend is
available. Run them from an X11 session or Windows desktop to exercise real
input injection.
