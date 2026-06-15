#include "gautogui-backend.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>

struct _GAutoguiBackend {
  GAutoguiController *controller;
  Display *display;
  gint xi_opcode;
  gint wake_pipe[2];
  GThread *thread;
  gint running;
};

typedef struct {
  KeySym keysym;
  gboolean shift;
} TextKey;

static gboolean
query_pointer_locked(GAutoguiBackend *backend,
                     gint *x,
                     gint *y)
{
  Window root_return = None;
  Window child_return = None;
  gint root_x = 0;
  gint root_y = 0;
  gint win_x = 0;
  gint win_y = 0;
  guint mask = 0;

  if (!XQueryPointer(backend->display,
                     DefaultRootWindow(backend->display),
                     &root_return,
                     &child_return,
                     &root_x,
                     &root_y,
                     &win_x,
                     &win_y,
                     &mask))
    return FALSE;

  if (x != NULL)
    *x = root_x;
  if (y != NULL)
    *y = root_y;

  return TRUE;
}

static GAutoguiMouseButton
mouse_button_from_x11(guint button)
{
  switch (button) {
  case 1:
    return GAUTOGUI_MOUSE_BUTTON_LEFT;
  case 2:
    return GAUTOGUI_MOUSE_BUTTON_MIDDLE;
  case 3:
    return GAUTOGUI_MOUSE_BUTTON_RIGHT;
  case 8:
    return GAUTOGUI_MOUSE_BUTTON_BACK;
  case 9:
    return GAUTOGUI_MOUSE_BUTTON_FORWARD;
  default:
    return 0;
  }
}

static guint
x11_button_from_mouse_button(GAutoguiMouseButton button)
{
  switch (button) {
  case GAUTOGUI_MOUSE_BUTTON_LEFT:
    return 1;
  case GAUTOGUI_MOUSE_BUTTON_MIDDLE:
    return 2;
  case GAUTOGUI_MOUSE_BUTTON_RIGHT:
    return 3;
  case GAUTOGUI_MOUSE_BUTTON_BACK:
    return 8;
  case GAUTOGUI_MOUSE_BUTTON_FORWARD:
    return 9;
  default:
    return 0;
  }
}

static GAutoguiKey
key_from_keysym(KeySym keysym)
{
  if (keysym >= XK_a && keysym <= XK_z)
    return (GAutoguiKey)(GAUTOGUI_KEY_A + (keysym - XK_a));
  if (keysym >= XK_A && keysym <= XK_Z)
    return (GAutoguiKey)(GAUTOGUI_KEY_A + (keysym - XK_A));
  if (keysym >= XK_0 && keysym <= XK_9)
    return (GAutoguiKey)(GAUTOGUI_KEY_0 + (keysym - XK_0));
  if (keysym >= XK_F1 && keysym <= XK_F24)
    return (GAutoguiKey)(GAUTOGUI_KEY_F1 + (keysym - XK_F1));

  switch (keysym) {
  case XK_space:
    return GAUTOGUI_KEY_SPACE;
  case XK_minus:
    return GAUTOGUI_KEY_MINUS;
  case XK_equal:
    return GAUTOGUI_KEY_EQUAL;
  case XK_bracketleft:
    return GAUTOGUI_KEY_LEFT_BRACKET;
  case XK_bracketright:
    return GAUTOGUI_KEY_RIGHT_BRACKET;
  case XK_backslash:
    return GAUTOGUI_KEY_BACKSLASH;
  case XK_semicolon:
    return GAUTOGUI_KEY_SEMICOLON;
  case XK_apostrophe:
    return GAUTOGUI_KEY_APOSTROPHE;
  case XK_grave:
    return GAUTOGUI_KEY_GRAVE;
  case XK_comma:
    return GAUTOGUI_KEY_COMMA;
  case XK_period:
    return GAUTOGUI_KEY_PERIOD;
  case XK_slash:
    return GAUTOGUI_KEY_SLASH;
  case XK_Return:
  case XK_KP_Enter:
    return GAUTOGUI_KEY_ENTER;
  case XK_Tab:
    return GAUTOGUI_KEY_TAB;
  case XK_Escape:
    return GAUTOGUI_KEY_ESCAPE;
  case XK_BackSpace:
    return GAUTOGUI_KEY_BACKSPACE;
  case XK_Delete:
  case XK_KP_Delete:
    return GAUTOGUI_KEY_DELETE;
  case XK_Insert:
  case XK_KP_Insert:
    return GAUTOGUI_KEY_INSERT;
  case XK_Home:
  case XK_KP_Home:
    return GAUTOGUI_KEY_HOME;
  case XK_End:
  case XK_KP_End:
    return GAUTOGUI_KEY_END;
  case XK_Page_Up:
  case XK_KP_Page_Up:
    return GAUTOGUI_KEY_PAGE_UP;
  case XK_Page_Down:
  case XK_KP_Page_Down:
    return GAUTOGUI_KEY_PAGE_DOWN;
  case XK_Left:
  case XK_KP_Left:
    return GAUTOGUI_KEY_LEFT;
  case XK_Right:
  case XK_KP_Right:
    return GAUTOGUI_KEY_RIGHT;
  case XK_Up:
  case XK_KP_Up:
    return GAUTOGUI_KEY_UP;
  case XK_Down:
  case XK_KP_Down:
    return GAUTOGUI_KEY_DOWN;
  case XK_Shift_L:
  case XK_Shift_R:
    return GAUTOGUI_KEY_SHIFT;
  case XK_Control_L:
  case XK_Control_R:
    return GAUTOGUI_KEY_CONTROL;
  case XK_Alt_L:
  case XK_Alt_R:
    return GAUTOGUI_KEY_ALT;
  case XK_Super_L:
  case XK_Super_R:
  case XK_Meta_L:
  case XK_Meta_R:
    return GAUTOGUI_KEY_SUPER;
  case XK_Caps_Lock:
    return GAUTOGUI_KEY_CAPS_LOCK;
  default:
    return GAUTOGUI_KEY_NONE;
  }
}

