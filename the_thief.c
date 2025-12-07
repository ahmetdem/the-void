#include "linux/gfp_types.h"
#include "linux/init.h"
#include "linux/input.h"
#include "linux/mod_devicetable.h"
#include "linux/module.h"
#include "linux/printk.h"
#include "linux/slab.h"
#include "linux/usb.h"
#include "linux/usb/input.h"

MODULE_AUTHOR("The Thief");
MODULE_DESCRIPTION("Drivers for my Areson Technology Corp 2.4G Wireless Mouse");
MODULE_LICENSE("GPL");

struct usb_fd_mouse {
  char name[128];

  struct usb_device *fd_udev;
  struct input_dev *fd_indev;
  struct urb *fd_irq;

  signed char *fd_data;
  dma_addr_t fd_data_dma;
};

static const struct usb_device_id fd_table[] = {{USB_DEVICE(0x25a7, 0xfa10)},
                                                {/* Terminating Entry */}};
MODULE_DEVICE_TABLE(usb, fd_table);

static void fd_set_name(struct usb_fd_mouse *fd_mouse) {
  if (fd_mouse->fd_udev->manufacturer) {
    strscpy(fd_mouse->name, fd_mouse->fd_udev->manufacturer,
            sizeof(fd_mouse->name));
  }

  if (fd_mouse->fd_udev->product) {
    if (fd_mouse->fd_udev->manufacturer) {
      strlcat(fd_mouse->name, " ", sizeof(fd_mouse->name));
    }

    strlcat(fd_mouse->name, fd_mouse->fd_udev->product, sizeof(fd_mouse->name));
  }

  if (!strlen(fd_mouse->name))
    snprintf(fd_mouse->name, sizeof(fd_mouse->name),
             "USB HIDBP Mouse %04x:%04x",
             le16_to_cpu(fd_mouse->fd_udev->descriptor.idVendor),
             le16_to_cpu(fd_mouse->fd_udev->descriptor.idProduct));
}

// Runs Every Time Mouse Moves
static void fd_irq(struct urb *urb) {
  struct usb_fd_mouse *fd_mouse = urb->context;
  int status;

  switch (urb->status) {
  case 0: // success
    pr_info("Raw: %02x %02x %02x %02x | %02x %02x %02x %02x\n",
            fd_mouse->fd_data[0], fd_mouse->fd_data[1], fd_mouse->fd_data[2],
            fd_mouse->fd_data[3], fd_mouse->fd_data[4], fd_mouse->fd_data[5],
            fd_mouse->fd_data[6], fd_mouse->fd_data[7]);
    break;
  case -ECONNRESET: /* unlink */
  case -ENOENT:
  case -ESHUTDOWN:
    return;
  default:
    goto resubmit;
  }

  // THESE ARE THE CONFIGS THAT MAKE THE MOUSE MOVE
  input_report_key(fd_mouse->fd_indev, BTN_LEFT, fd_mouse->fd_data[1] & 0x01);
  input_report_key(fd_mouse->fd_indev, BTN_RIGHT, fd_mouse->fd_data[1] & 0x02);
  input_report_key(fd_mouse->fd_indev, BTN_MIDDLE, fd_mouse->fd_data[1] & 0x04);
  input_report_key(fd_mouse->fd_indev, BTN_SIDE, fd_mouse->fd_data[1] & 0x08);
  input_report_key(fd_mouse->fd_indev, BTN_EXTRA, fd_mouse->fd_data[1] & 0x10);

  input_report_rel(fd_mouse->fd_indev, REL_X, fd_mouse->fd_data[2]);
  input_report_rel(fd_mouse->fd_indev, REL_Y, fd_mouse->fd_data[3]);

  input_report_rel(fd_mouse->fd_indev, REL_WHEEL, fd_mouse->fd_data[6]);

  input_sync(fd_mouse->fd_indev);

resubmit:
  status = usb_submit_urb(urb, GFP_ATOMIC);
  if (status)
    dev_err(&fd_mouse->fd_udev->dev,
            "can't resubmit intr, %s-%s/input0, status %d\n",
            fd_mouse->fd_udev->bus->bus_name, fd_mouse->fd_udev->devpath,
            status);
}

static int fd_mouse_open(struct input_dev *dev) {
  struct usb_fd_mouse *mouse = input_get_drvdata(dev);

  mouse->fd_irq->dev = mouse->fd_udev;
  if (usb_submit_urb(mouse->fd_irq, GFP_KERNEL))
    return -EIO;

  return 0;
}

static void fd_mouse_close(struct input_dev *dev) {
  struct usb_fd_mouse *mouse = input_get_drvdata(dev);

  usb_kill_urb(mouse->fd_irq);
}

