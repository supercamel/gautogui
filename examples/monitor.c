#include <gautogui/gautogui.h>

static void
on_mouse_moved(GAutoguiController *controller,
               gint x,
               gint y,
               gpointer user_data)
{
  (void)controller;
  (void)user_data;

  g_print("mouse moved: x=%d y=%d\n", x, y);
}

static void
on_mouse_button_event(GAutoguiController *controller,
                      guint button,
                      gboolean pressed,
                      gint x,
                      gint y,
                      gpointer user_data)
{
  (void)controller;
  (void)user_data;

  g_print("mouse button: button=%u %s x=%d y=%d\n",
          button,
          pressed ? "down" : "up",
          x,
          y);
}

static void
on_mouse_scrolled(GAutoguiController *controller,
                  gint dx,
                  gint dy,
                  gint x,
                  gint y,
                  gpointer user_data)
{
  (void)controller;
  (void)user_data;

  g_print("mouse scrolled: dx=%d dy=%d x=%d y=%d\n", dx, dy, x, y);
}

static void
on_key_event(GAutoguiController *controller,
             guint native_keycode,
             guint key,
             gboolean pressed,
             gpointer user_data)
{
  (void)controller;
  (void)user_data;

  g_print("key: native=%u key=%u %s\n",
          native_keycode,
          key,
          pressed ? "down" : "up");
}

int
main(void)
{
  g_autoptr(GAutoguiController) controller = NULL;
  g_autoptr(GMainLoop) loop = NULL;
  g_autoptr(GError) error = NULL;

  controller = gautogui_controller_new();
  g_signal_connect(controller, "mouse-moved", G_CALLBACK(on_mouse_moved), NULL);
  g_signal_connect(controller, "mouse-button-event", G_CALLBACK(on_mouse_button_event), NULL);
  g_signal_connect(controller, "mouse-scrolled", G_CALLBACK(on_mouse_scrolled), NULL);
  g_signal_connect(controller, "key-event", G_CALLBACK(on_key_event), NULL);

  if (!gautogui_controller_start(controller, &error)) {
    g_printerr("Unable to start gautogui monitor: %s\n", error->message);
    return 1;
  }

  g_print("Monitoring system-wide input. Press Ctrl+C to quit.\n");

  loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(loop);

  return 0;
}
