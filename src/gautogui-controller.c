#include "gautogui-backend.h"

struct _GAutoguiController {
  GObject parent_instance;

  GMainContext *context;
  GAutoguiBackend *backend;
  gboolean running;
  GMutex lock;
};

G_DEFINE_TYPE(GAutoguiController, gautogui_controller, G_TYPE_OBJECT)
G_DEFINE_QUARK(gautogui-error-quark, gautogui_error)

enum {
  SIGNAL_MOUSE_MOVED,
  SIGNAL_MOUSE_BUTTON_EVENT,
  SIGNAL_MOUSE_CLICKED,
  SIGNAL_MOUSE_SCROLLED,
  SIGNAL_KEY_EVENT,
  N_SIGNALS,
};

static guint signals[N_SIGNALS];

typedef struct {
  GAutoguiController *self;
  gint x;
  gint y;
} MouseMovedEvent;

typedef struct {
  GAutoguiController *self;
  GAutoguiMouseButton button;
  gboolean pressed;
  gint x;
  gint y;
} MouseButtonEvent;

typedef struct {
  GAutoguiController *self;
  gint dx;
  gint dy;
  gint x;
  gint y;
} MouseScrollEvent;

typedef struct {
  GAutoguiController *self;
  guint native_keycode;
  GAutoguiKey key;
  gboolean pressed;
} KeyEvent;

static gboolean
emit_mouse_moved_cb(gpointer user_data)
{
  MouseMovedEvent *event = user_data;

  g_signal_emit(event->self, signals[SIGNAL_MOUSE_MOVED], 0, event->x, event->y);

  g_object_unref(event->self);
  g_free(event);
  return G_SOURCE_REMOVE;
}

static gboolean
emit_mouse_button_cb(gpointer user_data)
{
  MouseButtonEvent *event = user_data;

  g_signal_emit(event->self,
                signals[SIGNAL_MOUSE_BUTTON_EVENT],
                0,
                event->button,
                event->pressed,
                event->x,
                event->y);

  if (!event->pressed) {
    g_signal_emit(event->self,
                  signals[SIGNAL_MOUSE_CLICKED],
                  0,
                  event->button,
                  event->x,
                  event->y);
  }

  g_object_unref(event->self);
  g_free(event);
  return G_SOURCE_REMOVE;
}

static gboolean
emit_mouse_scroll_cb(gpointer user_data)
{
  MouseScrollEvent *event = user_data;

  g_signal_emit(event->self,
                signals[SIGNAL_MOUSE_SCROLLED],
                0,
                event->dx,
                event->dy,
                event->x,
                event->y);

  g_object_unref(event->self);
  g_free(event);
  return G_SOURCE_REMOVE;
}

static gboolean
emit_key_cb(gpointer user_data)
{
  KeyEvent *event = user_data;

  g_signal_emit(event->self,
                signals[SIGNAL_KEY_EVENT],
                0,
                event->native_keycode,
                event->key,
                event->pressed);

  g_object_unref(event->self);
  g_free(event);
  return G_SOURCE_REMOVE;
}

static void
gautogui_controller_finalize(GObject *object)
{
  GAutoguiController *self = GAUTOGUI_CONTROLLER(object);

  gautogui_controller_stop(self);

  if (self->backend != NULL) {
    _gautogui_backend_free(self->backend);
    self->backend = NULL;
  }

  g_clear_pointer(&self->context, g_main_context_unref);
  g_mutex_clear(&self->lock);

  G_OBJECT_CLASS(gautogui_controller_parent_class)->finalize(object);
}

