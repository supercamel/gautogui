#include "gautogui-backend.h"

#include <windows.h>

struct _GAutoguiBackend {
  GAutoguiController *controller;
  HHOOK mouse_hook;
  HHOOK keyboard_hook;
  GThread *thread;
  DWORD thread_id;
  GMutex state_lock;
  GCond started_cond;
  gboolean started;
  gboolean start_ok;
  DWORD start_error;
};

static GMutex active_lock;
static GAutoguiBackend *active_backend;
static HINSTANCE gautogui_module_handle;

BOOL WINAPI
DllMain(HINSTANCE instance,
        DWORD reason,
        LPVOID reserved)
{
  (void)reserved;

  if (reason == DLL_PROCESS_ATTACH)
    gautogui_module_handle = instance;

  return TRUE;
}

static HINSTANCE
get_hook_module_handle(void)
{
  if (gautogui_module_handle != NULL)
    return gautogui_module_handle;

  return GetModuleHandleW(NULL);
}

static GAutoguiBackend *
get_active_backend(void)
{
  GAutoguiBackend *backend;

  g_mutex_lock(&active_lock);
  backend = active_backend;
  g_mutex_unlock(&active_lock);

  return backend;
}

static GAutoguiKey
key_from_vk(DWORD vk)
{
  if (vk >= 'A' && vk <= 'Z')
    return (GAutoguiKey)(GAUTOGUI_KEY_A + (vk - 'A'));
  if (vk >= '0' && vk <= '9')
    return (GAutoguiKey)(GAUTOGUI_KEY_0 + (vk - '0'));
  if (vk >= VK_F1 && vk <= VK_F24)
    return (GAutoguiKey)(GAUTOGUI_KEY_F1 + (vk - VK_F1));

  switch (vk) {
  case VK_SPACE:
    return GAUTOGUI_KEY_SPACE;
  case VK_OEM_MINUS:
    return GAUTOGUI_KEY_MINUS;
  case VK_OEM_PLUS:
    return GAUTOGUI_KEY_EQUAL;
  case VK_OEM_4:
    return GAUTOGUI_KEY_LEFT_BRACKET;
  case VK_OEM_6:
    return GAUTOGUI_KEY_RIGHT_BRACKET;
  case VK_OEM_5:
    return GAUTOGUI_KEY_BACKSLASH;
  case VK_OEM_1:
    return GAUTOGUI_KEY_SEMICOLON;
  case VK_OEM_7:
    return GAUTOGUI_KEY_APOSTROPHE;
  case VK_OEM_3:
    return GAUTOGUI_KEY_GRAVE;
  case VK_OEM_COMMA:
    return GAUTOGUI_KEY_COMMA;
  case VK_OEM_PERIOD:
    return GAUTOGUI_KEY_PERIOD;
  case VK_OEM_2:
    return GAUTOGUI_KEY_SLASH;
  case VK_RETURN:
    return GAUTOGUI_KEY_ENTER;
  case VK_TAB:
    return GAUTOGUI_KEY_TAB;
  case VK_ESCAPE:
    return GAUTOGUI_KEY_ESCAPE;
  case VK_BACK:
    return GAUTOGUI_KEY_BACKSPACE;
  case VK_DELETE:
    return GAUTOGUI_KEY_DELETE;
  case VK_INSERT:
    return GAUTOGUI_KEY_INSERT;
  case VK_HOME:
    return GAUTOGUI_KEY_HOME;
  case VK_END:
    return GAUTOGUI_KEY_END;
  case VK_PRIOR:
    return GAUTOGUI_KEY_PAGE_UP;
  case VK_NEXT:
    return GAUTOGUI_KEY_PAGE_DOWN;
  case VK_LEFT:
    return GAUTOGUI_KEY_LEFT;
  case VK_RIGHT:
    return GAUTOGUI_KEY_RIGHT;
  case VK_UP:
    return GAUTOGUI_KEY_UP;
  case VK_DOWN:
    return GAUTOGUI_KEY_DOWN;
  case VK_SHIFT:
  case VK_LSHIFT:
  case VK_RSHIFT:
    return GAUTOGUI_KEY_SHIFT;
  case VK_CONTROL:
  case VK_LCONTROL:
  case VK_RCONTROL:
    return GAUTOGUI_KEY_CONTROL;
  case VK_MENU:
  case VK_LMENU:
  case VK_RMENU:
    return GAUTOGUI_KEY_ALT;
  case VK_LWIN:
  case VK_RWIN:
    return GAUTOGUI_KEY_SUPER;
  case VK_CAPITAL:
    return GAUTOGUI_KEY_CAPS_LOCK;
  default:
    return GAUTOGUI_KEY_NONE;
  }
}

