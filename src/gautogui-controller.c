#include "gautogui-backend.h"

#include <gio/gio.h>

struct _GAutoguiController {
  GObject parent_instance;

  GMainContext *context;
  GAutoguiBackend *backend;
  gboolean running;
  GMutex lock;
  GMutex automation_lock;
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

#define DEFAULT_TYPE_TEXT_DELAY_MS 35

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

typedef enum {
  ASYNC_OPERATION_GET_MOUSE_POSITION,
  ASYNC_OPERATION_MOVE_MOUSE,
  ASYNC_OPERATION_MOUSE_DOWN,
  ASYNC_OPERATION_MOUSE_UP,
  ASYNC_OPERATION_CLICK,
  ASYNC_OPERATION_SCROLL,
  ASYNC_OPERATION_KEY_DOWN,
  ASYNC_OPERATION_KEY_UP,
  ASYNC_OPERATION_PRESS_KEY,
  ASYNC_OPERATION_TYPE_TEXT,
} AsyncOperation;

typedef struct {
  AsyncOperation operation;
  gint x;
  gint y;
  gint dx;
  gint dy;
  GAutoguiMouseButton button;
  GAutoguiKey key;
  gchar *utf8;
  guint delay_ms;
} AsyncTaskData;

typedef struct {
  gint x;
  gint y;
} AsyncMousePosition;

static void
async_task_data_free(AsyncTaskData *data)
{
  if (data == NULL)
    return;

  g_free(data->utf8);
  g_free(data);
}

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
  g_mutex_clear(&self->automation_lock);
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
  g_mutex_init(&self->automation_lock);
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

static gboolean
get_backend(GAutoguiController *self,
            GAutoguiBackend **backend,
            GError **error)
{
  g_mutex_lock(&self->lock);
  if (!ensure_backend(self, error)) {
    g_mutex_unlock(&self->lock);
    return FALSE;
  }

  *backend = self->backend;
  g_mutex_unlock(&self->lock);
  return TRUE;
}

static gboolean
controller_get_mouse_position_internal(GAutoguiController *self,
                                       gint *x,
                                       gint *y,
                                       GError **error)
{
  GAutoguiBackend *backend;

  if (!get_backend(self, &backend, error))
    return FALSE;

  return _gautogui_backend_get_mouse_position(backend, x, y, error);
}

static gboolean
controller_move_mouse_internal(GAutoguiController *self,
                               gint x,
                               gint y,
                               GError **error)
{
  GAutoguiBackend *backend;

  if (!get_backend(self, &backend, error))
    return FALSE;

  return _gautogui_backend_move_mouse(backend, x, y, error);
}

static gboolean
controller_mouse_button_internal(GAutoguiController *self,
                                 GAutoguiMouseButton button,
                                 gboolean pressed,
                                 GError **error)
{
  GAutoguiBackend *backend;

  if (!get_backend(self, &backend, error))
    return FALSE;

  return _gautogui_backend_mouse_button(backend, button, pressed, error);
}

static gboolean
controller_scroll_internal(GAutoguiController *self,
                           gint dx,
                           gint dy,
                           GError **error)
{
  GAutoguiBackend *backend;

  if (!get_backend(self, &backend, error))
    return FALSE;

  return _gautogui_backend_scroll(backend, dx, dy, error);
}

static gboolean
controller_key_internal(GAutoguiController *self,
                        GAutoguiKey key,
                        gboolean pressed,
                        GError **error)
{
  GAutoguiBackend *backend;

  if (!get_backend(self, &backend, error))
    return FALSE;

  return _gautogui_backend_key(backend, key, pressed, error);
}

static gboolean
controller_type_text_with_delay_internal(GAutoguiController *self,
                                         const gchar *utf8,
                                         guint delay_ms,
                                         GError **error)
{
  GAutoguiBackend *backend;

  if (utf8 == NULL || !g_utf8_validate(utf8, -1, NULL)) {
    g_set_error(error,
                GAUTOGUI_ERROR,
                GAUTOGUI_ERROR_INVALID_ARGUMENT,
                "Text must be valid UTF-8");
    return FALSE;
  }

  if (!get_backend(self, &backend, error))
    return FALSE;

  return _gautogui_backend_type_text(backend, utf8, delay_ms, error);
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
  g_return_val_if_fail(GAUTOGUI_IS_CONTROLLER(self), FALSE);

  return controller_get_mouse_position_internal(self, x, y, error);
}

gboolean
gautogui_controller_move_mouse(GAutoguiController *self,
                               gint x,
                               gint y,
                               GError **error)
{
  gboolean ok;

  g_return_val_if_fail(GAUTOGUI_IS_CONTROLLER(self), FALSE);

  g_mutex_lock(&self->automation_lock);
  ok = controller_move_mouse_internal(self, x, y, error);
  g_mutex_unlock(&self->automation_lock);

  return ok;
}

gboolean
gautogui_controller_mouse_down(GAutoguiController *self,
                               GAutoguiMouseButton button,
                               GError **error)
{
  gboolean ok;

  g_return_val_if_fail(GAUTOGUI_IS_CONTROLLER(self), FALSE);

  g_mutex_lock(&self->automation_lock);
  ok = controller_mouse_button_internal(self, button, TRUE, error);
  g_mutex_unlock(&self->automation_lock);

  return ok;
}

gboolean
gautogui_controller_mouse_up(GAutoguiController *self,
                             GAutoguiMouseButton button,
                             GError **error)
{
  gboolean ok;

  g_return_val_if_fail(GAUTOGUI_IS_CONTROLLER(self), FALSE);

  g_mutex_lock(&self->automation_lock);
  ok = controller_mouse_button_internal(self, button, FALSE, error);
  g_mutex_unlock(&self->automation_lock);

  return ok;
}

gboolean
gautogui_controller_click(GAutoguiController *self,
                          GAutoguiMouseButton button,
                          GError **error)
{
  gboolean ok;

  g_return_val_if_fail(GAUTOGUI_IS_CONTROLLER(self), FALSE);

  g_mutex_lock(&self->automation_lock);
  if (!controller_mouse_button_internal(self, button, TRUE, error)) {
    g_mutex_unlock(&self->automation_lock);
    return FALSE;
  }

  ok = controller_mouse_button_internal(self, button, FALSE, error);
  g_mutex_unlock(&self->automation_lock);

  return ok;
}

gboolean
gautogui_controller_scroll(GAutoguiController *self,
                           gint dx,
                           gint dy,
                           GError **error)
{
  gboolean ok;

  g_return_val_if_fail(GAUTOGUI_IS_CONTROLLER(self), FALSE);

  g_mutex_lock(&self->automation_lock);
  ok = controller_scroll_internal(self, dx, dy, error);
  g_mutex_unlock(&self->automation_lock);

  return ok;
}

gboolean
gautogui_controller_key_down(GAutoguiController *self,
                             GAutoguiKey key,
                             GError **error)
{
  gboolean ok;

  g_return_val_if_fail(GAUTOGUI_IS_CONTROLLER(self), FALSE);

  g_mutex_lock(&self->automation_lock);
  ok = controller_key_internal(self, key, TRUE, error);
  g_mutex_unlock(&self->automation_lock);

  return ok;
}

gboolean
gautogui_controller_key_up(GAutoguiController *self,
                           GAutoguiKey key,
                           GError **error)
{
  gboolean ok;

  g_return_val_if_fail(GAUTOGUI_IS_CONTROLLER(self), FALSE);

  g_mutex_lock(&self->automation_lock);
  ok = controller_key_internal(self, key, FALSE, error);
  g_mutex_unlock(&self->automation_lock);

  return ok;
}

gboolean
gautogui_controller_press_key(GAutoguiController *self,
                              GAutoguiKey key,
                              GError **error)
{
  gboolean ok;

  g_return_val_if_fail(GAUTOGUI_IS_CONTROLLER(self), FALSE);

  g_mutex_lock(&self->automation_lock);
  if (!controller_key_internal(self, key, TRUE, error)) {
    g_mutex_unlock(&self->automation_lock);
    return FALSE;
  }

  ok = controller_key_internal(self, key, FALSE, error);
  g_mutex_unlock(&self->automation_lock);

  return ok;
}

gboolean
gautogui_controller_type_text(GAutoguiController *self,
                              const gchar *utf8,
                              GError **error)
{
  return gautogui_controller_type_text_with_delay(self,
                                                 utf8,
                                                 DEFAULT_TYPE_TEXT_DELAY_MS,
                                                 error);
}

gboolean
gautogui_controller_type_text_with_delay(GAutoguiController *self,
                                         const gchar *utf8,
                                         guint delay_ms,
                                         GError **error)
{
  gboolean ok;

  g_return_val_if_fail(GAUTOGUI_IS_CONTROLLER(self), FALSE);

  g_mutex_lock(&self->automation_lock);
  ok = controller_type_text_with_delay_internal(self, utf8, delay_ms, error);
  g_mutex_unlock(&self->automation_lock);

  return ok;
}

static void
async_task_return_error(GTask *task,
                        GError *error)
{
  if (error != NULL) {
    g_task_return_error(task, error);
    return;
  }

  g_task_return_new_error(task,
                          GAUTOGUI_ERROR,
                          GAUTOGUI_ERROR_PLATFORM_ERROR,
                          "Automation operation failed");
}

static void
async_task_thread(GTask *task,
                  gpointer source_object,
                  gpointer task_data,
                  GCancellable *cancellable)
{
  GAutoguiController *self = GAUTOGUI_CONTROLLER(source_object);
  AsyncTaskData *data = task_data;
  GError *error = NULL;
  gboolean ok = FALSE;

  (void)cancellable;

  if (g_task_return_error_if_cancelled(task))
    return;

  switch (data->operation) {
  case ASYNC_OPERATION_GET_MOUSE_POSITION: {
    AsyncMousePosition *position = g_new0(AsyncMousePosition, 1);

    if (!gautogui_controller_get_mouse_position(self, &position->x, &position->y, &error)) {
      g_free(position);
      async_task_return_error(task, error);
      return;
    }

    g_task_return_pointer(task, position, g_free);
    return;
  }

  case ASYNC_OPERATION_MOVE_MOUSE:
    ok = gautogui_controller_move_mouse(self, data->x, data->y, &error);
    break;

  case ASYNC_OPERATION_MOUSE_DOWN:
    ok = gautogui_controller_mouse_down(self, data->button, &error);
    break;

  case ASYNC_OPERATION_MOUSE_UP:
    ok = gautogui_controller_mouse_up(self, data->button, &error);
    break;

  case ASYNC_OPERATION_CLICK:
    ok = gautogui_controller_click(self, data->button, &error);
    break;

  case ASYNC_OPERATION_SCROLL:
    ok = gautogui_controller_scroll(self, data->dx, data->dy, &error);
    break;

  case ASYNC_OPERATION_KEY_DOWN:
    ok = gautogui_controller_key_down(self, data->key, &error);
    break;

  case ASYNC_OPERATION_KEY_UP:
    ok = gautogui_controller_key_up(self, data->key, &error);
    break;

  case ASYNC_OPERATION_PRESS_KEY:
    ok = gautogui_controller_press_key(self, data->key, &error);
    break;

  case ASYNC_OPERATION_TYPE_TEXT:
    ok = gautogui_controller_type_text_with_delay(self, data->utf8, data->delay_ms, &error);
    break;
  }

  if (ok)
    g_task_return_boolean(task, TRUE);
  else
    async_task_return_error(task, error);
}

static void
start_async_operation(GAutoguiController *self,
                      AsyncTaskData *data,
                      GCancellable *cancellable,
                      GAsyncReadyCallback callback,
                      gpointer user_data,
                      gpointer source_tag)
{
  GTask *task = g_task_new(self, cancellable, callback, user_data);

  g_task_set_source_tag(task, source_tag);
  g_task_set_task_data(task, data, (GDestroyNotify)async_task_data_free);
  g_task_run_in_thread(task, async_task_thread);
  g_object_unref(task);
}

static gboolean
finish_boolean_operation(GAutoguiController *self,
                         GAsyncResult *result,
                         gpointer source_tag,
                         GError **error)
{
  g_return_val_if_fail(GAUTOGUI_IS_CONTROLLER(self), FALSE);
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  g_return_val_if_fail(g_async_result_is_tagged(result, source_tag), FALSE);

  return g_task_propagate_boolean(G_TASK(result), error);
}

void
gautogui_controller_get_mouse_position_async(GAutoguiController *self,
                                             GCancellable *cancellable,
                                             GAsyncReadyCallback callback,
                                             gpointer user_data)
{
  AsyncTaskData *data;

  g_return_if_fail(GAUTOGUI_IS_CONTROLLER(self));

  data = g_new0(AsyncTaskData, 1);
  data->operation = ASYNC_OPERATION_GET_MOUSE_POSITION;

  start_async_operation(self,
                        data,
                        cancellable,
                        callback,
                        user_data,
                        (gpointer)gautogui_controller_get_mouse_position_async);
}

gboolean
gautogui_controller_get_mouse_position_finish(GAutoguiController *self,
                                              GAsyncResult *result,
                                              gint *x,
                                              gint *y,
                                              GError **error)
{
  AsyncMousePosition *position;

  g_return_val_if_fail(GAUTOGUI_IS_CONTROLLER(self), FALSE);
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  g_return_val_if_fail(g_async_result_is_tagged(result,
                                                (gpointer)gautogui_controller_get_mouse_position_async),
                       FALSE);

  position = g_task_propagate_pointer(G_TASK(result), error);
  if (position == NULL)
    return FALSE;

  if (x != NULL)
    *x = position->x;
  if (y != NULL)
    *y = position->y;

  g_free(position);
  return TRUE;
}

void
gautogui_controller_move_mouse_async(GAutoguiController *self,
                                     gint x,
                                     gint y,
                                     GCancellable *cancellable,
                                     GAsyncReadyCallback callback,
                                     gpointer user_data)
{
  AsyncTaskData *data;

  g_return_if_fail(GAUTOGUI_IS_CONTROLLER(self));

  data = g_new0(AsyncTaskData, 1);
  data->operation = ASYNC_OPERATION_MOVE_MOUSE;
  data->x = x;
  data->y = y;

  start_async_operation(self,
                        data,
                        cancellable,
                        callback,
                        user_data,
                        (gpointer)gautogui_controller_move_mouse_async);
}

gboolean
gautogui_controller_move_mouse_finish(GAutoguiController *self,
                                      GAsyncResult *result,
                                      GError **error)
{
  return finish_boolean_operation(self,
                                  result,
                                  (gpointer)gautogui_controller_move_mouse_async,
                                  error);
}

void
gautogui_controller_mouse_down_async(GAutoguiController *self,
                                     GAutoguiMouseButton button,
                                     GCancellable *cancellable,
                                     GAsyncReadyCallback callback,
                                     gpointer user_data)
{
  AsyncTaskData *data;

  g_return_if_fail(GAUTOGUI_IS_CONTROLLER(self));

  data = g_new0(AsyncTaskData, 1);
  data->operation = ASYNC_OPERATION_MOUSE_DOWN;
  data->button = button;

  start_async_operation(self,
                        data,
                        cancellable,
                        callback,
                        user_data,
                        (gpointer)gautogui_controller_mouse_down_async);
}

gboolean
gautogui_controller_mouse_down_finish(GAutoguiController *self,
                                      GAsyncResult *result,
                                      GError **error)
{
  return finish_boolean_operation(self,
                                  result,
                                  (gpointer)gautogui_controller_mouse_down_async,
                                  error);
}

void
gautogui_controller_mouse_up_async(GAutoguiController *self,
                                   GAutoguiMouseButton button,
                                   GCancellable *cancellable,
                                   GAsyncReadyCallback callback,
                                   gpointer user_data)
{
  AsyncTaskData *data;

  g_return_if_fail(GAUTOGUI_IS_CONTROLLER(self));

  data = g_new0(AsyncTaskData, 1);
  data->operation = ASYNC_OPERATION_MOUSE_UP;
  data->button = button;

  start_async_operation(self,
                        data,
                        cancellable,
                        callback,
                        user_data,
                        (gpointer)gautogui_controller_mouse_up_async);
}

gboolean
gautogui_controller_mouse_up_finish(GAutoguiController *self,
                                    GAsyncResult *result,
                                    GError **error)
{
  return finish_boolean_operation(self,
                                  result,
                                  (gpointer)gautogui_controller_mouse_up_async,
                                  error);
}

void
gautogui_controller_click_async(GAutoguiController *self,
                                GAutoguiMouseButton button,
                                GCancellable *cancellable,
                                GAsyncReadyCallback callback,
                                gpointer user_data)
{
  AsyncTaskData *data;

  g_return_if_fail(GAUTOGUI_IS_CONTROLLER(self));

  data = g_new0(AsyncTaskData, 1);
  data->operation = ASYNC_OPERATION_CLICK;
  data->button = button;

  start_async_operation(self,
                        data,
                        cancellable,
                        callback,
                        user_data,
                        (gpointer)gautogui_controller_click_async);
}

gboolean
gautogui_controller_click_finish(GAutoguiController *self,
                                 GAsyncResult *result,
                                 GError **error)
{
  return finish_boolean_operation(self,
                                  result,
                                  (gpointer)gautogui_controller_click_async,
                                  error);
}

void
gautogui_controller_scroll_async(GAutoguiController *self,
                                 gint dx,
                                 gint dy,
                                 GCancellable *cancellable,
                                 GAsyncReadyCallback callback,
                                 gpointer user_data)
{
  AsyncTaskData *data;

  g_return_if_fail(GAUTOGUI_IS_CONTROLLER(self));

  data = g_new0(AsyncTaskData, 1);
  data->operation = ASYNC_OPERATION_SCROLL;
  data->dx = dx;
  data->dy = dy;

  start_async_operation(self,
                        data,
                        cancellable,
                        callback,
                        user_data,
                        (gpointer)gautogui_controller_scroll_async);
}

gboolean
gautogui_controller_scroll_finish(GAutoguiController *self,
                                  GAsyncResult *result,
                                  GError **error)
{
  return finish_boolean_operation(self,
                                  result,
                                  (gpointer)gautogui_controller_scroll_async,
                                  error);
}

void
gautogui_controller_key_down_async(GAutoguiController *self,
                                   GAutoguiKey key,
                                   GCancellable *cancellable,
                                   GAsyncReadyCallback callback,
                                   gpointer user_data)
{
  AsyncTaskData *data;

  g_return_if_fail(GAUTOGUI_IS_CONTROLLER(self));

  data = g_new0(AsyncTaskData, 1);
  data->operation = ASYNC_OPERATION_KEY_DOWN;
  data->key = key;

  start_async_operation(self,
                        data,
                        cancellable,
                        callback,
                        user_data,
                        (gpointer)gautogui_controller_key_down_async);
}

gboolean
gautogui_controller_key_down_finish(GAutoguiController *self,
                                    GAsyncResult *result,
                                    GError **error)
{
  return finish_boolean_operation(self,
                                  result,
                                  (gpointer)gautogui_controller_key_down_async,
                                  error);
}

void
gautogui_controller_key_up_async(GAutoguiController *self,
                                 GAutoguiKey key,
                                 GCancellable *cancellable,
                                 GAsyncReadyCallback callback,
                                 gpointer user_data)
{
  AsyncTaskData *data;

  g_return_if_fail(GAUTOGUI_IS_CONTROLLER(self));

  data = g_new0(AsyncTaskData, 1);
  data->operation = ASYNC_OPERATION_KEY_UP;
  data->key = key;

  start_async_operation(self,
                        data,
                        cancellable,
                        callback,
                        user_data,
                        (gpointer)gautogui_controller_key_up_async);
}

gboolean
gautogui_controller_key_up_finish(GAutoguiController *self,
                                  GAsyncResult *result,
                                  GError **error)
{
  return finish_boolean_operation(self,
                                  result,
                                  (gpointer)gautogui_controller_key_up_async,
                                  error);
}

void
gautogui_controller_press_key_async(GAutoguiController *self,
                                    GAutoguiKey key,
                                    GCancellable *cancellable,
                                    GAsyncReadyCallback callback,
                                    gpointer user_data)
{
  AsyncTaskData *data;

  g_return_if_fail(GAUTOGUI_IS_CONTROLLER(self));

  data = g_new0(AsyncTaskData, 1);
  data->operation = ASYNC_OPERATION_PRESS_KEY;
  data->key = key;

  start_async_operation(self,
                        data,
                        cancellable,
                        callback,
                        user_data,
                        (gpointer)gautogui_controller_press_key_async);
}

gboolean
gautogui_controller_press_key_finish(GAutoguiController *self,
                                     GAsyncResult *result,
                                     GError **error)
{
  return finish_boolean_operation(self,
                                  result,
                                  (gpointer)gautogui_controller_press_key_async,
                                  error);
}

void
gautogui_controller_type_text_async(GAutoguiController *self,
                                    const gchar *utf8,
                                    GCancellable *cancellable,
                                    GAsyncReadyCallback callback,
                                    gpointer user_data)
{
  AsyncTaskData *data;

  g_return_if_fail(GAUTOGUI_IS_CONTROLLER(self));

  data = g_new0(AsyncTaskData, 1);
  data->operation = ASYNC_OPERATION_TYPE_TEXT;
  data->utf8 = g_strdup(utf8);
  data->delay_ms = DEFAULT_TYPE_TEXT_DELAY_MS;

  start_async_operation(self,
                        data,
                        cancellable,
                        callback,
                        user_data,
                        (gpointer)gautogui_controller_type_text_async);
}

gboolean
gautogui_controller_type_text_finish(GAutoguiController *self,
                                     GAsyncResult *result,
                                     GError **error)
{
  return finish_boolean_operation(self,
                                  result,
                                  (gpointer)gautogui_controller_type_text_async,
                                  error);
}

void
gautogui_controller_type_text_with_delay_async(GAutoguiController *self,
                                               const gchar *utf8,
                                               guint delay_ms,
                                               GCancellable *cancellable,
                                               GAsyncReadyCallback callback,
                                               gpointer user_data)
{
  AsyncTaskData *data;

  g_return_if_fail(GAUTOGUI_IS_CONTROLLER(self));

  data = g_new0(AsyncTaskData, 1);
  data->operation = ASYNC_OPERATION_TYPE_TEXT;
  data->utf8 = g_strdup(utf8);
  data->delay_ms = delay_ms;

  start_async_operation(self,
                        data,
                        cancellable,
                        callback,
                        user_data,
                        (gpointer)gautogui_controller_type_text_with_delay_async);
}

gboolean
gautogui_controller_type_text_with_delay_finish(GAutoguiController *self,
                                                GAsyncResult *result,
                                                GError **error)
{
  return finish_boolean_operation(self,
                                  result,
                                  (gpointer)gautogui_controller_type_text_with_delay_async,
                                  error);
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