static void
gautogui_controller_class_init(GAutoguiControllerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->finalize = gautogui_controller_finalize;

  /**
   * GAutoguiController::mouse-moved:
   * @self: the controller
   * @x: screen X coordinate in pixels
   * @y: screen Y coordinate in pixels
   *
   * Emitted when the system pointer moves.
   */
  signals[SIGNAL_MOUSE_MOVED] =
    g_signal_new("mouse-moved",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_LAST,
                 0,
                 NULL,
                 NULL,
                 NULL,
                 G_TYPE_NONE,
                 2,
                 G_TYPE_INT,
                 G_TYPE_INT);

  /**
   * GAutoguiController::mouse-button-event:
   * @self: the controller
   * @button: a #GAutoguiMouseButton
   * @pressed: %TRUE for button down, %FALSE for button up
   * @x: screen X coordinate in pixels
   * @y: screen Y coordinate in pixels
   *
   * Emitted for global pointer button press and release events.
   */
  signals[SIGNAL_MOUSE_BUTTON_EVENT] =
    g_signal_new("mouse-button-event",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_LAST,
                 0,
                 NULL,
                 NULL,
                 NULL,
                 G_TYPE_NONE,
                 4,
                 G_TYPE_UINT,
                 G_TYPE_BOOLEAN,
                 G_TYPE_INT,
                 G_TYPE_INT);

  /**
   * GAutoguiController::mouse-clicked:
   * @self: the controller
   * @button: a #GAutoguiMouseButton
   * @x: screen X coordinate in pixels
   * @y: screen Y coordinate in pixels
   *
   * Emitted after a global pointer button release.
   */
  signals[SIGNAL_MOUSE_CLICKED] =
    g_signal_new("mouse-clicked",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_LAST,
                 0,
                 NULL,
                 NULL,
                 NULL,
                 G_TYPE_NONE,
                 3,
                 G_TYPE_UINT,
                 G_TYPE_INT,
                 G_TYPE_INT);

  /**
   * GAutoguiController::mouse-scrolled:
   * @self: the controller
   * @dx: horizontal scroll clicks, positive to the right
   * @dy: vertical scroll clicks, positive downward
   * @x: screen X coordinate in pixels
   * @y: screen Y coordinate in pixels
   *
   * Emitted for global pointer wheel events.
   */
  signals[SIGNAL_MOUSE_SCROLLED] =
    g_signal_new("mouse-scrolled",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_LAST,
                 0,
                 NULL,
                 NULL,
                 NULL,
                 G_TYPE_NONE,
                 4,
                 G_TYPE_INT,
                 G_TYPE_INT,
                 G_TYPE_INT,
                 G_TYPE_INT);

  /**
   * GAutoguiController::key-event:
   * @self: the controller
   * @native_keycode: platform native virtual key or hardware keycode
   * @key: a #GAutoguiKey, or %GAUTOGUI_KEY_NONE when unmapped
   * @pressed: %TRUE for key down, %FALSE for key up
   *
   * Emitted for global keyboard press and release events.
   */
  signals[SIGNAL_KEY_EVENT] =
    g_signal_new("key-event",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_LAST,
                 0,
                 NULL,
                 NULL,
                 NULL,
                 G_TYPE_NONE,
                 3,
                 G_TYPE_UINT,
                 G_TYPE_UINT,
                 G_TYPE_BOOLEAN);
}

static void
gautogui_controller_init(GAutoguiController *self)
{
  GMainContext *thread_context = g_main_context_get_thread_default();

  self->context = g_main_context_ref(thread_context != NULL ? thread_context : g_main_context_default());
  g_mutex_init(&self->lock);
}

static gboolean
ensure_backend(GAutoguiController *self,
               GError **error)
{
  if (self->backend != NULL)
    return TRUE;

  self->backend = _gautogui_backend_new(self, error);
  return self->backend != NULL;
}

GAutoguiController *
gautogui_controller_new(void)
{
  return g_object_new(GAUTOGUI_TYPE_CONTROLLER, NULL);
}

GAutoguiController *
gautogui_controller_new_for_context(GMainContext *context)
{
  GAutoguiController *self = g_object_new(GAUTOGUI_TYPE_CONTROLLER, NULL);

  if (context != NULL) {
    g_main_context_unref(self->context);
    self->context = g_main_context_ref(context);
  }

  return self;
}

gboolean
gautogui_controller_start(GAutoguiController *self,
                          GError **error)
{
  gboolean ok;

  g_return_val_if_fail(GAUTOGUI_IS_CONTROLLER(self), FALSE);

  g_mutex_lock(&self->lock);

  if (self->running) {
    g_mutex_unlock(&self->lock);
    return TRUE;
  }

  ok = ensure_backend(self, error);
  if (ok)
    ok = _gautogui_backend_start(self->backend, error);

  self->running = ok;
  g_mutex_unlock(&self->lock);

  return ok;
}