static WORD
vk_from_key(GAutoguiKey key)
{
  if (key >= GAUTOGUI_KEY_A && key <= GAUTOGUI_KEY_Z)
    return (WORD)('A' + (key - GAUTOGUI_KEY_A));
  if (key >= GAUTOGUI_KEY_0 && key <= GAUTOGUI_KEY_9)
    return (WORD)('0' + (key - GAUTOGUI_KEY_0));
  if (key >= GAUTOGUI_KEY_F1 && key <= GAUTOGUI_KEY_F24)
    return (WORD)(VK_F1 + (key - GAUTOGUI_KEY_F1));

  switch (key) {
  case GAUTOGUI_KEY_SPACE:
    return VK_SPACE;
  case GAUTOGUI_KEY_MINUS:
    return VK_OEM_MINUS;
  case GAUTOGUI_KEY_EQUAL:
    return VK_OEM_PLUS;
  case GAUTOGUI_KEY_LEFT_BRACKET:
    return VK_OEM_4;
  case GAUTOGUI_KEY_RIGHT_BRACKET:
    return VK_OEM_6;
  case GAUTOGUI_KEY_BACKSLASH:
    return VK_OEM_5;
  case GAUTOGUI_KEY_SEMICOLON:
    return VK_OEM_1;
  case GAUTOGUI_KEY_APOSTROPHE:
    return VK_OEM_7;
  case GAUTOGUI_KEY_GRAVE:
    return VK_OEM_3;
  case GAUTOGUI_KEY_COMMA:
    return VK_OEM_COMMA;
  case GAUTOGUI_KEY_PERIOD:
    return VK_OEM_PERIOD;
  case GAUTOGUI_KEY_SLASH:
    return VK_OEM_2;
  case GAUTOGUI_KEY_ENTER:
    return VK_RETURN;
  case GAUTOGUI_KEY_TAB:
    return VK_TAB;
  case GAUTOGUI_KEY_ESCAPE:
    return VK_ESCAPE;
  case GAUTOGUI_KEY_BACKSPACE:
    return VK_BACK;
  case GAUTOGUI_KEY_DELETE:
    return VK_DELETE;
  case GAUTOGUI_KEY_INSERT:
    return VK_INSERT;
  case GAUTOGUI_KEY_HOME:
    return VK_HOME;
  case GAUTOGUI_KEY_END:
    return VK_END;
  case GAUTOGUI_KEY_PAGE_UP:
    return VK_PRIOR;
  case GAUTOGUI_KEY_PAGE_DOWN:
    return VK_NEXT;
  case GAUTOGUI_KEY_LEFT:
    return VK_LEFT;
  case GAUTOGUI_KEY_RIGHT:
    return VK_RIGHT;
  case GAUTOGUI_KEY_UP:
    return VK_UP;
  case GAUTOGUI_KEY_DOWN:
    return VK_DOWN;
  case GAUTOGUI_KEY_SHIFT:
    return VK_SHIFT;
  case GAUTOGUI_KEY_CONTROL:
    return VK_CONTROL;
  case GAUTOGUI_KEY_ALT:
    return VK_MENU;
  case GAUTOGUI_KEY_SUPER:
    return VK_LWIN;
  case GAUTOGUI_KEY_CAPS_LOCK:
    return VK_CAPITAL;
  default:
    return 0;
  }
}