static KeySym
keysym_from_key(GAutoguiKey key)
{
  if (key >= GAUTOGUI_KEY_A && key <= GAUTOGUI_KEY_Z)
    return XK_a + (key - GAUTOGUI_KEY_A);
  if (key >= GAUTOGUI_KEY_0 && key <= GAUTOGUI_KEY_9)
    return XK_0 + (key - GAUTOGUI_KEY_0);
  if (key >= GAUTOGUI_KEY_F1 && key <= GAUTOGUI_KEY_F24)
    return XK_F1 + (key - GAUTOGUI_KEY_F1);

  switch (key) {
  case GAUTOGUI_KEY_SPACE:
    return XK_space;
  case GAUTOGUI_KEY_MINUS:
    return XK_minus;
  case GAUTOGUI_KEY_EQUAL:
    return XK_equal;
  case GAUTOGUI_KEY_LEFT_BRACKET:
    return XK_bracketleft;
  case GAUTOGUI_KEY_RIGHT_BRACKET:
    return XK_bracketright;
  case GAUTOGUI_KEY_BACKSLASH:
    return XK_backslash;
  case GAUTOGUI_KEY_SEMICOLON:
    return XK_semicolon;
  case GAUTOGUI_KEY_APOSTROPHE:
    return XK_apostrophe;
  case GAUTOGUI_KEY_GRAVE:
    return XK_grave;
  case GAUTOGUI_KEY_COMMA:
    return XK_comma;
  case GAUTOGUI_KEY_PERIOD:
    return XK_period;
  case GAUTOGUI_KEY_SLASH:
    return XK_slash;
  case GAUTOGUI_KEY_ENTER:
    return XK_Return;
  case GAUTOGUI_KEY_TAB:
    return XK_Tab;
  case GAUTOGUI_KEY_ESCAPE:
    return XK_Escape;
  case GAUTOGUI_KEY_BACKSPACE:
    return XK_BackSpace;
  case GAUTOGUI_KEY_DELETE:
    return XK_Delete;
  case GAUTOGUI_KEY_INSERT:
    return XK_Insert;
  case GAUTOGUI_KEY_HOME:
    return XK_Home;
  case GAUTOGUI_KEY_END:
    return XK_End;
  case GAUTOGUI_KEY_PAGE_UP:
    return XK_Page_Up;
  case GAUTOGUI_KEY_PAGE_DOWN:
    return XK_Page_Down;
  case GAUTOGUI_KEY_LEFT:
    return XK_Left;
  case GAUTOGUI_KEY_RIGHT:
    return XK_Right;
  case GAUTOGUI_KEY_UP:
    return XK_Up;
  case GAUTOGUI_KEY_DOWN:
    return XK_Down;
  case GAUTOGUI_KEY_SHIFT:
    return XK_Shift_L;
  case GAUTOGUI_KEY_CONTROL:
    return XK_Control_L;
  case GAUTOGUI_KEY_ALT:
    return XK_Alt_L;
  case GAUTOGUI_KEY_SUPER:
    return XK_Super_L;
  case GAUTOGUI_KEY_CAPS_LOCK:
    return XK_Caps_Lock;
  default:
    return NoSymbol;
  }
}