void
gautogui_controller_stop(GAutoguiController *self)
{
  GAutoguiBackend *backend = NULL;

  g_return_if_fail(GAUTOGUI_IS_CONTROLLER(self));

  g_mutex_lock(&self->lock);
  if (self->running) {
    self->running = FALSE;
    backend = self->backend;
  }
  g_mutex_unlock(&self->lock);

  if (backend != NULL)
    _gautogui_backend_stop(backend);
}

gboolean
gautogui_controller_is_running(GAutoguiController *self)
{
  gboolean running;

  g_return_val_if_fail(GAUTOGUI_IS_CONTROLLER(self), FALSE);

  g_mutex_lock(&self->lock);
  running = self->running;
  g_mutex_unlock(&self->lock);

  return running;
}

gboolean
gautogui_controller_get_mouse_position(GAutoguiController *self,
                                       gint *x,
                                       gint *y,
                                       GError **error)
{
  GAutoguiBackend *backend;

  g_return_val_if_fail(GAUTOGUI_IS_CONTROLLER(self), FALSE);

  g_mutex_lock(&self->lock);
  if (!ensure_backend(self, error)) {
    g_mutex_unlock(&self->lock);
    return FALSE;
  }
  backend = self->backend;
  g_mutex_unlock(&self->lock);

  return _gautogui_backend_get_mouse_position(backend, x, y, error);
}

gboolean
gautogui_controller_move_mouse(GAutoguiController *self,
                               gint x,
                               gint y,
                               GError **error)
{
  GAutoguiBackend *backend;

  g_return_val_if_fail(GAUTOGUI_IS_CONTROLLER(self), FALSE);

  g_mutex_lock(&self->lock);
  if (!ensure_backend(self, error)) {
    g_mutex_unlock(&self->lock);
    return FALSE;
  }
  backend = self->backend;
  g_mutex_unlock(&self->lock);

  return _gautogui_backend_move_mouse(backend, x, y, error);
}

gboolean
gautogui_controller_mouse_down(GAutoguiController *self,
                               GAutoguiMouseButton button,
                               GError **error)
{
  GAutoguiBackend *backend;

  g_return_val_if_fail(GAUTOGUI_IS_CONTROLLER(self), FALSE);

  g_mutex_lock(&self->lock);
  if (!ensure_backend(self, error)) {
    g_mutex_unlock(&self->lock);
    return FALSE;
  }
  backend = self->backend;
  g_mutex_unlock(&self->lock);

  return _gautogui_backend_mouse_button(backend, button, TRUE, error);
}

gboolean
gautogui_controller_mouse_up(GAutoguiController *self,
                             GAutoguiMouseButton button,
                             GError **error)
{
  GAutoguiBackend *backend;

  g_return_val_if_fail(GAUTOGUI_IS_CONTROLLER(self), FALSE);

  g_mutex_lock(&self->lock);
  if (!ensure_backend(self, error)) {
    g_mutex_unlock(&self->lock);
    return FALSE;
  }
  backend = self->backend;
  g_mutex_unlock(&self->lock);

  return _gautogui_backend_mouse_button(backend, button, FALSE, error);
}

gboolean
gautogui_controller_click(GAutoguiController *self,
                          GAutoguiMouseButton button,
                          GError **error)
{
  g_return_val_if_fail(GAUTOGUI_IS_CONTROLLER(self), FALSE);

  if (!gautogui_controller_mouse_down(self, button, error))
    return FALSE;

  return gautogui_controller_mouse_up(self, button, error);
}

gboolean
gautogui_controller_scroll(GAutoguiController *self,
                           gint dx,
                           gint dy,
                           GError **error)
{
  GAutoguiBackend *backend;

  g_return_val_if_fail(GAUTOGUI_IS_CONTROLLER(self), FALSE);

  g_mutex_lock(&self->lock);
  if (!ensure_backend(self, error)) {
    g_mutex_unlock(&self->lock);
    return FALSE;
  }
  backend = self->backend;
  g_mutex_unlock(&self->lock);

  return _gautogui_backend_scroll(backend, dx, dy, error);
}

gboolean
gautogui_controller_key_down(GAutoguiController *self,
                             GAutoguiKey key,
                             GError **error)
{
  GAutoguiBackend *backend;

  g_return_val_if_fail(GAUTOGUI_IS_CONTROLLER(self), FALSE);

  g_mutex_lock(&self->lock);
  if (!ensure_backend(self, error)) {
    g_mutex_unlock(&self->lock);
    return FALSE;
  }
  backend = self->backend;
  g_mutex_unlock(&self->lock);

  return _gautogui_backend_key(backend, key, TRUE, error);
}

