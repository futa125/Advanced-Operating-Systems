/*
 * shofer.c -- module implementation
 *
 * Example module which creates a virtual device driver.
 * Circular buffer (kfifo) is used to store received data (with write) and
 * reply with them on read operation.
 *
 * Copyright (C) 2021 Leonardo Jelenkovic
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form.
 * No warranty is attached.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/kfifo.h>
#include <linux/log2.h>
#include <linux/delay.h>

#include "config.h"

/* Buffer size */
static int buffer_size = BUFFER_SIZE;
static int max_thread_count = MAX_THREAD_COUNT;

/* Parameter buffer_size can be given at module load time */
module_param(buffer_size, int, S_IRUGO);
MODULE_PARM_DESC(buffer_size, "Buffer size in bytes, must be a power of 2");

module_param(max_thread_count, int, S_IRUGO);
MODULE_PARM_DESC(max_thread_count, "Max number of threads allowed to read or write");

MODULE_AUTHOR(AUTHOR);
MODULE_LICENSE(LICENSE);

struct shofer_dev *Shofer = NULL;
struct buffer *Buffer = NULL;
static dev_t Dev_no = 0;

/* prototypes */
static struct buffer *buffer_create(size_t, int *);
static void buffer_delete(struct buffer *);
static struct shofer_dev *shofer_create(dev_t, struct file_operations *,
	struct buffer *, int *);
static void shofer_delete(struct shofer_dev *);
static void cleanup(void);
static void dump_buffer(struct buffer *);

static int shofer_open(struct inode *, struct file *);
static int shofer_release(struct inode *, struct file *);
static ssize_t shofer_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t shofer_write(struct file *, const char __user *, size_t, loff_t *);

static struct file_operations shofer_fops = {
	.owner =    THIS_MODULE,
	.open =     shofer_open,
	.release =  shofer_release,
	.read =     shofer_read,
	.write =    shofer_write
};

/* init module */
static int __init shofer_module_init(void)
{
	int retval;
	struct buffer *buffer;
	struct shofer_dev *shofer;
	dev_t dev_no = 0;

	printk(KERN_NOTICE "Module 'shofer' started initialization\n");

	/* get device number(s) */
	retval = alloc_chrdev_region(&dev_no, 0, 1, DRIVER_NAME);
	if (retval < 0) {
		printk(KERN_WARNING "%s: can't get major device number %d\n",
			DRIVER_NAME, MAJOR(dev_no));
		return retval;
	}

	/* create a buffer */
	/* buffer size must be a power of 2 */
	if (!is_power_of_2(buffer_size))
		buffer_size = roundup_pow_of_two(buffer_size);
	buffer = buffer_create(buffer_size, &retval);
	if (!buffer)
		goto no_driver;
	Buffer = buffer;

	/* create a device */
	shofer = shofer_create(dev_no, &shofer_fops, buffer, &retval);
	if (!shofer)
		goto no_driver;

	printk(KERN_NOTICE "Module 'shofer' initialized with major=%d, minor=%d\n",
		MAJOR(dev_no), MINOR(dev_no));

	Shofer = shofer;
	Dev_no = dev_no;

	return 0;

no_driver:
	cleanup();

	return retval;
}

static void cleanup(void) {
	if (Shofer)
		shofer_delete(Shofer);
	if (Buffer)
		buffer_delete(Buffer);
	if (Dev_no)
		unregister_chrdev_region(Dev_no, 1);
}

/* called when module exit */
static void __exit shofer_module_exit(void)
{
	printk(KERN_NOTICE "Module 'shofer' started exit operation\n");
	cleanup();
	printk(KERN_NOTICE "Module 'shofer' finished exit operation\n");
}

module_init(shofer_module_init);
module_exit(shofer_module_exit);

/* Create and initialize a single buffer */
static struct buffer *buffer_create(size_t size, int *retval)
{
	struct buffer *buffer = kmalloc(sizeof(struct buffer) + size, GFP_KERNEL);
	if (!buffer) {
		*retval = -ENOMEM;
		printk(KERN_NOTICE "shofer:kmalloc failed\n");
		return NULL;
	}
	*retval = kfifo_init(&buffer->fifo, buffer + 1, size);
	if (*retval) {
		kfree(buffer);
		printk(KERN_NOTICE "shofer:kfifo_init failed\n");
		return NULL;
	}

	*retval = 0;

	return buffer;
}

static void buffer_delete(struct buffer *buffer)
{
	kfree(buffer);
}

/* Create and initialize a single shofer_dev */
static struct shofer_dev *shofer_create(dev_t dev_no,
	struct file_operations *fops, struct buffer *buffer, int *retval)
{
	struct shofer_dev *shofer = kmalloc(sizeof(struct shofer_dev), GFP_KERNEL);
	if (!shofer){
		*retval = -ENOMEM;
		printk(KERN_NOTICE "shofer:kmalloc failed\n");
		return NULL;
	}
	memset(shofer, 0, sizeof(struct shofer_dev));
	shofer->buffer = buffer;
	shofer->threadcnt = 0;

