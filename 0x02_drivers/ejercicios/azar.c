#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/errno.h>
#include <linux/gfp.h>

static struct cdev azar_device;
static dev_t major;
static struct class *azar_class;

static unsigned int n = 0;

static ssize_t azar_read(struct file *filp, char __user *data, size_t s, loff_t *off) {
	if (n == 0) {
		return -EPERM;
	}

	unsigned int rnd;
	get_random_bytes(&rnd, sizeof(int));
	rnd %= n;
	// El nro más grande tiene 10 chars y sumamos \n.
	char text[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	snprintf(text, 11, "%d\n", rnd);
	if(copy_to_user(data, text, strlen(text)) != 0){
		printk(KERN_ALERT "error copy_to_user\n");
	}
	printk(KERN_ALERT "rnd %d end\n",rnd);
	return strlen(text);
}

static ssize_t azar_write(struct file *filp, const char __user *data, size_t s, loff_t *off) {
	char* kdata = kmalloc(s+1, GFP_KERNEL);

	copy_from_user(kdata, data, s+1);
	// Los datos que leemos del usuario pueden no terminar en \0
	// lo cual haria fallar a kstrtoint. Por eso lo agregamos:
	kdata[s] = '\0';

	unsigned int retval;

	if (kdata[0] == '-') {
		printk(KERN_ALERT "%s\n", kdata);
		printk(KERN_ALERT "No aceptamos negativos -\n");
		kfree(kdata);
		return -EPERM;
	} else if ((retval = kstrtoint(kdata, 10,&n)) != 0) {
		kfree(kdata);
		return -EPERM;
	}
	kfree(kdata);
	printk(KERN_ALERT "n: %d end\n", n);
	return s;
}

static struct file_operations azar_operaciones = {
  .owner = THIS_MODULE,
  .read = azar_read,
  .write = azar_write,
};

static int __init azar_init(void) {
  cdev_init(&azar_device, &azar_operaciones);
  alloc_chrdev_region(&major, 0, 1, "azar");
  cdev_add(&azar_device, major, 1);

  azar_class = class_create(THIS_MODULE, "azar");
  device_create(azar_class, NULL, major, NULL, "azar");

	printk(KERN_ALERT "Cargando modulo 'azar'\n");
	return 0;
}

static void __exit azar_exit(void) {
  device_destroy(azar_class, major);
  class_destroy(azar_class);

  unregister_chrdev_region(major, 1);
  cdev_del(&azar_device);

	printk(KERN_ALERT "Descargando modulo 'azar'.\n");
}

module_init(azar_init);
module_exit(azar_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("La banda de SO");
MODULE_DESCRIPTION("Una suerte de '/dev/azar'");