static gboolean
lookup_text_key(gunichar ch,
                TextKey *text_key)
{
  text_key->keysym = NoSymbol;
  text_key->shift = FALSE;

  if (ch >= 'a' && ch <= 'z') {
    text_key->keysym = XK_a + (ch - 'a');
    return TRUE;
  }

  if (ch >= 'A' && ch <= 'Z') {
    text_key->keysym = XK_a + (ch - 'A');
    text_key->shift = TRUE;
    return TRUE;
  }

  if (ch >= '0' && ch <= '9') {
    text_key->keysym = XK_0 + (ch - '0');
    return TRUE;
  }

  switch (ch) {
  case ' ':
    text_key->keysym = XK_space;
    return TRUE;
  case '\n':
    text_key->keysym = XK_Return;
    return TRUE;
  case '\t':
    text_key->keysym = XK_Tab;
    return TRUE;
  case '-':
    text_key->keysym = XK_minus;
    return TRUE;
  case '_':
    text_key->keysym = XK_minus;
    text_key->shift = TRUE;
    return TRUE;
  case '=':
    text_key->keysym = XK_equal;
    return TRUE;
  case '+':
    text_key->keysym = XK_equal;
    text_key->shift = TRUE;
    return TRUE;
  case '[':
    text_key->keysym = XK_bracketleft;
    return TRUE;
  case '{':
    text_key->keysym = XK_bracketleft;
    text_key->shift = TRUE;
    return TRUE;
  case ']':
    text_key->keysym = XK_bracketright;
    return TRUE;
  case '}':
    text_key->keysym = XK_bracketright;
    text_key->shift = TRUE;
    return TRUE;
  case '\\':
    text_key->keysym = XK_backslash;
    return TRUE;
  case '|':
    text_key->keysym = XK_backslash;
    text_key->shift = TRUE;
    return TRUE;
  case ';':
    text_key->keysym = XK_semicolon;
    return TRUE;
  case ':':
    text_key->keysym = XK_semicolon;
    text_key->shift = TRUE;
    return TRUE;
  case '\'':
    text_key->keysym = XK_apostrophe;
    return TRUE;
  case '"':
    text_key->keysym = XK_apostrophe;
    text_key->shift = TRUE;
    return TRUE;
  case '`':
    text_key->keysym = XK_grave;
    return TRUE;
  case '~':
    text_key->keysym = XK_grave;
    text_key->shift = TRUE;
    return TRUE;
  case ',':
    text_key->keysym = XK_comma;
    return TRUE;
  case '<':
    text_key->keysym = XK_comma;
    text_key->shift = TRUE;
    return TRUE;
  case '.':
    text_key->keysym = XK_period;
    return TRUE;
  case '>':
    text_key->keysym = XK_period;
    text_key->shift = TRUE;
    return TRUE;
  case '/':
    text_key->keysym = XK_slash;
    return TRUE;
  case '?':
    text_key->keysym = XK_slash;
    text_key->shift = TRUE;
    return TRUE;
  case '!':
    text_key->keysym = XK_1;
    text_key->shift = TRUE;
    return TRUE;
  case '@':
    text_key->keysym = XK_2;
    text_key->shift = TRUE;
    return TRUE;
  case '#':
    text_key->keysym = XK_3;
    text_key->shift = TRUE;
    return TRUE;
  case '$':
    text_key->keysym = XK_4;
    text_key->shift = TRUE;
    return TRUE;
  case '%':
    text_key->keysym = XK_5;
    text_key->shift = TRUE;
    return TRUE;
  case '^':
    text_key->keysym = XK_6;
    text_key->shift = TRUE;
    return TRUE;
  case '&':
    text_key->keysym = XK_7;
    text_key->shift = TRUE;
    return TRUE;
  case '*':
    text_key->keysym = XK_8;
    text_key->shift = TRUE;
    return TRUE;
  case '(':
    text_key->keysym = XK_9;
    text_key->shift = TRUE;
    return TRUE;
  case ')':
    text_key->keysym = XK_0;
    text_key->shift = TRUE;
    return TRUE;
  default:
    return FALSE;
  }
}

