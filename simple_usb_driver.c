#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

// usb device identifiers
#define USB_DEVICE_VENDOR_ID  0x0483
#define USB_DEVICE_PRODUCT_ID 0x3748

// variables for usb device read and write 
#define BULK_EP_OUT 0x01
#define BULK_EP_IN 0x82
#define MAX_PCKG_SIZE 512
static unsigned char bulk_buf[MAX_PCKG_SIZE];
static int readcount = 0;
static int value = -1;
// usb device event callbacks, all static
static int usb_open(struct inode*, struct file*);
static int usb_close(struct inode*, struct file*);
static ssize_t usb_write(struct file*, const char*, size_t, loff_t*);
static ssize_t usb_read(struct file*, char*, size_t, loff_t*);
static int usb_device_plugged(struct usb_interface *intf, const struct usb_device_id *id);
static void usb_device_disconnected(struct usb_interface *intf);

// define module init exit functions.
static int __init usb_driver_init(void);
static void __exit usb_driver_exit(void);


// create new usb device and its class driver
static struct usb_device *dev;
static struct usb_class_driver usb_device_class;

// set vendor id and product id for usb recognition. when you plug usb device , will trigger
static struct usb_device_id usb_driver_table[] = { 
	{ USB_DEVICE(USB_DEVICE_VENDOR_ID, USB_DEVICE_PRODUCT_ID) },
	{}
}; 
 
// register module table for this usb device
MODULE_DEVICE_TABLE(usb, usb_driver_table);

// usb serial communication port opened callback function
static int usb_open(struct inode *ind, struct file *f) {
	printk(KERN_INFO "USB: USB SERIAL BUS OPENED.\n");
	return 0 ;
}

// usb serial communication port closed callback function
static int usb_close(struct inode *ind, struct file *f) {
	printk(KERN_INFO "USB: USB SERIAL BUS CLOSED.\n");
	return 0 ;
}

// usb serial communication (write operations) callback function
static ssize_t usb_write(struct file *f, const char *buffer, size_t buffer_len, loff_t *offsetCounter) {
	readcount = 0;
	if (copy_from_user(bulk_buf, buffer, buffer_len)) {
		printk(KERN_ALERT "USB: ERROR READING FROM USB DEVICE, ERR: BAD ADRESS\n");
		
		return -EFAULT;
	}
    //ctr    message   device   send funct    dev    write       buffer    max pck size actual length timeout	
	value = usb_bulk_msg(dev, usb_sndbulkpipe(dev, BULK_EP_OUT), bulk_buf, MAX_PCKG_SIZE, &readcount, 5000);
	if (value < 0) {
		printk(KERN_ALERT "USB: CANT WRITE TO USB DEVICE.\n");
		return value;
	}
	printk(KERN_INFO "USB: WRITING TO USB DEVICE.\n");
	return 0;
	
}

// usb serial communication(read operations) callback function
static ssize_t usb_read(struct file *f, char *buffer, size_t buffer_len, loff_t *offsetCounter) {
	readcount = 0;
	value = usb_bulk_msg(dev, usb_rcvbulkpipe(dev, BULK_EP_IN), bulk_buf, MAX_PCKG_SIZE, &readcount, 5000);

	if (value < 0) {
		printk(KERN_ALERT "USB:CANT READ MESSAGE FROM USB DEVICE.\n");
		return value;
	}
	
	if (copy_to_user(buffer, bulk_buf, buffer_len)) {
		printk(KERN_ALERT "USB: ERROR READING FROM USB DEVICE, ERR: BAD ADRESS\n");
		return -EFAULT;
	}
	printk(KERN_INFO "USB: READING FROM USB DEVICE\n");

	return 0;
}

// file operation function callbacks for device i/o
static struct file_operations file_oprts = { 
	.read = usb_read,		// for reading usb read
	.write = usb_write,		// for writing usb write
	.open = usb_open,		// for opening serial port
	.release = usb_close	// for closing serial port
};

// usb device recognition callback function 
static int usb_device_plugged(struct usb_interface *intf, const struct usb_device_id *id) {
	// if plugged to linux system
	dev = interface_to_usbdev(intf);			// get usb interface arguments
	usb_device_class.name = "/usb/stm32%d";		// set name
	usb_device_class.fops = &file_oprts;		// set file operations
	
	printk(KERN_INFO "USB: OUR USB PLUGGED IN!, Vendor ID: %d, Product ID: %d", id->idVendor, id->idProduct);
	printk(KERN_INFO "USB: OUR USB DEVICE NOW REGISTERING TO DEVICE TABLE");
	
	// if not registered print error
	if ((value = usb_register_dev(intf, &usb_device_class)) < 0) {
		printk(KERN_ALERT "USB: USB DEVICE REGISTER FAILED, TRY RECONNECT.\n");
	} else {
		printk(KERN_INFO "USB:USB DEVICE HAS BEEN REGISTERED SUCCESSFULLY, MINOR NO: %d", intf->minor);
	}

	return value;
}

// usb device disconnect callback --> deregister usb device
static void usb_device_disconnected(struct usb_interface *intf) {
	printk(KERN_INFO "USB: USB DEVICE NOW REMOVING FROM SYSTEM.\n");
	// remove usb interface from kernel 
	usb_deregister_dev(intf, &usb_device_class);

	printk(KERN_INFO "USB: USB DEVICE DISCONNECTED.\n");
	
}

// usb device identifier
static struct usb_driver our_usb_device = {
	.name = "stm32",						// name for our usb
	.id_table = usb_driver_table,			// tables.
	.probe = usb_device_plugged,			// plugged callback
	.disconnect = usb_device_disconnected	// disconnected callback
};

// init function for load usb driver module
static int __init usb_driver_init(void) {
	printk(KERN_INFO "USB: USB DRIVER MODULE EXAMPLE NOW LOADED TO SYSTEM.\n");
	value = usb_register(&our_usb_device); 
	
	// if not registered, exit and return value
	if (value < 0) {
		printk(KERN_ALERT "USB: DEVICE CAN NOT REGISTERED.\n");
		return value;
	}
	// so if we are here,then module registration is success
	printk(KERN_INFO "USB: USB REGISTRATION SUCCESFULL.\n");

	return value;
}

// exit function for close usb driver module
static void __exit usb_driver_exit(void) {
	printk(KERN_INFO "USB: USB DRIVER MODULE WILL QUIT.\n");
	usb_deregister(&our_usb_device);
}


// set functions for usb module
module_init(usb_driver_init);
module_exit(usb_driver_exit);

