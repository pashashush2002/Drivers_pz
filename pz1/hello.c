#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/delay.h>
MODULE_LICENSE("GPL");

struct task_struct *ts;
static char *whom = "world";
module_param(whom, charp, 0);
static int howmany = 1;
module_param(howmany, int, 0);         // Передается пара параметров - сколько рaз и  кому передается привет 

int thread(void *data) {
  while(1) {
      printk(KERN_ALERT "Hello, %s\n", whom);
      msleep(howmany);
      if (kthread_should_stop())          break;
  }
  return 0;
}

int init_module(void) {
  printk(KERN_INFO "init_module() called\n");
  ts=kthread_run(thread,NULL,"foo kthread");
  return 0;
}

void cleanup_module(void) {
  printk(KERN_INFO "cleanup_module() called\n");
  kthread_stop(ts);
}
