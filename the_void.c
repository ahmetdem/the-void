#include "asm-generic/errno-base.h"
#include "linux/cdev.h"
#include "linux/export.h"
#include "linux/fs.h"
#include "linux/inet.h"
#include "linux/init.h"
#include "linux/module.h"
#include "linux/printk.h" // for pr_info()
#include "linux/types.h"  // for dev_t
#include "linux/uaccess.h"
#include "void_common.h"

MODULE_AUTHOR("THE VOID");
MODULE_DESCRIPTION("THE VOID EMERGES FROM THE ASHES OF IT'S ENEMIES!!!");
MODULE_LICENSE("GPL");

static dev_t dev = 0;
static struct cdev void_cdev;

static char buffer[256];
static int buffer_data_len = 0;

static __be32 banned_ip = 0;

static int driver_open(struct inode *inode, struct file *file) {
  pr_info("The Void: Device opened");
  return 0;
}

static int driver_close(struct inode *inode, struct file *file) {
  pr_info("The Void: Device Closed");
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

  unsigned long to_copy = 255;

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

  u8 raw_ip[4];

  if (!in4_pton(buffer, count, raw_ip, -1, NULL))
    return -EFAULT;

  memcpy(&banned_ip, raw_ip, 4);

  pr_info("The Void received: %s\n", buffer);
  return count;
}

static struct file_operations fops = {.owner = THIS_MODULE,
                                      .open = driver_open,
                                      .release = driver_close,
                                      .read = driver_read,
                                      .write = device_write};

static int __init init_fn(void) {
  pr_info("Hello the Void.\n");

  int ret = alloc_chrdev_region(&dev, 1, 1, "THE VOID");
  if (ret < 0)
    return ret;

  cdev_init(&void_cdev, &fops);

  ret = cdev_add(&void_cdev, dev, 1);
  if (ret < 0)
    goto out_unregister;

  pr_info("Major: %d, Minor: %d\n", MAJOR(dev), MINOR(dev));
  return 0;

out_unregister:
  unregister_chrdev_region(dev, 1);
  return ret;
}

static void __exit exit_fn(void) {
  cdev_del(&void_cdev);
  unregister_chrdev_region(dev, 1);

  pr_info("Goodbye the Void \n");
}

module_init(init_fn);
module_exit(exit_fn);

bool is_ip_banned(__be32 ip) {
  if (banned_ip == 0)
    return false;
  return (ip == banned_ip);
}

EXPORT_SYMBOL(is_ip_banned);