static int fd_probe(struct usb_interface *intf,
                    const struct usb_device_id *id) {
  pr_info("Mouse Thief: Probe called! Stealing device %04x:%04x\n",
          id->idVendor, id->idProduct);
  int err = -ENOMEM;

  struct usb_fd_mouse *fd_mouse =
      kzalloc(sizeof(struct usb_fd_mouse), GFP_KERNEL);
  fd_mouse->fd_indev = input_allocate_device();
  if (!fd_mouse || !fd_mouse->fd_indev) {
    goto fd_data_fail;
  }

  if (intf->cur_altsetting->desc.bNumEndpoints != 1)
    return -ENODEV;

  struct usb_endpoint_descriptor *endpoint =
      &intf->cur_altsetting->endpoint[0].desc;
  if (!usb_endpoint_is_int_in(endpoint))
    return -ENODEV;

  fd_mouse->fd_udev = interface_to_usbdev(intf);
  fd_mouse->fd_data = usb_alloc_coherent(fd_mouse->fd_udev, 8, GFP_KERNEL,
                                         &fd_mouse->fd_data_dma);

  int pipe = usb_rcvintpipe(fd_mouse->fd_udev, endpoint->bEndpointAddress);
  int maxpacket = usb_maxpacket(fd_mouse->fd_udev, pipe);

  if (!fd_mouse->fd_data) {
    goto fd_data_fail;
  }

  fd_mouse->fd_irq = usb_alloc_urb(0, GFP_KERNEL);
  if (!fd_mouse->fd_irq) {
    goto fd_urb_fail;
  }

  fd_set_name(fd_mouse);

  fd_mouse->fd_indev->name = fd_mouse->name;
  usb_to_input_id(fd_mouse->fd_udev, &fd_mouse->fd_indev->id);
  fd_mouse->fd_indev->dev.parent = &intf->dev;

  fd_mouse->fd_indev->open = fd_mouse_open;
  fd_mouse->fd_indev->close = fd_mouse_close;

  usb_fill_int_urb(fd_mouse->fd_irq, fd_mouse->fd_udev, pipe, fd_mouse->fd_data,
                   (maxpacket > 8 ? 8 : maxpacket), fd_irq, fd_mouse,
                   endpoint->bInterval);

  fd_mouse->fd_indev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);
  fd_mouse->fd_indev->keybit[BIT_WORD(BTN_MOUSE)] =
      BIT_MASK(BTN_LEFT) | BIT_MASK(BTN_RIGHT) | BIT_MASK(BTN_MIDDLE);
  fd_mouse->fd_indev->relbit[0] = BIT_MASK(REL_X) | BIT_MASK(REL_Y);
  fd_mouse->fd_indev->keybit[BIT_WORD(BTN_MOUSE)] |=
      BIT_MASK(BTN_SIDE) | BIT_MASK(BTN_EXTRA);
  fd_mouse->fd_indev->relbit[0] |= BIT_MASK(REL_WHEEL);

  fd_mouse->fd_irq->transfer_dma = fd_mouse->fd_data_dma;
  fd_mouse->fd_irq->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

  input_set_drvdata(fd_mouse->fd_indev, fd_mouse);

  err = input_register_device(fd_mouse->fd_indev);
  usb_set_intfdata(intf, fd_mouse);

  return 0;

fd_data_fail:
  kfree(fd_mouse);
  return err;

fd_urb_fail:
  usb_free_coherent(fd_mouse->fd_udev, 8, fd_mouse->fd_data,
                    fd_mouse->fd_data_dma);
  return err;
}

static void fd_disconnect(struct usb_interface *intf) {
  pr_info("Mouse Thief: Disconnect called! Returning device to OS.\n");

  struct usb_fd_mouse *fd_mouse = usb_get_intfdata(intf);

  if (!fd_mouse)
    return;

  usb_kill_urb(fd_mouse->fd_irq);
  input_unregister_device(fd_mouse->fd_indev);
  usb_free_urb(fd_mouse->fd_irq);
  usb_free_coherent(fd_mouse->fd_udev, 8, fd_mouse->fd_data,
                    fd_mouse->fd_data_dma);
  kfree(fd_mouse);
}

static struct usb_driver fd_driver = {.name = "fd-i920",
                                      .probe = fd_probe,
                                      .disconnect = fd_disconnect,
                                      .id_table = fd_table,
                                      .disable_hub_initiated_lpm = 1};

static int __init init_fn(void) {
  pr_info("Driver Initilized.\n");

  int result = usb_register(&fd_driver);
  if (result < 0) {
    pr_err("usb_register failed for the " __FILE__ "driver."
           "Error number %d",
           result);

    return -1;
  }

  return 0;
}

static void __exit exit_fn(void) {
  pr_info("Driver Exited.\n");
  usb_deregister(&fd_driver);
}

module_init(init_fn);
module_exit(exit_fn);