gboolean
gautogui_controller_key_up(GAutoguiController *self,
                           GAutoguiKey key,
                           GError **error)
{
  GAutoguiBackend *backend;

  g_return_val_if_fail(GAUTOGUI_IS_CONTROLLER(self), FALSE);

  g_mutex_lock(&self->lock);
  if (!ensure_backend(self, error)) {
    g_mutex_unlock(&self->lock);
    return FALSE;
  }
  backend = self->backend;
  g_mutex_unlock(&self->lock);

  return _gautogui_backend_key(backend, key, FALSE, error);
}

gboolean
gautogui_controller_press_key(GAutoguiController *self,
                              GAutoguiKey key,
                              GError **error)
{
  g_return_val_if_fail(GAUTOGUI_IS_CONTROLLER(self), FALSE);

  if (!gautogui_controller_key_down(self, key, error))
    return FALSE;

  return gautogui_controller_key_up(self, key, error);
}

gboolean
gautogui_controller_type_text(GAutoguiController *self,
                              const gchar *utf8,
                              GError **error)
{
  GAutoguiBackend *backend;

  g_return_val_if_fail(GAUTOGUI_IS_CONTROLLER(self), FALSE);

  if (utf8 == NULL || !g_utf8_validate(utf8, -1, NULL)) {
    g_set_error(error,
                GAUTOGUI_ERROR,
                GAUTOGUI_ERROR_INVALID_ARGUMENT,
                "Text must be valid UTF-8");
    return FALSE;
  }

  g_mutex_lock(&self->lock);
  if (!ensure_backend(self, error)) {
    g_mutex_unlock(&self->lock);
    return FALSE;
  }
  backend = self->backend;
  g_mutex_unlock(&self->lock);

  return _gautogui_backend_type_text(backend, utf8, error);
}

void
_gautogui_controller_emit_mouse_moved(GAutoguiController *self,
                                      gint x,
                                      gint y)
{
  MouseMovedEvent *event;

  g_return_if_fail(GAUTOGUI_IS_CONTROLLER(self));

  event = g_new0(MouseMovedEvent, 1);
  event->self = g_object_ref(self);
  event->x = x;
  event->y = y;

  g_main_context_invoke_full(self->context, G_PRIORITY_DEFAULT, emit_mouse_moved_cb, event, NULL);
}

void
_gautogui_controller_emit_mouse_button(GAutoguiController *self,
                                       GAutoguiMouseButton button,
                                       gboolean pressed,
                                       gint x,
                                       gint y)
{
  MouseButtonEvent *event;

  g_return_if_fail(GAUTOGUI_IS_CONTROLLER(self));

  event = g_new0(MouseButtonEvent, 1);
  event->self = g_object_ref(self);
  event->button = button;
  event->pressed = pressed;
  event->x = x;
  event->y = y;

  g_main_context_invoke_full(self->context, G_PRIORITY_DEFAULT, emit_mouse_button_cb, event, NULL);
}

void
_gautogui_controller_emit_mouse_scroll(GAutoguiController *self,
                                       gint dx,
                                       gint dy,
                                       gint x,
                                       gint y)
{
  MouseScrollEvent *event;

  g_return_if_fail(GAUTOGUI_IS_CONTROLLER(self));

  event = g_new0(MouseScrollEvent, 1);
  event->self = g_object_ref(self);
  event->dx = dx;
  event->dy = dy;
  event->x = x;
  event->y = y;

  g_main_context_invoke_full(self->context, G_PRIORITY_DEFAULT, emit_mouse_scroll_cb, event, NULL);
}

void
_gautogui_controller_emit_key(GAutoguiController *self,
                              guint native_keycode,
                              GAutoguiKey key,
                              gboolean pressed)
{
  KeyEvent *event;

  g_return_if_fail(GAUTOGUI_IS_CONTROLLER(self));

  event = g_new0(KeyEvent, 1);
  event->self = g_object_ref(self);
  event->native_keycode = native_keycode;
  event->key = key;
  event->pressed = pressed;

  g_main_context_invoke_full(self->context, G_PRIORITY_DEFAULT, emit_key_cb, event, NULL);
}
