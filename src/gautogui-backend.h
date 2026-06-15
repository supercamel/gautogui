#ifndef GAUTOGUI_BACKEND_H
#define GAUTOGUI_BACKEND_H

#include "gautogui-controller-private.h"

G_BEGIN_DECLS

typedef struct _GAutoguiBackend GAutoguiBackend;

GAutoguiBackend *_gautogui_backend_new(GAutoguiController *controller,
                                       GError **error);
void _gautogui_backend_free(GAutoguiBackend *backend);

gboolean _gautogui_backend_start(GAutoguiBackend *backend,
                                 GError **error);
void _gautogui_backend_stop(GAutoguiBackend *backend);

gboolean _gautogui_backend_get_mouse_position(GAutoguiBackend *backend,
                                              gint *x,
                                              gint *y,
                                              GError **error);
gboolean _gautogui_backend_move_mouse(GAutoguiBackend *backend,
                                      gint x,
                                      gint y,
                                      GError **error);
gboolean _gautogui_backend_mouse_button(GAutoguiBackend *backend,
                                        GAutoguiMouseButton button,
                                        gboolean pressed,
                                        GError **error);
gboolean _gautogui_backend_scroll(GAutoguiBackend *backend,
                                  gint dx,
                                  gint dy,
                                  GError **error);
gboolean _gautogui_backend_key(GAutoguiBackend *backend,
                               GAutoguiKey key,
                               gboolean pressed,
                               GError **error);
gboolean _gautogui_backend_type_text(GAutoguiBackend *backend,
                                     const gchar *utf8,
                                     guint delay_ms,
                                     GError **error);

G_END_DECLS

#endif /* GAUTOGUI_BACKEND_H */