	cdev_init(&shofer->cdev, fops);
	shofer->cdev.owner = THIS_MODULE;
	shofer->cdev.ops = fops;
	*retval = cdev_add (&shofer->cdev, dev_no, 1);
	shofer->dev_no = dev_no;
	if (*retval) {
		printk(KERN_NOTICE "Error (%d) when adding device shofer\n",
			*retval);
		kfree(shofer);
		shofer = NULL;
	}

	mutex_init(&shofer->rlock);
	mutex_init(&shofer->wlock);
	init_waitqueue_head(&shofer->rdwq);
	init_waitqueue_head(&shofer->wrwq);

	return shofer;
}

static void shofer_delete(struct shofer_dev *shofer)
{
	cdev_del(&shofer->cdev);
	kfree(shofer);
}

/* Called when a process calls "open" on this device */
static int shofer_open(struct inode *inode, struct file *filp)
{
	struct shofer_dev *shofer; /* device information */

	shofer = container_of(inode->i_cdev, struct shofer_dev, cdev);
	filp->private_data = shofer; /* for other methods */

	if ( (filp->f_flags & O_ACCMODE) != O_RDONLY && (filp->f_flags & O_ACCMODE) != O_WRONLY )
		return -EPERM;

	if (shofer->threadcnt + 1 > max_thread_count)
	{
		return -1;
	}

	shofer->threadcnt = shofer->threadcnt + 1;

	printk("number of threads %d\n", shofer->threadcnt);

	return 0;
}

/* Called when a process performs "close" operation */
static int shofer_release(struct inode *inode, struct file *filp)
{
	struct shofer_dev *shofer; /* device information */

	shofer = container_of(inode->i_cdev, struct shofer_dev, cdev);
	filp->private_data = shofer; /* for other methods */

	shofer->threadcnt = shofer->threadcnt - 1;

	printk("number of threads %d\n", shofer->threadcnt);

	return 0; /* nothing to do; could not set this function in fops */
}

/* Read count bytes from buffer to user space ubuf */
static ssize_t shofer_read(struct file *filp, char __user *ubuf, size_t count,
	loff_t *f_pos /* ignoring f_pos */)
{
	ssize_t retval = 0;
	struct shofer_dev *shofer = filp->private_data;
	struct buffer *buffer = shofer->buffer;
	struct kfifo *fifo = &buffer->fifo;
	unsigned int copied;

	if (mutex_lock_interruptible(&shofer->rlock))
		return -ERESTARTSYS;

	if (kfifo_is_empty(fifo)) {
		printk("%d: waiting read, buffer is empty\n", current->pid);
		if (wait_event_interruptible(shofer->rdwq, !kfifo_is_empty(fifo)))
			return -ERESTARTSYS;
	}

	printk("%d: thread started reading %ld bytes\n", current->pid, count);

	dump_buffer(buffer);

	retval = kfifo_to_user(fifo, (char __user *) ubuf, count, &copied);
	if (retval)
		printk(KERN_NOTICE "shofer:kfifo_to_user failed\n");
	else
		retval = copied;

	dump_buffer(buffer);

	printk("%d: thread finished reading %ld bytes\n", current->pid, retval);

	wake_up_interruptible(&shofer->wrwq);

	mutex_unlock(&shofer->rlock);

	return retval;
}

/* Write count bytes from user space ubuf to buffer */
static ssize_t shofer_write(struct file *filp, const char __user *ubuf,
	size_t count, loff_t *f_pos /* ignoring f_pos */)
{
	ssize_t retval = 0;
	struct shofer_dev *shofer = filp->private_data;
	struct buffer *buffer = shofer->buffer;
	struct kfifo *fifo = &buffer->fifo;
	unsigned int copied;

	if (mutex_lock_interruptible(&shofer->wlock))
		return -ERESTARTSYS;

	if (count > buffer_size) {
		printk("%d: attempting to write data larger than buffer size, data: %ld > buffer: %d\n", 
				current->pid,
				count, 
				buffer_size);
		mutex_unlock(&shofer->wlock);
		return -1;
	}
	
	if (kfifo_avail(fifo) < count) {
		printk("%d: waiting write, not enough space in buffer, required: %ld, available: %d\n", 
			current->pid,
			count, 
			kfifo_avail(fifo)
		);
		if (wait_event_interruptible(shofer->wrwq, kfifo_avail(fifo) >= count))
			return -ERESTARTSYS;
	}

	printk("%d: thread started writing %ld bytes\n", current->pid, count);

	dump_buffer(buffer);

	retval = kfifo_from_user(fifo, (char __user *) ubuf, count, &copied);
	if (retval)
		printk(KERN_NOTICE "shofer:kfifo_from_user failed\n");
	else
		retval = copied;

	dump_buffer(buffer);

	printk("%d: thread finished writing %ld bytes\n", current->pid, retval);

	wake_up_interruptible(&shofer->rdwq);

	mutex_unlock(&shofer->wlock);

	return retval;
}

static void dump_buffer(struct buffer *b)
{
	char buf[BUFFER_SIZE];
	size_t copied;

	memset(buf, 0, BUFFER_SIZE);
	copied = kfifo_out_peek(&b->fifo, buf, BUFFER_SIZE);

	printk("%d: shofer:buffer:size=%u:contains=%u:buf=%s\n",
		current->pid, kfifo_size(&b->fifo), kfifo_len(&b->fifo), buf);
}