static void
drain_wake_pipe(GAutoguiBackend *backend)
{
  guint8 buf[64];

  while (read(backend->wake_pipe[0], buf, sizeof buf) > 0) {
  }
}

static void
handle_raw_event(GAutoguiBackend *backend,
                 XIRawEvent *raw,
                 gint evtype)
{
  gint x = 0;
  gint y = 0;

  query_pointer_locked(backend, &x, &y);

  switch (evtype) {
  case XI_RawMotion:
    _gautogui_controller_emit_mouse_moved(backend->controller, x, y);
    break;
  case XI_RawButtonPress:
    switch (raw->detail) {
    case 4:
      _gautogui_controller_emit_mouse_scroll(backend->controller, 0, -1, x, y);
      break;
    case 5:
      _gautogui_controller_emit_mouse_scroll(backend->controller, 0, 1, x, y);
      break;
    case 6:
      _gautogui_controller_emit_mouse_scroll(backend->controller, -1, 0, x, y);
      break;
    case 7:
      _gautogui_controller_emit_mouse_scroll(backend->controller, 1, 0, x, y);
      break;
    default: {
      GAutoguiMouseButton button = mouse_button_from_x11(raw->detail);
      if (button != 0)
        _gautogui_controller_emit_mouse_button(backend->controller, button, TRUE, x, y);
      break;
    }
    }
    break;
  case XI_RawButtonRelease: {
    GAutoguiMouseButton button = mouse_button_from_x11(raw->detail);
    if (button != 0)
      _gautogui_controller_emit_mouse_button(backend->controller, button, FALSE, x, y);
    break;
  }
  case XI_RawKeyPress:
  case XI_RawKeyRelease: {
    KeySym keysym = XkbKeycodeToKeysym(backend->display, (KeyCode)raw->detail, 0, 0);
    GAutoguiKey key = key_from_keysym(keysym);

    _gautogui_controller_emit_key(backend->controller,
                                  raw->detail,
                                  key,
                                  evtype == XI_RawKeyPress);
    break;
  }
  default:
    break;
  }
}

static void
process_pending_x11_events(GAutoguiBackend *backend)
{
  XLockDisplay(backend->display);
  while (XPending(backend->display) > 0) {
    XEvent event;

    XNextEvent(backend->display, &event);

    if (event.xcookie.type != GenericEvent || event.xcookie.extension != backend->xi_opcode)
      continue;

    if (XGetEventData(backend->display, &event.xcookie)) {
      if (event.xcookie.evtype == XI_RawMotion ||
          event.xcookie.evtype == XI_RawButtonPress ||
          event.xcookie.evtype == XI_RawButtonRelease ||
          event.xcookie.evtype == XI_RawKeyPress ||
          event.xcookie.evtype == XI_RawKeyRelease) {
        handle_raw_event(backend, event.xcookie.data, event.xcookie.evtype);
      }

      XFreeEventData(backend->display, &event.xcookie);
    }
  }
  XUnlockDisplay(backend->display);
}

static gpointer
x11_event_thread(gpointer user_data)
{
  GAutoguiBackend *backend = user_data;
  GPollFD poll_fds[2];

  poll_fds[0].fd = ConnectionNumber(backend->display);
  poll_fds[0].events = G_IO_IN | G_IO_HUP | G_IO_ERR;
  poll_fds[1].fd = backend->wake_pipe[0];
  poll_fds[1].events = G_IO_IN | G_IO_HUP | G_IO_ERR;

  while (g_atomic_int_get(&backend->running)) {
    poll_fds[0].revents = 0;
    poll_fds[1].revents = 0;

    if (g_poll(poll_fds, G_N_ELEMENTS(poll_fds), -1) < 0) {
      if (errno == EINTR)
        continue;
      break;
    }

    if (poll_fds[1].revents != 0) {
      drain_wake_pipe(backend);
      if (!g_atomic_int_get(&backend->running))
        break;
    }

    if (poll_fds[0].revents != 0)
      process_pending_x11_events(backend);
  }

  return NULL;
}