static void
emit_mouse_button_from_message(GAutoguiBackend *backend,
                               WPARAM message,
                               const MSLLHOOKSTRUCT *mouse)
{
  GAutoguiMouseButton button = 0;
  gboolean pressed = FALSE;

  switch (message) {
  case WM_LBUTTONDOWN:
    button = GAUTOGUI_MOUSE_BUTTON_LEFT;
    pressed = TRUE;
    break;
  case WM_LBUTTONUP:
    button = GAUTOGUI_MOUSE_BUTTON_LEFT;
    break;
  case WM_MBUTTONDOWN:
    button = GAUTOGUI_MOUSE_BUTTON_MIDDLE;
    pressed = TRUE;
    break;
  case WM_MBUTTONUP:
    button = GAUTOGUI_MOUSE_BUTTON_MIDDLE;
    break;
  case WM_RBUTTONDOWN:
    button = GAUTOGUI_MOUSE_BUTTON_RIGHT;
    pressed = TRUE;
    break;
  case WM_RBUTTONUP:
    button = GAUTOGUI_MOUSE_BUTTON_RIGHT;
    break;
  case WM_XBUTTONDOWN:
    button = HIWORD(mouse->mouseData) == XBUTTON1 ? GAUTOGUI_MOUSE_BUTTON_BACK
                                                  : GAUTOGUI_MOUSE_BUTTON_FORWARD;
    pressed = TRUE;
    break;
  case WM_XBUTTONUP:
    button = HIWORD(mouse->mouseData) == XBUTTON1 ? GAUTOGUI_MOUSE_BUTTON_BACK
                                                  : GAUTOGUI_MOUSE_BUTTON_FORWARD;
    break;
  default:
    break;
  }

  if (button != 0) {
    _gautogui_controller_emit_mouse_button(backend->controller,
                                           button,
                                           pressed,
                                           mouse->pt.x,
                                           mouse->pt.y);
  }
}

static LRESULT CALLBACK
mouse_hook_proc(int code,
                WPARAM wparam,
                LPARAM lparam)
{
  GAutoguiBackend *backend = get_active_backend();
  const MSLLHOOKSTRUCT *mouse = (const MSLLHOOKSTRUCT *)lparam;

  if (code == HC_ACTION && backend != NULL && mouse != NULL) {
    switch (wparam) {
    case WM_MOUSEMOVE:
      _gautogui_controller_emit_mouse_moved(backend->controller, mouse->pt.x, mouse->pt.y);
      break;
    case WM_MOUSEWHEEL: {
      SHORT delta = (SHORT)HIWORD(mouse->mouseData);
      _gautogui_controller_emit_mouse_scroll(backend->controller,
                                             0,
                                             delta > 0 ? -1 : 1,
                                             mouse->pt.x,
                                             mouse->pt.y);
      break;
    }
    case WM_MOUSEHWHEEL: {
      SHORT delta = (SHORT)HIWORD(mouse->mouseData);
      _gautogui_controller_emit_mouse_scroll(backend->controller,
                                             delta > 0 ? 1 : -1,
                                             0,
                                             mouse->pt.x,
                                             mouse->pt.y);
      break;
    }
    default:
      emit_mouse_button_from_message(backend, wparam, mouse);
      break;
    }
  }

  return CallNextHookEx(NULL, code, wparam, lparam);
}

static LRESULT CALLBACK
keyboard_hook_proc(int code,
                   WPARAM wparam,
                   LPARAM lparam)
{
  GAutoguiBackend *backend = get_active_backend();
  const KBDLLHOOKSTRUCT *keyboard = (const KBDLLHOOKSTRUCT *)lparam;

  if (code == HC_ACTION && backend != NULL && keyboard != NULL) {
    gboolean pressed = wparam == WM_KEYDOWN || wparam == WM_SYSKEYDOWN;

    if (pressed || wparam == WM_KEYUP || wparam == WM_SYSKEYUP) {
      _gautogui_controller_emit_key(backend->controller,
                                    keyboard->vkCode,
                                    key_from_vk(keyboard->vkCode),
                                    pressed);
    }
  }

  return CallNextHookEx(NULL, code, wparam, lparam);
}

