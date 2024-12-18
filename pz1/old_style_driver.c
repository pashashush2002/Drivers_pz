#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/ioctl.h>

#define DEVICE_NAME "simple_char_device"
#define IOCTL_RESET_BUFFER _IO('a', 1)
#define IOCTL_SET_SIZE _IOW('a', 2, int)

static int major;
static char *buffer;
static size_t buffer_size = 1024;

static ssize_t device_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offset) {
    if (*offset >= buffer_size) {
        return 0; // EOF
    }

    if (*offset + count > buffer_size) {
        count = buffer_size - *offset; // Limit count to remaining bytes
    }

    if (copy_to_user(user_buffer, buffer + *offset, count)) {
        return -EFAULT;
    }

    *offset += count;
    return count;
}

static ssize_t device_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *offset) {
    if (*offset >= buffer_size) {
        return -ENOSPC; // No space left on device
    }

    if (*offset + count > buffer_size) {
        count = buffer_size - *offset; // Limit count to remaining space
    }

    if (copy_from_user(buffer + *offset, user_buffer, count)) {
        return -EFAULT;
    }

    *offset += count;
    return count;
}

static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
        case IOCTL_RESET_BUFFER:
            memset(buffer, 0, buffer_size);
            break;
        case IOCTL_SET_SIZE:
            if (arg > 0) {
                char *new_buffer = krealloc(buffer, arg, GFP_KERNEL);
                if (!new_buffer) {
                    return -ENOMEM;
                }
                buffer = new_buffer;
                buffer_size = arg;
                memset(buffer, 0, buffer_size); // Reset the new buffer
            }
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .unlocked_ioctl = device_ioctl,
};

static int __init simple_char_device_init(void) {
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        printk(KERN_ALERT "Failed to register character device\n");
        return major;
    }

    buffer = kmalloc(buffer_size, GFP_KERNEL);
    if (!buffer) {
        unregister_chrdev(major, DEVICE_NAME);
        return -ENOMEM;
    }
    
    memset(buffer, 0, buffer_size); // Initialize buffer
    printk(KERN_INFO "Simple char device registered with major number %d\n", major);
    return 0;
}

static void __exit simple_char_device_exit(void) {
    kfree(buffer);
    unregister_chrdev(major, DEVICE_NAME);
    printk(KERN_INFO "Simple char device unregistered\n");
}

module_init(simple_char_device_init);
module_exit(simple_char_device_exit);
MODULE_LICENSE("GPL");

