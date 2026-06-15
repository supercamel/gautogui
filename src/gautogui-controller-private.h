#ifndef GAUTOGUI_CONTROLLER_PRIVATE_H
#define GAUTOGUI_CONTROLLER_PRIVATE_H

#include <gautogui/gautogui-controller.h>

G_BEGIN_DECLS

void _gautogui_controller_emit_mouse_moved(GAutoguiController *self,
                                           gint x,
                                           gint y);
void _gautogui_controller_emit_mouse_button(GAutoguiController *self,
                                            GAutoguiMouseButton button,
                                            gboolean pressed,
                                            gint x,
                                            gint y);
void _gautogui_controller_emit_mouse_scroll(GAutoguiController *self,
                                            gint dx,
                                            gint dy,
                                            gint x,
                                            gint y);
void _gautogui_controller_emit_key(GAutoguiController *self,
                                   guint native_keycode,
                                   GAutoguiKey key,
                                   gboolean pressed);

G_END_DECLS

#endif /* GAUTOGUI_CONTROLLER_PRIVATE_H */
