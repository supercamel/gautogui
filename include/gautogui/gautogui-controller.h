#ifndef GAUTOGUI_CONTROLLER_H
#define GAUTOGUI_CONTROLLER_H

#include <glib-object.h>

G_BEGIN_DECLS

#if defined(_WIN32) && !defined(GAUTOGUI_STATIC_COMPILATION)
# if defined(GAUTOGUI_COMPILATION)
#  define GAUTOGUI_API __declspec(dllexport)
# else
#  define GAUTOGUI_API __declspec(dllimport)
# endif
#else
# define GAUTOGUI_API
#endif

/**
 * GAutoguiError:
 * @GAUTOGUI_ERROR_BACKEND_UNAVAILABLE: No usable platform backend is available.
 * @GAUTOGUI_ERROR_PERMISSION_DENIED: The operating system denied access.
 * @GAUTOGUI_ERROR_INVALID_ARGUMENT: The caller supplied an invalid argument.
 * @GAUTOGUI_ERROR_PLATFORM_ERROR: The platform API reported an unexpected error.
 *
 * Error codes returned by gautogui.
 */
typedef enum {
  GAUTOGUI_ERROR_BACKEND_UNAVAILABLE,
  GAUTOGUI_ERROR_PERMISSION_DENIED,
  GAUTOGUI_ERROR_INVALID_ARGUMENT,
  GAUTOGUI_ERROR_PLATFORM_ERROR,
} GAutoguiError;

#define GAUTOGUI_ERROR (gautogui_error_quark())
GAUTOGUI_API GQuark gautogui_error_quark(void);

/**
 * GAutoguiMouseButton:
 * @GAUTOGUI_MOUSE_BUTTON_LEFT: Primary pointer button.
 * @GAUTOGUI_MOUSE_BUTTON_MIDDLE: Middle pointer button.
 * @GAUTOGUI_MOUSE_BUTTON_RIGHT: Secondary pointer button.
 * @GAUTOGUI_MOUSE_BUTTON_BACK: Browser/system back side button.
 * @GAUTOGUI_MOUSE_BUTTON_FORWARD: Browser/system forward side button.
 *
 * Cross-platform logical mouse buttons.
 */
typedef enum {
  GAUTOGUI_MOUSE_BUTTON_LEFT = 1,
  GAUTOGUI_MOUSE_BUTTON_MIDDLE = 2,
  GAUTOGUI_MOUSE_BUTTON_RIGHT = 3,
  GAUTOGUI_MOUSE_BUTTON_BACK = 4,
  GAUTOGUI_MOUSE_BUTTON_FORWARD = 5,
} GAutoguiMouseButton;

/**
 * GAutoguiKey:
 * @GAUTOGUI_KEY_NONE: Unknown or unmapped key.
 *
 * Cross-platform logical keys. Printable alphanumeric keys intentionally use
 * their ASCII values so applications can compare against familiar constants.
 * Use gautogui_controller_type_text() for text input that requires layout-aware
 * modifiers or Unicode characters.
 */
