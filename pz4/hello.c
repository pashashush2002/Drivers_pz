#include <linux/module.h>  
#include <linux/kernel.h>  
#include <linux/init.h>  
#include <linux/timer.h>  
#include <linux/fs.h>  
#include <linux/uaccess.h>  
#include <linux/kobject.h>  
#include <linux/slab.h>
#include <linux/device.h> 
 
#define DEVICE_NAME "my_timer_device"  
#define CLASS_NAME "my_timer_class"  
 
static int majorNumber;  
static struct class* myClass = NULL;  
static struct device* myDevice = NULL;  
 
static struct timer_list myTimer;  
static int global_variable = 0;  
static bool timer_running = false;  
 
static ssize_t read_value(struct kobject *kobj, struct kobj_attribute *attr, char *buf); 
static ssize_t reset_value(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count); 
static ssize_t start_timer(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count); 
static ssize_t stop_timer(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count); 
 
static struct kobj_attribute value_attribute = __ATTR(value, 0664, read_value, reset_value); 
static struct kobj_attribute start_attribute = __ATTR(start, 0220, NULL, start_timer); 
static struct kobj_attribute stop_attribute = __ATTR(stop, 0220, NULL, stop_timer); 
 
static struct attribute *attrs[] = { 
    &value_attribute.attr, 
    &start_attribute.attr, 
    &stop_attribute.attr, 
    NULL, 
}; 
 
static struct attribute_group attr_group = { 
    .attrs = attrs, 
};
 
void timer_callback(struct timer_list *timer) {  
    global_variable++;  
    printk(KERN_INFO "Timer callback called! Global variable: %d\n", global_variable);  
    if (timer_running) {  
        mod_timer(timer, jiffies + msecs_to_jiffies(1000)); // 1 second interval  
    }  
}  
 
static ssize_t read_value(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {  
    return sprintf(buf, "%d\n", global_variable);  
}  
 
static ssize_t reset_value(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {  
    global_variable = 0;  
    return count;  
}  
 
static ssize_t start_timer(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {  
    if (!timer_running) {  
        timer_running = true;  
        mod_timer(&myTimer, jiffies + msecs_to_jiffies(1000)); // Start timer  
        printk(KERN_INFO "Timer started\n");  
    }  
    return count;  
}  
 
static ssize_t stop_timer(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {  
    if (timer_running) {  
        timer_running = false;  
        del_timer(&myTimer); // Stop timer  
        printk(KERN_INFO "Timer stopped\n");  
    }  
    return count;  
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
};  
 
static int __init my_module_init(void) {
	printk(KERN_INFO "my_module_init called\n");    
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);  
    if (majorNumber < 0) {  
        printk(KERN_ALERT "Failed to register a major number\n");  
        return majorNumber;  
    }
    printk(KERN_INFO "Major number registred\n");     
 
    myClass = class_create(CLASS_NAME);  
    if (IS_ERR(myClass)) {  
        unregister_chrdev(majorNumber, DEVICE_NAME);  
        printk(KERN_ALERT "Failed to register device class\n");  
        return PTR_ERR(myClass);  
    }
    printk(KERN_INFO "Device class registred\n");   
 
    myDevice = device_create(myClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);  
    if (IS_ERR(myDevice)) {  
        class_destroy(myClass);  
        unregister_chrdev(majorNumber, DEVICE_NAME);  
        printk(KERN_ALERT "Failed to create the device\n");  
        return PTR_ERR(myDevice);  
    }
    printk(KERN_INFO "Device registred\n");   
    
    // Create sysfs entries
    if (sysfs_create_group(&myDevice->kobj, &attr_group)) { 
        device_destroy(myClass, MKDEV(majorNumber, 0)); 
        class_destroy(myClass); 
        unregister_chrdev(majorNumber, DEVICE_NAME); 
        return -1; 
    } 
    
    printk(KERN_INFO "Sysfs registred\n"); 
    
 
    // Initialize timer  
    timer_setup(&myTimer, timer_callback, 0);  
    
    printk(KERN_INFO "Device created successfully\n");  
    return 0;  
}  
 
static void __exit my_module_exit(void) {  
    del_timer(&myTimer); 
    sysfs_remove_group(&myDevice->kobj, &attr_group); 
    device_destroy(myClass, MKDEV(majorNumber, 0)); 
    class_destroy(myClass); 
    unregister_chrdev(majorNumber, DEVICE_NAME); 
    printk(KERN_INFO "Device destroyed successfully\n"); 
} 
 
module_init(my_module_init); 
module_exit(my_module_exit); 
 
MODULE_LICENSE("GPL"); 
