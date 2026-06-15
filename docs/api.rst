API Reference
=============

All public declarations are available through:

.. code-block:: c

   #include <gautogui/gautogui.h>

The library installs a pkg-config file named ``gautogui-1.0`` and a GObject
Introspection namespace named ``Gautogui-1.0``.

Error domain
------------

``GAUTOGUI_ERROR`` is the error domain returned by gautogui APIs. Use
``gautogui_error_quark()`` when comparing error domains in C.

.. list-table::
   :header-rows: 1

   * - Error code
     - Meaning
   * - ``GAUTOGUI_ERROR_BACKEND_UNAVAILABLE``
     - No usable backend is available on the current platform or session.
   * - ``GAUTOGUI_ERROR_PERMISSION_DENIED``
     - The operating system denied access to monitoring or injection.
   * - ``GAUTOGUI_ERROR_INVALID_ARGUMENT``
     - The caller supplied an invalid value, unsupported key, or invalid text.
   * - ``GAUTOGUI_ERROR_PLATFORM_ERROR``
     - A native platform API reported an unexpected failure.

Mouse buttons
-------------

``GAutoguiMouseButton`` is a portable logical mouse button enum:

.. list-table::
   :header-rows: 1

   * - Value
     - Meaning
   * - ``GAUTOGUI_MOUSE_BUTTON_LEFT``
     - Primary pointer button.
   * - ``GAUTOGUI_MOUSE_BUTTON_MIDDLE``
     - Middle pointer button.
   * - ``GAUTOGUI_MOUSE_BUTTON_RIGHT``
     - Secondary pointer button.
   * - ``GAUTOGUI_MOUSE_BUTTON_BACK``
     - Browser or system back side button.
   * - ``GAUTOGUI_MOUSE_BUTTON_FORWARD``
     - Browser or system forward side button.

Keys
----

``GAutoguiKey`` is a portable logical key enum. Printable alphanumeric keys use
their ASCII values so callers can compare them with familiar constants.

Supported key groups:

* ``GAUTOGUI_KEY_NONE``
* ``GAUTOGUI_KEY_A`` through ``GAUTOGUI_KEY_Z``
* ``GAUTOGUI_KEY_0`` through ``GAUTOGUI_KEY_9``
* ``GAUTOGUI_KEY_SPACE``
* ``GAUTOGUI_KEY_MINUS``, ``GAUTOGUI_KEY_EQUAL``
* ``GAUTOGUI_KEY_LEFT_BRACKET``, ``GAUTOGUI_KEY_RIGHT_BRACKET``
* ``GAUTOGUI_KEY_BACKSLASH``, ``GAUTOGUI_KEY_SEMICOLON``
* ``GAUTOGUI_KEY_APOSTROPHE``, ``GAUTOGUI_KEY_GRAVE``
* ``GAUTOGUI_KEY_COMMA``, ``GAUTOGUI_KEY_PERIOD``, ``GAUTOGUI_KEY_SLASH``
* ``GAUTOGUI_KEY_ENTER``, ``GAUTOGUI_KEY_TAB``, ``GAUTOGUI_KEY_ESCAPE``
* ``GAUTOGUI_KEY_BACKSPACE``, ``GAUTOGUI_KEY_DELETE``,
  ``GAUTOGUI_KEY_INSERT``
* ``GAUTOGUI_KEY_HOME``, ``GAUTOGUI_KEY_END``, ``GAUTOGUI_KEY_PAGE_UP``,
  ``GAUTOGUI_KEY_PAGE_DOWN``
* ``GAUTOGUI_KEY_LEFT``, ``GAUTOGUI_KEY_RIGHT``, ``GAUTOGUI_KEY_UP``,
  ``GAUTOGUI_KEY_DOWN``
* ``GAUTOGUI_KEY_SHIFT``, ``GAUTOGUI_KEY_CONTROL``, ``GAUTOGUI_KEY_ALT``,
  ``GAUTOGUI_KEY_SUPER``, ``GAUTOGUI_KEY_CAPS_LOCK``