typedef enum {
  GAUTOGUI_KEY_NONE = 0,

  GAUTOGUI_KEY_A = 'A',
  GAUTOGUI_KEY_B = 'B',
  GAUTOGUI_KEY_C = 'C',
  GAUTOGUI_KEY_D = 'D',
  GAUTOGUI_KEY_E = 'E',
  GAUTOGUI_KEY_F = 'F',
  GAUTOGUI_KEY_G = 'G',
  GAUTOGUI_KEY_H = 'H',
  GAUTOGUI_KEY_I = 'I',
  GAUTOGUI_KEY_J = 'J',
  GAUTOGUI_KEY_K = 'K',
  GAUTOGUI_KEY_L = 'L',
  GAUTOGUI_KEY_M = 'M',
  GAUTOGUI_KEY_N = 'N',
  GAUTOGUI_KEY_O = 'O',
  GAUTOGUI_KEY_P = 'P',
  GAUTOGUI_KEY_Q = 'Q',
  GAUTOGUI_KEY_R = 'R',
  GAUTOGUI_KEY_S = 'S',
  GAUTOGUI_KEY_T = 'T',
  GAUTOGUI_KEY_U = 'U',
  GAUTOGUI_KEY_V = 'V',
  GAUTOGUI_KEY_W = 'W',
  GAUTOGUI_KEY_X = 'X',
  GAUTOGUI_KEY_Y = 'Y',
  GAUTOGUI_KEY_Z = 'Z',

  GAUTOGUI_KEY_0 = '0',
  GAUTOGUI_KEY_1 = '1',
  GAUTOGUI_KEY_2 = '2',
  GAUTOGUI_KEY_3 = '3',
  GAUTOGUI_KEY_4 = '4',
  GAUTOGUI_KEY_5 = '5',
  GAUTOGUI_KEY_6 = '6',
  GAUTOGUI_KEY_7 = '7',
  GAUTOGUI_KEY_8 = '8',
  GAUTOGUI_KEY_9 = '9',

  GAUTOGUI_KEY_SPACE = ' ',
  GAUTOGUI_KEY_MINUS = '-',
  GAUTOGUI_KEY_EQUAL = '=',
  GAUTOGUI_KEY_LEFT_BRACKET = '[',
  GAUTOGUI_KEY_RIGHT_BRACKET = ']',
  GAUTOGUI_KEY_BACKSLASH = '\\',
  GAUTOGUI_KEY_SEMICOLON = ';',
  GAUTOGUI_KEY_APOSTROPHE = 39,
  GAUTOGUI_KEY_GRAVE = '`',
  GAUTOGUI_KEY_COMMA = ',',
  GAUTOGUI_KEY_PERIOD = '.',
  GAUTOGUI_KEY_SLASH = '/',

  GAUTOGUI_KEY_ENTER = 0x1000,
  GAUTOGUI_KEY_TAB,
  GAUTOGUI_KEY_ESCAPE,
  GAUTOGUI_KEY_BACKSPACE,
  GAUTOGUI_KEY_DELETE,
  GAUTOGUI_KEY_INSERT,
  GAUTOGUI_KEY_HOME,
  GAUTOGUI_KEY_END,
  GAUTOGUI_KEY_PAGE_UP,
  GAUTOGUI_KEY_PAGE_DOWN,
  GAUTOGUI_KEY_LEFT,
  GAUTOGUI_KEY_RIGHT,
  GAUTOGUI_KEY_UP,
  GAUTOGUI_KEY_DOWN,

  GAUTOGUI_KEY_SHIFT = 0x1100,
  GAUTOGUI_KEY_CONTROL,
  GAUTOGUI_KEY_ALT,
  GAUTOGUI_KEY_SUPER,
  GAUTOGUI_KEY_CAPS_LOCK,

  GAUTOGUI_KEY_F1 = 0x1200,
  GAUTOGUI_KEY_F2,
  GAUTOGUI_KEY_F3,
  GAUTOGUI_KEY_F4,
  GAUTOGUI_KEY_F5,
  GAUTOGUI_KEY_F6,
  GAUTOGUI_KEY_F7,
  GAUTOGUI_KEY_F8,
  GAUTOGUI_KEY_F9,
  GAUTOGUI_KEY_F10,
  GAUTOGUI_KEY_F11,
  GAUTOGUI_KEY_F12,
  GAUTOGUI_KEY_F13,
  GAUTOGUI_KEY_F14,
  GAUTOGUI_KEY_F15,
  GAUTOGUI_KEY_F16,
  GAUTOGUI_KEY_F17,
  GAUTOGUI_KEY_F18,
  GAUTOGUI_KEY_F19,
  GAUTOGUI_KEY_F20,
  GAUTOGUI_KEY_F21,
  GAUTOGUI_KEY_F22,
  GAUTOGUI_KEY_F23,
  GAUTOGUI_KEY_F24,
} GAutoguiKey;

