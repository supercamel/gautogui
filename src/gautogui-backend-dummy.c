#include "gautogui-backend.h"

struct _GAutoguiBackend {
  GAutoguiController *controller;
};

GAutoguiBackend *
_gautogui_backend_new(GAutoguiController *controller,
                      GError **error)
{
  g_set_error(error,
              GAUTOGUI_ERROR,
              GAUTOGUI_ERROR_BACKEND_UNAVAILABLE,
              "gautogui does not have a backend for this platform");
  (void)controller;
  return NULL;
}

void
_gautogui_backend_free(GAutoguiBackend *backend)
{
  g_free(backend);
}

gboolean
_gautogui_backend_start(GAutoguiBackend *backend,
                        GError **error)
{
  (void)backend;
  g_set_error(error,
              GAUTOGUI_ERROR,
              GAUTOGUI_ERROR_BACKEND_UNAVAILABLE,
              "gautogui does not have a backend for this platform");
  return FALSE;
}

void
_gautogui_backend_stop(GAutoguiBackend *backend)
{
  (void)backend;
}

gboolean
_gautogui_backend_get_mouse_position(GAutoguiBackend *backend,
                                     gint *x,
                                     gint *y,
                                     GError **error)
{
  (void)backend;
  (void)x;
  (void)y;
  g_set_error(error,
              GAUTOGUI_ERROR,
              GAUTOGUI_ERROR_BACKEND_UNAVAILABLE,
              "gautogui does not have a backend for this platform");
  return FALSE;
}

gboolean
_gautogui_backend_move_mouse(GAutoguiBackend *backend,
                             gint x,
                             gint y,
                             GError **error)
{
  (void)backend;
  (void)x;
  (void)y;
  g_set_error(error,
              GAUTOGUI_ERROR,
              GAUTOGUI_ERROR_BACKEND_UNAVAILABLE,
              "gautogui does not have a backend for this platform");
  return FALSE;
}

gboolean
_gautogui_backend_mouse_button(GAutoguiBackend *backend,
                               GAutoguiMouseButton button,
                               gboolean pressed,
                               GError **error)
{
  (void)backend;
  (void)button;
  (void)pressed;
  g_set_error(error,
              GAUTOGUI_ERROR,
              GAUTOGUI_ERROR_BACKEND_UNAVAILABLE,
              "gautogui does not have a backend for this platform");
  return FALSE;
}

gboolean
_gautogui_backend_scroll(GAutoguiBackend *backend,
                         gint dx,
                         gint dy,
                         GError **error)
{
  (void)backend;
  (void)dx;
  (void)dy;
  g_set_error(error,
              GAUTOGUI_ERROR,
              GAUTOGUI_ERROR_BACKEND_UNAVAILABLE,
              "gautogui does not have a backend for this platform");
  return FALSE;
}

gboolean
_gautogui_backend_key(GAutoguiBackend *backend,
                      GAutoguiKey key,
                      gboolean pressed,
                      GError **error)
{
  (void)backend;
  (void)key;
  (void)pressed;
  g_set_error(error,
              GAUTOGUI_ERROR,
              GAUTOGUI_ERROR_BACKEND_UNAVAILABLE,
              "gautogui does not have a backend for this platform");
  return FALSE;
}

gboolean
_gautogui_backend_type_text(GAutoguiBackend *backend,
                            const gchar *utf8,
                            GError **error)
{
  (void)backend;
  (void)utf8;
  g_set_error(error,
              GAUTOGUI_ERROR,
              GAUTOGUI_ERROR_BACKEND_UNAVAILABLE,
              "gautogui does not have a backend for this platform");
  return FALSE;
}
