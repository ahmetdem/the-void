#include "asm-generic/errno-base.h"
#include "linux/init.h"
#include "linux/input-event-codes.h"
#include "linux/input.h"
#include "linux/jiffies.h"
#include "linux/module.h"
#include "linux/printk.h" // for pr_info()
#include "linux/timer.h"
#include "linux/timer_types.h"

MODULE_AUTHOR("THE VOID");
MODULE_DESCRIPTION("THE VOID EMERGES FROM THE ASHES OF IT'S ENEMIES!!!");
MODULE_LICENSE("GPL");

static struct input_dev *void_device;
static struct timer_list timer;

static void timer_callback(struct timer_list *t) {
  input_report_key(void_device, KEY_A, 1); // Press KEY A
  input_sync(void_device);
  input_report_key(void_device, KEY_A, 0); // Release KEY A
  input_sync(void_device);

  input_report_key(void_device, KEY_B, 1); // Press KEY B
  input_sync(void_device);
  input_report_key(void_device, KEY_B, 0); // Release KEY B
  input_sync(void_device);

  input_report_key(void_device, KEY_C, 1); // Press KEY C
  input_sync(void_device);
  input_report_key(void_device, KEY_C, 0); // Release KEY C
  input_sync(void_device);

  return;
}

static int __init init_fn(void) {
  pr_info("Hello the void device.\n");

  void_device = input_allocate_device();
  if (!void_device) {
    return -ENOMEM;
  }

  void_device->name = "The Void Keyboard";

  input_set_capability(void_device, EV_KEY, KEY_A);
  input_set_capability(void_device, EV_KEY, KEY_B);
  input_set_capability(void_device, EV_KEY, KEY_C);

  int err = input_register_device(void_device);
  if (err < 0)
    goto regis_fail;

  timer_setup(&timer, timer_callback, 0);
  mod_timer(&timer, jiffies + msecs_to_jiffies(5000));

  return 0;

regis_fail:
  input_free_device(void_device);
  return err;
}

static void __exit exit_fn(void) {
	timer_delete(&timer);
  input_unregister_device(void_device);

  pr_info("Goodbye the Void \n");
}

module_init(init_fn);
module_exit(exit_fn);
