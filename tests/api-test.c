#include <gautogui/gautogui.h>

static void
test_controller_lifecycle(void)
{
  g_autoptr(GAutoguiController) controller = NULL;

  controller = gautogui_controller_new();

  g_assert_true(GAUTOGUI_IS_CONTROLLER(controller));
  g_assert_false(gautogui_controller_is_running(controller));

  gautogui_controller_stop(controller);
  g_assert_false(gautogui_controller_is_running(controller));
}

static void
test_controller_context(void)
{
  GMainContext *context = g_main_context_new();
  g_autoptr(GAutoguiController) controller = NULL;

  controller = gautogui_controller_new_for_context(context);

  g_assert_true(GAUTOGUI_IS_CONTROLLER(controller));
  g_main_context_unref(context);
}

static void
test_key_values(void)
{
  g_assert_cmpuint(GAUTOGUI_KEY_A, ==, 'A');
  g_assert_cmpuint(GAUTOGUI_KEY_0, ==, '0');
  g_assert_cmpuint(GAUTOGUI_KEY_SPACE, ==, ' ');
  g_assert_cmpuint(GAUTOGUI_KEY_ENTER, !=, GAUTOGUI_KEY_NONE);
}

int
main(int argc,
     char **argv)
{
  g_test_init(&argc, &argv, NULL);

  g_test_add_func("/gautogui/controller/lifecycle", test_controller_lifecycle);
  g_test_add_func("/gautogui/controller/context", test_controller_context);
  g_test_add_func("/gautogui/key/values", test_key_values);

  return g_test_run();
}
