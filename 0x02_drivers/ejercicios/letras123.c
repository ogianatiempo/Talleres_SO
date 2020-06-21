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
#include <linux/semaphore.h>

static struct cdev letras123_device;
static dev_t major;
static struct class *letras123_class;

static bool espacio_asignado[3] = {false, false, false};
static char espacios[3] = {'\0', '\0', '\0'};

static struct semaphore letras123_mutex;

static ssize_t letras123_read(struct file *filp, char __user *data, size_t s, loff_t *off) {
	if (filp->private_data == NULL) {
		// Si no habia hecho open o el open fallo, private_data es NULL
		return -EACCES;
	};

	int indice = (unsigned int) filp->private_data - 1;

	if (espacios[indice] == '\0') {
		// No se escribio
		printk(KERN_ALERT "No se habia escrito en espacio %d\n", indice);
		return -EACCES;;
	};

	char *res = kmalloc(s+1, GFP_KERNEL);
	size_t i;

	for (i = 0; i < s; i++) {
		res[i] = espacios[indice];
	};

	res[s] = '\0';	
	
	if(copy_to_user(data, res, s+1) != 0){
		printk(KERN_ALERT "error copy_to_user\n");
	};

	kfree(res);

	return s;
}

static ssize_t letras123_write(struct file *filp, const char __user *data, size_t s, loff_t *off) {
	if (filp->private_data == NULL) {
		// Si no habia hecho open o el open fallo, private_data es NULL
		return -EACCES;
	};
	int indice = (unsigned int) filp->private_data - 1;
	printk(KERN_ALERT "Escribiendo espacio %d\n", indice);
	if (espacios[indice] != '\0') {
		// Ya escribio
		printk(KERN_ALERT "Ya se habia escrito en espacio %d\n", indice);
		return 0;
	};

	printk(KERN_ALERT "copiando %c en espacio %d\n", data[0], indice);
	copy_from_user(&espacios[indice], data, 1);

	return s;
}

static int letras123_open(struct inode *inode, struct file *f) {
	int i;
	down(&letras123_mutex);
		for (i = 0; i < 3; i++) {
			if (!espacio_asignado[i]) {
				// Si hay lugar, asigno un espacio y guardo el indice en private_data
				printk(KERN_ALERT "Asignado espacio %d\n", i);
				espacio_asignado[i] = true;
				f->private_data = i + 1;
				up(&letras123_mutex);
				return 0;
			};
		};
	up(&letras123_mutex);
	printk(KERN_ALERT "No hay espacio disponible\n");
	return -EACCES;
};

static int letras123_close(struct inode *inode, struct file *f) {
	if (f->private_data == NULL) {
		// Si no habia hecho open o el open fallo, private_data es NULL
		return -EACCES;
	};
	int indice = (unsigned int) f->private_data - 1;
	printk(KERN_ALERT "Liberando espacio %d\n", indice);
	down(&letras123_mutex);
		// Libero el espacio y reseteo contenido
		espacio_asignado[indice] = false;
		espacios[indice] = '\0';
	up(&letras123_mutex);
	printk(KERN_ALERT "Liberado espacio %d\n", indice);
	return 0;
};

static struct file_operations letras123_operaciones = {
  .owner = THIS_MODULE,
  .read = letras123_read,
  .write = letras123_write,
  .open = letras123_open,
  .release = letras123_close
};

static int __init letras123_init(void) {
	cdev_init(&letras123_device, &letras123_operaciones);
	alloc_chrdev_region(&major, 0, 1, "letras123");
	cdev_add(&letras123_device, major, 1);

	letras123_class = class_create(THIS_MODULE, "letras123");
	device_create(letras123_class, NULL, major, NULL, "letras123");

	sema_init(&letras123_mutex, 1);
	printk(KERN_ALERT "Cargando modulo 'letras123'\n");
	return 0;
}

static void __exit letras123_exit(void) {
  device_destroy(letras123_class, major);
  class_destroy(letras123_class);

  unregister_chrdev_region(major, 1);
  cdev_del(&letras123_device);

	printk(KERN_ALERT "Descargando modulo 'letras123'.\n");
}

module_init(letras123_init);
module_exit(letras123_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("La banda de SO");
MODULE_DESCRIPTION("Una suerte de '/dev/letras123'");

