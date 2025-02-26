
#include "linux/err.h"
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/device.h>

#define DEVICE_NAME "iocsr"

static int major;
static struct class *iocsr_class;

static ssize_t la64iocsr_read(struct file *file, char __user *buf, size_t count,
				loff_t *ppos) {
	loff_t offset = *ppos;
	size_t mycount = count;
	union {
		uint64_t val;
		uint8_t bytes[8];
	} iocsr;
	
	if (offset % 8) {
		int len = 8 - (offset & 0x7);
		iocsr.val = iocsr_read64(offset & ~0x7);
		memcpy(buf, &iocsr.bytes[offset & 0x7], len);
		mycount -= len;
		offset += len;
		buf += len;
	}

	while (mycount > 8) {
		iocsr.val = iocsr_read64(offset);
		memcpy(buf, &iocsr.bytes, 8);
		mycount -= 8;
		offset += 8;
		buf += 8;
	}

	if (mycount) {
		iocsr.val = iocsr_read64(offset);
		memcpy(buf, &iocsr.bytes, mycount);
		offset += mycount;
	}

	*ppos = offset;
	return mycount;
}

static loff_t la64iocsr_llseek(struct file *file, loff_t offset, int whence) {
	loff_t new_pos = 0;

	switch (whence) {
	case SEEK_SET: // Absolute positioning
		new_pos = offset;
		break;
	case SEEK_CUR: // Relative to current position
		new_pos = file->f_pos + offset;
		break;
	case SEEK_END: // Relative to the end (simulated size)
		new_pos = (~0u) - offset;
		break;
	default:
		return -EINVAL;
	}

	// Ensure new position is within bounds
	if (new_pos < 0 || new_pos > (~0u)) {
		return -EINVAL;
	}

	file->f_pos = new_pos;
	return new_pos;
}


static struct file_operations iocsr_ops = {
	.owner = THIS_MODULE,
	.read = la64iocsr_read,
	.llseek = la64iocsr_llseek,
};

static int __init la64iocsr_init(void) {
	struct device *dev;
	int status;
	
	status = register_chrdev(0, DEVICE_NAME, &iocsr_ops);
	if (status < 0) {
		return status;
	}
	major = status;

	iocsr_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(iocsr_class)) {
		status = PTR_ERR(iocsr_class);
		goto err;
	}

	dev = device_create(iocsr_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
	if (IS_ERR(dev)) {
		status = PTR_ERR(dev);
		goto err;
	}

	return 0;
err:
	unregister_blkdev(major, DEVICE_NAME);
	return status;
}

static void __exit la64iocsr_exit(void) {
	device_destroy(iocsr_class, MKDEV(major, 0));
	class_destroy(iocsr_class);
	unregister_chrdev(0, DEVICE_NAME);
}

module_init(la64iocsr_init);
module_exit(la64iocsr_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("RigoLigo");