* ``GAUTOGUI_KEY_F1`` through ``GAUTOGUI_KEY_F24``

Use ``gautogui_controller_type_text()`` for text input. It lets the backend use
the platform's text injection path instead of requiring the caller to map every
character to portable key constants.

Controller construction
-----------------------

``GAutoguiController`` is the main object.

.. code-block:: c

   GAutoguiController *gautogui_controller_new(void);

Creates a controller that emits signals into the current thread-default
``GMainContext`` if one exists, otherwise into the global default context.

.. code-block:: c

   GAutoguiController *
   gautogui_controller_new_for_context(GMainContext *context);

Creates a controller that emits signals into ``context``.

Monitoring lifecycle
--------------------

.. code-block:: c

   gboolean gautogui_controller_start(GAutoguiController *self,
                                      GError **error);

Starts the platform backend and begins monitoring global input. Returns
``TRUE`` on success.

.. code-block:: c

   void gautogui_controller_stop(GAutoguiController *self);

Stops monitoring. It is safe to call this when the controller is already
stopped.

.. code-block:: c

   gboolean gautogui_controller_is_running(GAutoguiController *self);

Returns whether monitoring is currently active.

Signals
-------

Signals are emitted in the controller's ``GMainContext``.

.. list-table::
   :header-rows: 1

   * - Signal
     - Parameters
     - Emitted when
   * - ``mouse-moved``
     - ``gint x, gint y``
     - The system pointer moves.
   * - ``mouse-button-event``
     - ``guint button, gboolean pressed, gint x, gint y``
     - A global pointer button is pressed or released.
   * - ``mouse-clicked``
     - ``guint button, gint x, gint y``
     - A pointer button release completes a click.
   * - ``mouse-scrolled``
     - ``gint dx, gint dy, gint x, gint y``
     - A global wheel or scroll event occurs.
   * - ``key-event``
     - ``guint native_keycode, guint key, gboolean pressed``
     - A global keyboard key is pressed or released.

``native_keycode`` is the platform native virtual key or hardware keycode.
``key`` is a ``GAutoguiKey`` value, or ``GAUTOGUI_KEY_NONE`` when no portable
mapping is available.

Mouse state and injection
-------------------------

.. code-block:: c

   gboolean gautogui_controller_get_mouse_position(GAutoguiController *self,
                                                   gint *x,
                                                   gint *y,
                                                   GError **error);

Reads the current pointer position in screen coordinates. ``x`` and ``y`` are
optional out parameters.

.. code-block:: c

   gboolean gautogui_controller_move_mouse(GAutoguiController *self,
                                           gint x,
                                           gint y,
                                           GError **error);

Moves the system pointer to ``x, y`` in screen coordinates.

.. code-block:: c

   gboolean gautogui_controller_mouse_down(GAutoguiController *self,
                                           GAutoguiMouseButton button,
                                           GError **error);

Sends a mouse button press.

.. code-block:: c

   gboolean gautogui_controller_mouse_up(GAutoguiController *self,
                                         GAutoguiMouseButton button,
                                         GError **error);

Sends a mouse button release.

.. code-block:: c

   gboolean gautogui_controller_click(GAutoguiController *self,
                                      GAutoguiMouseButton button,
                                      GError **error);

Sends a press and release for ``button``.

.. code-block:: c

   gboolean gautogui_controller_scroll(GAutoguiController *self,
                                       gint dx,
                                       gint dy,
                                       GError **error);

Sends a scroll event. Positive ``dy`` scrolls downward. Positive ``dx`` scrolls
rightward.

Keyboard and text injection
---------------------------

.. code-block:: c

   gboolean gautogui_controller_key_down(GAutoguiController *self,
                                         GAutoguiKey key,
                                         GError **error);

Sends a key press.

.. code-block:: c

   gboolean gautogui_controller_key_up(GAutoguiController *self,
                                       GAutoguiKey key,
                                       GError **error);

Sends a key release.

.. code-block:: c

   gboolean gautogui_controller_press_key(GAutoguiController *self,
                                          GAutoguiKey key,
                                          GError **error);