static gboolean
set_nonblocking(gint fd)
{
  gint flags = fcntl(fd, F_GETFL, 0);

  if (flags < 0)
    return FALSE;

  return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

static gboolean
force_x11_backend(void)
{
  const gchar *force = g_getenv("GAUTOGUI_FORCE_X11");

  return force != NULL && force[0] != '\0' && g_strcmp0(force, "0") != 0;
}

GAutoguiBackend *
_gautogui_backend_new(GAutoguiController *controller,
                      GError **error)
{
  GAutoguiBackend *backend;
  Display *display;
  gint event_base = 0;
  gint error_base = 0;
  gint major = 2;
  gint minor = 0;

  if (!force_x11_backend() &&
      (g_strcmp0(g_getenv("XDG_SESSION_TYPE"), "wayland") == 0 ||
       (g_getenv("DISPLAY") == NULL && g_getenv("WAYLAND_DISPLAY") != NULL))) {
    g_set_error(error,
                GAUTOGUI_ERROR,
                GAUTOGUI_ERROR_BACKEND_UNAVAILABLE,
                "Wayland/XWayland does not provide a generic client API for true system-wide input hooks or injection; run under X11/Xorg or set GAUTOGUI_FORCE_X11=1 for XWayland-limited testing");
    return NULL;
  }

  if (!XInitThreads()) {
    g_set_error(error,
                GAUTOGUI_ERROR,
                GAUTOGUI_ERROR_PLATFORM_ERROR,
                "Xlib thread support is unavailable");
    return NULL;
  }

  display = XOpenDisplay(NULL);
  if (display == NULL) {
    g_set_error(error,
                GAUTOGUI_ERROR,
                GAUTOGUI_ERROR_BACKEND_UNAVAILABLE,
                "Unable to open X11 display");
    return NULL;
  }

  backend = g_new0(GAutoguiBackend, 1);
  backend->controller = g_object_ref(controller);
  backend->display = display;
  backend->wake_pipe[0] = -1;
  backend->wake_pipe[1] = -1;

  if (!XQueryExtension(display, "XInputExtension", &backend->xi_opcode, &event_base, &error_base) ||
      XIQueryVersion(display, &major, &minor) != Success) {
    g_set_error(error,
                GAUTOGUI_ERROR,
                GAUTOGUI_ERROR_BACKEND_UNAVAILABLE,
                "XInput2 is required for global raw input events");
    _gautogui_backend_free(backend);
    return NULL;
  }

  if (!XTestQueryExtension(display, &event_base, &error_base, &major, &minor)) {
    g_set_error(error,
                GAUTOGUI_ERROR,
                GAUTOGUI_ERROR_BACKEND_UNAVAILABLE,
                "XTest is required for input injection");
    _gautogui_backend_free(backend);
    return NULL;
  }

  if (pipe(backend->wake_pipe) != 0 ||
      !set_nonblocking(backend->wake_pipe[0]) ||
      !set_nonblocking(backend->wake_pipe[1])) {
    g_set_error(error,
                GAUTOGUI_ERROR,
                GAUTOGUI_ERROR_PLATFORM_ERROR,
                "Unable to create backend wake pipe: %s",
                g_strerror(errno));
    _gautogui_backend_free(backend);
    return NULL;
  }

  return backend;
}

void
_gautogui_backend_free(GAutoguiBackend *backend)
{
  if (backend == NULL)
    return;

  _gautogui_backend_stop(backend);

  if (backend->wake_pipe[0] >= 0)
    close(backend->wake_pipe[0]);
  if (backend->wake_pipe[1] >= 0)
    close(backend->wake_pipe[1]);
  if (backend->display != NULL)
    XCloseDisplay(backend->display);

  g_clear_object(&backend->controller);
  g_free(backend);
}

gboolean
_gautogui_backend_start(GAutoguiBackend *backend,
                        GError **error)
{
  Window root;
  XIEventMask mask;
  unsigned char raw_mask[(XI_LASTEVENT + 7) / 8] = { 0 };

  if (g_atomic_int_get(&backend->running))
    return TRUE;

  root = DefaultRootWindow(backend->display);

  XISetMask(raw_mask, XI_RawMotion);
  XISetMask(raw_mask, XI_RawButtonPress);
  XISetMask(raw_mask, XI_RawButtonRelease);
  XISetMask(raw_mask, XI_RawKeyPress);
  XISetMask(raw_mask, XI_RawKeyRelease);

  mask.deviceid = XIAllMasterDevices;
  mask.mask_len = sizeof raw_mask;
  mask.mask = raw_mask;

  XLockDisplay(backend->display);
  XISelectEvents(backend->display, root, &mask, 1);
  XSync(backend->display, False);
  XUnlockDisplay(backend->display);

  g_atomic_int_set(&backend->running, TRUE);
  backend->thread = g_thread_new("gautogui-x11-events", x11_event_thread, backend);
  if (backend->thread == NULL) {
    g_atomic_int_set(&backend->running, FALSE);
    g_set_error(error,
                GAUTOGUI_ERROR,
                GAUTOGUI_ERROR_PLATFORM_ERROR,
                "Unable to start X11 event thread");
    return FALSE;
  }

  return TRUE;
}

void
_gautogui_backend_stop(GAutoguiBackend *backend)
{
  guint8 wake = 1;

  if (backend == NULL || !g_atomic_int_get(&backend->running))
    return;

  g_atomic_int_set(&backend->running, FALSE);
  if (backend->wake_pipe[1] >= 0)
    (void)write(backend->wake_pipe[1], &wake, sizeof wake);

  if (backend->thread != NULL) {
    g_thread_join(backend->thread);
    backend->thread = NULL;
  }
}

gboolean
_gautogui_backend_get_mouse_position(GAutoguiBackend *backend,
                                     gint *x,
                                     gint *y,
                                     GError **error)
{
  gboolean ok;

  XLockDisplay(backend->display);
  ok = query_pointer_locked(backend, x, y);
  XUnlockDisplay(backend->display);

  if (!ok) {
    g_set_error(error,
                GAUTOGUI_ERROR,
                GAUTOGUI_ERROR_PLATFORM_ERROR,
                "Unable to query X11 pointer position");
  }

  return ok;
}

gboolean
_gautogui_backend_move_mouse(GAutoguiBackend *backend,
                             gint x,
                             gint y,
                             GError **error)
{
  gboolean ok;

  XLockDisplay(backend->display);
  ok = XTestFakeMotionEvent(backend->display, DefaultScreen(backend->display), x, y, CurrentTime);
  XFlush(backend->display);
  XUnlockDisplay(backend->display);

  if (!ok) {
    g_set_error(error,
                GAUTOGUI_ERROR,
                GAUTOGUI_ERROR_PLATFORM_ERROR,
                "Unable to move X11 pointer");
  }

  return ok;
}

gboolean
_gautogui_backend_mouse_button(GAutoguiBackend *backend,
                               GAutoguiMouseButton button,
                               gboolean pressed,
                               GError **error)
{
  guint x11_button = x11_button_from_mouse_button(button);
  gboolean ok;

  if (x11_button == 0) {
    g_set_error(error,
                GAUTOGUI_ERROR,
                GAUTOGUI_ERROR_INVALID_ARGUMENT,
                "Unsupported mouse button %u",
                button);
    return FALSE;
  }

  XLockDisplay(backend->display);
  ok = XTestFakeButtonEvent(backend->display, x11_button, pressed, CurrentTime);
  XFlush(backend->display);
  XUnlockDisplay(backend->display);

  if (!ok) {
    g_set_error(error,
                GAUTOGUI_ERROR,
                GAUTOGUI_ERROR_PLATFORM_ERROR,
                "Unable to inject X11 mouse button event");
  }

  return ok;
}

gboolean
_gautogui_backend_scroll(GAutoguiBackend *backend,
                         gint dx,
                         gint dy,
                         GError **error)
{
  gboolean ok = TRUE;

  XLockDisplay(backend->display);

  while (dy < 0 && ok) {
    ok = XTestFakeButtonEvent(backend->display, 4, True, CurrentTime) &&
         XTestFakeButtonEvent(backend->display, 4, False, CurrentTime);
    dy++;
  }
  while (dy > 0 && ok) {
    ok = XTestFakeButtonEvent(backend->display, 5, True, CurrentTime) &&
         XTestFakeButtonEvent(backend->display, 5, False, CurrentTime);
    dy--;
  }
  while (dx < 0 && ok) {
    ok = XTestFakeButtonEvent(backend->display, 6, True, CurrentTime) &&
         XTestFakeButtonEvent(backend->display, 6, False, CurrentTime);
    dx++;
  }
  while (dx > 0 && ok) {
    ok = XTestFakeButtonEvent(backend->display, 7, True, CurrentTime) &&
         XTestFakeButtonEvent(backend->display, 7, False, CurrentTime);
    dx--;
  }

  XFlush(backend->display);
  XUnlockDisplay(backend->display);

  if (!ok) {
    g_set_error(error,
                GAUTOGUI_ERROR,
                GAUTOGUI_ERROR_PLATFORM_ERROR,
                "Unable to inject X11 scroll event");
  }

  return ok;
}

gboolean
_gautogui_backend_key(GAutoguiBackend *backend,
                      GAutoguiKey key,
                      gboolean pressed,
                      GError **error)
{
  KeySym keysym = keysym_from_key(key);
  KeyCode keycode;
  gboolean ok;

  if (keysym == NoSymbol) {
    g_set_error(error,
                GAUTOGUI_ERROR,
                GAUTOGUI_ERROR_INVALID_ARGUMENT,
                "Unsupported key %u",
                key);
    return FALSE;
  }

  XLockDisplay(backend->display);
  keycode = XKeysymToKeycode(backend->display, keysym);
  if (keycode == 0) {
    XUnlockDisplay(backend->display);
    g_set_error(error,
                GAUTOGUI_ERROR,
                GAUTOGUI_ERROR_PLATFORM_ERROR,
                "No X11 keycode is mapped for key %u",
                key);
    return FALSE;
  }

  ok = XTestFakeKeyEvent(backend->display, keycode, pressed, CurrentTime);
  XFlush(backend->display);
  XUnlockDisplay(backend->display);

  if (!ok) {
    g_set_error(error,
                GAUTOGUI_ERROR,
                GAUTOGUI_ERROR_PLATFORM_ERROR,
                "Unable to inject X11 key event");
  }

  return ok;
}

gboolean
_gautogui_backend_type_text(GAutoguiBackend *backend,
                            const gchar *utf8,
                            GError **error)
{
  KeyCode shift_keycode;

  XLockDisplay(backend->display);
  shift_keycode = XKeysymToKeycode(backend->display, XK_Shift_L);

  for (const gchar *p = utf8; *p != '\0'; p = g_utf8_next_char(p)) {
    TextKey text_key;
    KeyCode keycode;

    if (!lookup_text_key(g_utf8_get_char(p), &text_key)) {
      XUnlockDisplay(backend->display);
      g_set_error(error,
                  GAUTOGUI_ERROR,
                  GAUTOGUI_ERROR_INVALID_ARGUMENT,
                  "The X11 backend currently supports ASCII text injection only");
      return FALSE;
    }

    keycode = XKeysymToKeycode(backend->display, text_key.keysym);
    if (keycode == 0 || (text_key.shift && shift_keycode == 0)) {
      XUnlockDisplay(backend->display);
      g_set_error(error,
                  GAUTOGUI_ERROR,
                  GAUTOGUI_ERROR_PLATFORM_ERROR,
                  "No X11 keycode is mapped for a requested text character");
      return FALSE;
    }

    if (text_key.shift)
      XTestFakeKeyEvent(backend->display, shift_keycode, True, CurrentTime);
    XTestFakeKeyEvent(backend->display, keycode, True, CurrentTime);
    XTestFakeKeyEvent(backend->display, keycode, False, CurrentTime);
    if (text_key.shift)
      XTestFakeKeyEvent(backend->display, shift_keycode, False, CurrentTime);
  }

  XFlush(backend->display);
  XUnlockDisplay(backend->display);

  return TRUE;
}