static gpointer
hook_thread(gpointer user_data)
{
  GAutoguiBackend *backend = user_data;
  MSG msg;

  backend->thread_id = GetCurrentThreadId();
  PeekMessageW(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

  backend->mouse_hook = SetWindowsHookExW(WH_MOUSE_LL,
                                          mouse_hook_proc,
                                          get_hook_module_handle(),
                                          0);
  backend->keyboard_hook = SetWindowsHookExW(WH_KEYBOARD_LL,
                                             keyboard_hook_proc,
                                             get_hook_module_handle(),
                                             0);

  g_mutex_lock(&backend->state_lock);
  backend->start_ok = backend->mouse_hook != NULL && backend->keyboard_hook != NULL;
  backend->start_error = backend->start_ok ? ERROR_SUCCESS : GetLastError();
  backend->started = TRUE;
  g_cond_signal(&backend->started_cond);
  g_mutex_unlock(&backend->state_lock);

  if (!backend->start_ok) {
    if (backend->mouse_hook != NULL) {
      UnhookWindowsHookEx(backend->mouse_hook);
      backend->mouse_hook = NULL;
    }
    if (backend->keyboard_hook != NULL) {
      UnhookWindowsHookEx(backend->keyboard_hook);
      backend->keyboard_hook = NULL;
    }
    return NULL;
  }

  while (GetMessageW(&msg, NULL, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  if (backend->mouse_hook != NULL) {
    UnhookWindowsHookEx(backend->mouse_hook);
    backend->mouse_hook = NULL;
  }
  if (backend->keyboard_hook != NULL) {
    UnhookWindowsHookEx(backend->keyboard_hook);
    backend->keyboard_hook = NULL;
  }

  return NULL;
}

static gboolean
send_input_checked(INPUT *inputs,
                   guint n_inputs,
                   GError **error)
{
  UINT sent = SendInput(n_inputs, inputs, sizeof(INPUT));

  if (sent == n_inputs)
    return TRUE;

  g_set_error(error,
              GAUTOGUI_ERROR,
              GAUTOGUI_ERROR_PLATFORM_ERROR,
              "SendInput failed with Windows error %lu",
              GetLastError());
  return FALSE;
}

GAutoguiBackend *
_gautogui_backend_new(GAutoguiController *controller,
                      GError **error)
{
  GAutoguiBackend *backend = g_new0(GAutoguiBackend, 1);

  (void)error;

  backend->controller = g_object_ref(controller);
  g_mutex_init(&backend->state_lock);
  g_cond_init(&backend->started_cond);

  return backend;
}

void
_gautogui_backend_free(GAutoguiBackend *backend)
{
  if (backend == NULL)
    return;

  _gautogui_backend_stop(backend);
  g_clear_object(&backend->controller);
  g_cond_clear(&backend->started_cond);
  g_mutex_clear(&backend->state_lock);
  g_free(backend);
}

gboolean
_gautogui_backend_start(GAutoguiBackend *backend,
                        GError **error)
{
  gboolean start_ok;
  DWORD start_error;

  if (backend->thread != NULL)
    return TRUE;

  g_mutex_lock(&active_lock);
  if (active_backend != NULL && active_backend != backend) {
    g_mutex_unlock(&active_lock);
    g_set_error(error,
                GAUTOGUI_ERROR,
                GAUTOGUI_ERROR_PLATFORM_ERROR,
                "Only one Windows gautogui hook can be active in this process");
    return FALSE;
  }
  active_backend = backend;
  g_mutex_unlock(&active_lock);

  g_mutex_lock(&backend->state_lock);
  backend->started = FALSE;
  backend->start_ok = FALSE;
  backend->start_error = ERROR_SUCCESS;
  backend->thread = g_thread_new("gautogui-win32-hooks", hook_thread, backend);

  while (!backend->started)
    g_cond_wait(&backend->started_cond, &backend->state_lock);

  start_ok = backend->start_ok;
  start_error = backend->start_error;
  g_mutex_unlock(&backend->state_lock);

  if (!start_ok) {
    if (backend->thread != NULL) {
      g_thread_join(backend->thread);
      backend->thread = NULL;
    }

    g_mutex_lock(&active_lock);
    if (active_backend == backend)
      active_backend = NULL;
    g_mutex_unlock(&active_lock);

    g_set_error(error,
                GAUTOGUI_ERROR,
                GAUTOGUI_ERROR_PLATFORM_ERROR,
                "Unable to install Windows input hooks: error %lu",
                start_error);
    return FALSE;
  }

  return TRUE;
}

void
_gautogui_backend_stop(GAutoguiBackend *backend)
{
  GThread *thread;
  DWORD thread_id;

  if (backend == NULL || backend->thread == NULL)
    return;

  thread = backend->thread;
  thread_id = backend->thread_id;

  if (thread_id != 0)
    PostThreadMessageW(thread_id, WM_QUIT, 0, 0);

  g_thread_join(thread);
  backend->thread = NULL;
  backend->thread_id = 0;

  g_mutex_lock(&active_lock);
  if (active_backend == backend)
    active_backend = NULL;
  g_mutex_unlock(&active_lock);
}

gboolean
_gautogui_backend_get_mouse_position(GAutoguiBackend *backend,
                                     gint *x,
                                     gint *y,
                                     GError **error)
{
  POINT point;

  (void)backend;

  if (!GetCursorPos(&point)) {
    g_set_error(error,
                GAUTOGUI_ERROR,
                GAUTOGUI_ERROR_PLATFORM_ERROR,
                "GetCursorPos failed with Windows error %lu",
                GetLastError());
    return FALSE;
  }

  if (x != NULL)
    *x = point.x;
  if (y != NULL)
    *y = point.y;

  return TRUE;
}

gboolean
_gautogui_backend_move_mouse(GAutoguiBackend *backend,
                             gint x,
                             gint y,
                             GError **error)
{
  (void)backend;

  if (SetCursorPos(x, y))
    return TRUE;

  g_set_error(error,
              GAUTOGUI_ERROR,
              GAUTOGUI_ERROR_PLATFORM_ERROR,
              "SetCursorPos failed with Windows error %lu",
              GetLastError());
  return FALSE;
}

gboolean
_gautogui_backend_mouse_button(GAutoguiBackend *backend,
                               GAutoguiMouseButton button,
                               gboolean pressed,
                               GError **error)
{
  INPUT input = { 0 };

  (void)backend;

  input.type = INPUT_MOUSE;

  switch (button) {
  case GAUTOGUI_MOUSE_BUTTON_LEFT:
    input.mi.dwFlags = pressed ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
    break;
  case GAUTOGUI_MOUSE_BUTTON_MIDDLE:
    input.mi.dwFlags = pressed ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
    break;
  case GAUTOGUI_MOUSE_BUTTON_RIGHT:
    input.mi.dwFlags = pressed ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
    break;
  case GAUTOGUI_MOUSE_BUTTON_BACK:
    input.mi.dwFlags = pressed ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
    input.mi.mouseData = XBUTTON1;
    break;
  case GAUTOGUI_MOUSE_BUTTON_FORWARD:
    input.mi.dwFlags = pressed ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
    input.mi.mouseData = XBUTTON2;
    break;
  default:
    g_set_error(error,
                GAUTOGUI_ERROR,
                GAUTOGUI_ERROR_INVALID_ARGUMENT,
                "Unsupported mouse button %u",
                button);
    return FALSE;
  }

  return send_input_checked(&input, 1, error);
}

gboolean
_gautogui_backend_scroll(GAutoguiBackend *backend,
                         gint dx,
                         gint dy,
                         GError **error)
{
  INPUT inputs[2] = { 0 };
  guint n_inputs = 0;

  (void)backend;

  if (dy != 0) {
    inputs[n_inputs].type = INPUT_MOUSE;
    inputs[n_inputs].mi.dwFlags = MOUSEEVENTF_WHEEL;
    inputs[n_inputs].mi.mouseData = (DWORD)(-dy * WHEEL_DELTA);
    n_inputs++;
  }

  if (dx != 0) {
    inputs[n_inputs].type = INPUT_MOUSE;
    inputs[n_inputs].mi.dwFlags = MOUSEEVENTF_HWHEEL;
    inputs[n_inputs].mi.mouseData = (DWORD)(dx * WHEEL_DELTA);
    n_inputs++;
  }

  if (n_inputs == 0)
    return TRUE;

  return send_input_checked(inputs, n_inputs, error);
}

gboolean
_gautogui_backend_key(GAutoguiBackend *backend,
                      GAutoguiKey key,
                      gboolean pressed,
                      GError **error)
{
  INPUT input = { 0 };
  WORD vk = vk_from_key(key);

  (void)backend;

  if (vk == 0) {
    g_set_error(error,
                GAUTOGUI_ERROR,
                GAUTOGUI_ERROR_INVALID_ARGUMENT,
                "Unsupported key %u",
                key);
    return FALSE;
  }

  input.type = INPUT_KEYBOARD;
  input.ki.wVk = vk;
  input.ki.dwFlags = pressed ? 0 : KEYEVENTF_KEYUP;

  return send_input_checked(&input, 1, error);
}

gboolean
_gautogui_backend_type_text(GAutoguiBackend *backend,
                            const gchar *utf8,
                            GError **error)
{
  gunichar2 *utf16;
  glong n_units = 0;

  (void)backend;

  utf16 = g_utf8_to_utf16(utf8, -1, NULL, &n_units, error);
  if (utf16 == NULL)
    return FALSE;

  for (glong i = 0; i < n_units; i++) {
    INPUT inputs[2] = { 0 };

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wScan = utf16[i];
    inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wScan = utf16[i];
    inputs[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

    if (!send_input_checked(inputs, G_N_ELEMENTS(inputs), error)) {
      g_free(utf16);
      return FALSE;
    }
  }

  g_free(utf16);
  return TRUE;
}