Sends a press and release for ``key``.

.. code-block:: c

   gboolean gautogui_controller_type_text(GAutoguiController *self,
                                          const gchar *utf8,
                                          GError **error);

Types valid UTF-8 text using the backend's default inter-character delay. The
current default is 35 ms.

.. code-block:: c

   gboolean gautogui_controller_type_text_with_delay(GAutoguiController *self,
                                                     const gchar *utf8,
                                                     guint delay_ms,
                                                     GError **error);

Types valid UTF-8 text with an explicit delay in milliseconds between
characters. Pass ``0`` to request no inter-character delay. Use a non-zero delay
when the target application drops keystrokes from bursty synthetic input.

Asynchronous methods
--------------------

Every pointer query and input injection method has a GIO-style asynchronous
form. These methods run the synchronous backend operation in a ``GTask`` worker
thread and complete in the caller's thread-default ``GMainContext``.

Automation operations are serialized per controller so overlapping async calls
do not interleave keystrokes or mouse events. Cancellation is checked before the
worker starts; after native input injection begins, the operation runs to
completion.

.. list-table::
   :header-rows: 1

   * - Synchronous method
     - Async pair
   * - ``gautogui_controller_get_mouse_position()``
     - ``gautogui_controller_get_mouse_position_async()`` and
       ``gautogui_controller_get_mouse_position_finish()``
   * - ``gautogui_controller_move_mouse()``
     - ``gautogui_controller_move_mouse_async()`` and
       ``gautogui_controller_move_mouse_finish()``
   * - ``gautogui_controller_mouse_down()``
     - ``gautogui_controller_mouse_down_async()`` and
       ``gautogui_controller_mouse_down_finish()``
   * - ``gautogui_controller_mouse_up()``
     - ``gautogui_controller_mouse_up_async()`` and
       ``gautogui_controller_mouse_up_finish()``
   * - ``gautogui_controller_click()``
     - ``gautogui_controller_click_async()`` and
       ``gautogui_controller_click_finish()``
   * - ``gautogui_controller_scroll()``
     - ``gautogui_controller_scroll_async()`` and
       ``gautogui_controller_scroll_finish()``
   * - ``gautogui_controller_key_down()``
     - ``gautogui_controller_key_down_async()`` and
       ``gautogui_controller_key_down_finish()``
   * - ``gautogui_controller_key_up()``
     - ``gautogui_controller_key_up_async()`` and
       ``gautogui_controller_key_up_finish()``
   * - ``gautogui_controller_press_key()``
     - ``gautogui_controller_press_key_async()`` and
       ``gautogui_controller_press_key_finish()``
   * - ``gautogui_controller_type_text()``
     - ``gautogui_controller_type_text_async()`` and
       ``gautogui_controller_type_text_finish()``
   * - ``gautogui_controller_type_text_with_delay()``
     - ``gautogui_controller_type_text_with_delay_async()`` and
       ``gautogui_controller_type_text_with_delay_finish()``

The async function shape is:

.. code-block:: c

   void gautogui_controller_move_mouse_async(GAutoguiController *self,
                                             gint x,
                                             gint y,
                                             GCancellable *cancellable,
                                             GAsyncReadyCallback callback,
                                             gpointer user_data);

   gboolean gautogui_controller_move_mouse_finish(GAutoguiController *self,
                                                  GAsyncResult *result,
                                                  GError **error);

Example:

.. code-block:: c

   static void
   on_typed(GObject *source_object, GAsyncResult *result, gpointer user_data)
   {
     GAutoguiController *controller = GAUTOGUI_CONTROLLER(source_object);
     GMainLoop *loop = user_data;
     g_autoptr(GError) error = NULL;

     if (!gautogui_controller_type_text_with_delay_finish(controller,
                                                          result,
                                                          &error)) {
       g_printerr("%s\n", error->message);
     }

     g_main_loop_quit(loop);
   }

   gautogui_controller_type_text_with_delay_async(controller,
                                                  "Hello from gautogui\n",
                                                  35,
                                                  NULL,
                                                  on_typed,
                                                  loop);
