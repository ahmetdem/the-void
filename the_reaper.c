#include "asm-generic/errno-base.h"
#include "asm/signal.h"
#include "linux/cdev.h"
#include "linux/fs.h"
#include "linux/init.h"
#include "linux/kstrtox.h"
#include "linux/module.h"
#include "linux/printk.h" // for pr_info()
#include "linux/rcupdate.h"
#include "linux/sched.h"
#include "linux/sched/signal.h"
#include "linux/types.h" // for dev_t
#include "linux/uaccess.h"

MODULE_AUTHOR("THE REAPER");
MODULE_DESCRIPTION("THE REAPER EMERGES FROM THE ASHES OF IT'S ENEMIES!!!");
MODULE_LICENSE("GPL");

static dev_t dev = 0;
static struct cdev reaper_cdev;

static char buffer[32];
static int buffer_data_len = 0;

static struct task_struct *t;

static int driver_open(struct inode *inode, struct file *file) {
  pr_info("The Reaper: Device opened");
  return 0;
}

static int driver_close(struct inode *inode, struct file *file) {
  pr_info("The Reaper: Device Closed");
  return 0;
}

static ssize_t driver_read(struct file *file, char __user *user_buffer,
                           size_t count, loff_t *offset) {

  char *msg = buffer;

  if (*offset >= buffer_data_len || buffer_data_len == 0) {
    return 0;
  }

  size_t bytes_to_copy = buffer_data_len - *offset;
  if (bytes_to_copy > count) {
    bytes_to_copy = count;
  }

  if (copy_to_user(user_buffer, msg + *offset, bytes_to_copy)) {
    return -EFAULT;
  }

  *offset += bytes_to_copy;
  return bytes_to_copy;
}

static ssize_t device_write(struct file *file, const char __user *user_buffer,
                            size_t count, loff_t *offset) {

  unsigned long to_copy = 31;
  int usr_pid, ret;

  if (count > sizeof(buffer) - 1) {
    to_copy = sizeof(buffer) - 1;
  } else {
    to_copy = count;
  }

  if (copy_from_user(buffer, user_buffer, to_copy)) {
    return -EFAULT;
  }

  buffer[to_copy] = '\0';
  buffer_data_len = to_copy;

  pr_info("The Reaper received: %s", buffer);

  if (buffer[to_copy - 1] == '\n')
    buffer[to_copy - 1] = '\0';

  ret = kstrtoint(buffer, 10, &usr_pid);
  if (ret < 0)
    goto str_to_int_error;

  rcu_read_lock();
  for_each_process(t) {
    if (t->pid == usr_pid) {
      pr_info("Found the target: %s [PID: %d]\n", t->comm, t->pid);

      ret = send_sig(SIGKILL, t, 1);
      if (ret < 0)
        goto signal_error;
    }
  }
  rcu_read_unlock();

  return count;

str_to_int_error:
  pr_info("Reaper: Could not parse PID (Mostly integer conversion Error). \n");
  return ret;

signal_error:
  pr_info("Error on sending SIGKILL signal.\n");
  return ret;
}

static struct file_operations fops = {.owner = THIS_MODULE,
                                      .open = driver_open,
                                      .release = driver_close,
                                      .read = driver_read,
                                      .write = device_write};

static int __init init_fn(void) {
  pr_info("Hello the reaper.\n");

  int ret = alloc_chrdev_region(&dev, 1, 1, "THE REAPER");
  if (ret < 0)
    return ret;

  cdev_init(&reaper_cdev, &fops);

  ret = cdev_add(&reaper_cdev, dev, 1);
  if (ret < 0)
    goto out_unregister;

  pr_info("Major: %d, Minor: %d\n", MAJOR(dev), MINOR(dev));
  return 0;

out_unregister:
  unregister_chrdev_region(dev, 1);
  return ret;
}

static void __exit exit_fn(void) {
  cdev_del(&reaper_cdev);
  unregister_chrdev_region(dev, 1);

  pr_info("Goodbye the reaper \n");
}

module_init(init_fn);
module_exit(exit_fn);