#define GAUTOGUI_TYPE_CONTROLLER (gautogui_controller_get_type())
GAUTOGUI_API G_DECLARE_FINAL_TYPE(GAutoguiController,
                                   gautogui_controller,
                                   GAUTOGUI,
                                   CONTROLLER,
                                   GObject)

GAUTOGUI_API GAutoguiController *gautogui_controller_new(void);
GAUTOGUI_API GAutoguiController *gautogui_controller_new_for_context(GMainContext *context);

GAUTOGUI_API gboolean gautogui_controller_start(GAutoguiController *self,
                                                GError **error);
GAUTOGUI_API void gautogui_controller_stop(GAutoguiController *self);
GAUTOGUI_API gboolean gautogui_controller_is_running(GAutoguiController *self);

/**
 * gautogui_controller_get_mouse_position:
 * @self: a #GAutoguiController
 * @x: (out) (optional): return location for the screen X coordinate
 * @y: (out) (optional): return location for the screen Y coordinate
 * @error: return location for a #GError, or %NULL
 *
 * Reads the current system pointer position in screen coordinates.
 *
 * Returns: %TRUE on success, %FALSE with @error set on failure
 */
GAUTOGUI_API gboolean gautogui_controller_get_mouse_position(GAutoguiController *self,
                                                             gint *x,
                                                             gint *y,
                                                             GError **error);
GAUTOGUI_API gboolean gautogui_controller_move_mouse(GAutoguiController *self,
                                                     gint x,
                                                     gint y,
                                                     GError **error);
GAUTOGUI_API gboolean gautogui_controller_mouse_down(GAutoguiController *self,
                                                     GAutoguiMouseButton button,
                                                     GError **error);
GAUTOGUI_API gboolean gautogui_controller_mouse_up(GAutoguiController *self,
                                                   GAutoguiMouseButton button,
                                                   GError **error);
GAUTOGUI_API gboolean gautogui_controller_click(GAutoguiController *self,
                                                GAutoguiMouseButton button,
                                                GError **error);
GAUTOGUI_API gboolean gautogui_controller_scroll(GAutoguiController *self,
                                                 gint dx,
                                                 gint dy,
                                                 GError **error);

GAUTOGUI_API gboolean gautogui_controller_key_down(GAutoguiController *self,
                                                   GAutoguiKey key,
                                                   GError **error);
GAUTOGUI_API gboolean gautogui_controller_key_up(GAutoguiController *self,
                                                 GAutoguiKey key,
                                                 GError **error);
GAUTOGUI_API gboolean gautogui_controller_press_key(GAutoguiController *self,
                                                    GAutoguiKey key,
                                                    GError **error);

/**
 * gautogui_controller_type_text:
 * @self: a #GAutoguiController
 * @utf8: valid UTF-8 text to type
 * @error: return location for a #GError, or %NULL
 *
 * Types @utf8 using the backend's default inter-character delay.
 *
 * Returns: %TRUE on success, %FALSE with @error set on failure
 */
GAUTOGUI_API gboolean gautogui_controller_type_text(GAutoguiController *self,
                                                    const gchar *utf8,
                                                    GError **error);

/**
 * gautogui_controller_type_text_with_delay:
 * @self: a #GAutoguiController
 * @utf8: valid UTF-8 text to type
 * @delay_ms: delay in milliseconds between typed characters
 * @error: return location for a #GError, or %NULL
 *
 * Types @utf8 with an explicit delay between characters. Use this when the
 * target application or operating system drops keystrokes if input arrives too
 * quickly. Pass 0 to request no inter-character delay.
 *
 * Returns: %TRUE on success, %FALSE with @error set on failure
 */
GAUTOGUI_API gboolean gautogui_controller_type_text_with_delay(GAutoguiController *self,
                                                               const gchar *utf8,
                                                               guint delay_ms,
                                                               GError **error);

G_END_DECLS

#endif /* GAUTOGUI_CONTROLLER_H */
