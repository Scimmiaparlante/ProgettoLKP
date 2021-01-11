#include <linux/poll.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/list.h>


MODULE_AUTHOR("Matteo Zini");
MODULE_DESCRIPTION("Scheduler");
MODULE_LICENSE("GPL");

#define MAX_MESS_LEN		70
#define MAX_RESPONSE_LEN	70


/*************** DATA STRUCTURES *******************/


struct scehdule_node {
    int slot_number;
	int thread_id;
};


/*************** GLOBAL VARIABLES ******************/

long int slice_size;
long int num_slices;

struct scehdule_node* task_list;


/******* PARKING DEVICE **********/

static int parking_open(struct inode *inode, struct file *file)
{
  /*int *count;

  count = kmalloc(sizeof(int), GFP_USER);
  if (count == NULL) {
    return -1;
  }

  *count = 0;
  file->private_data = count;*/
  
  printk("Parking device open!\n");

  return 0;
}

static int parking_close(struct inode *inode, struct file *file)
{
  //kfree(file->private_data);
  printk("Parking device close!\n");

  return 0;
}

ssize_t parking_read(struct file *file, char __user *buf, size_t len, loff_t *ppos)
{
  int res, err, *count;

  count = file->private_data;
  *count = *count + 1;
  if (*count == 10) {
    return 0;
  }

  if (len > 10) {
    res = 10;
  } else {
    res = len;
  }
  err = copy_to_user(buf, "hi there!\n", res);
  if (err) {
    return -EFAULT;
  }
  printk("Read %ld (%d)\n", len, *count);

  return res;
}

static struct file_operations parking_fops = {
  .owner =        THIS_MODULE,
  .read =         parking_read,
  .open =         parking_open,
  .release =      parking_close,
#if 0
  .write =        my_write,
  .poll =         my_poll,
  .fasync =       my_fasync,
#endif
};

static struct miscdevice parking_device = {
  MISC_DYNAMIC_MINOR, "scheduler_parking", &parking_fops
};



/******* COMMUNICATION DEVICE **********/

static int communication_open(struct inode *inode, struct file *file)
{
  /*int *count;

  count = kmalloc(sizeof(int), GFP_USER);
  if (count == NULL) {
    return -1;
  }

  *count = 0;
  file->private_data = count;*/

  
  printk("Communication device open!\n");

  return 0;
}

static int communication_close(struct inode *inode, struct file *file)
{
  //kfree(file->private_data);
  printk("Communication device close!\n");

  return 0;
}

ssize_t communication_read(struct file *file, char __user *buf, size_t len, loff_t *ppos)
{
	char response[MAX_RESPONSE_LEN];
	int resp_len = 0;
	int err;

	if (num_slices <= 0 || slice_size <= 0)
	return -EFAULT;

	resp_len += sprintf(response, "num_slices: %ld; slice_size: %ld\n", num_slices, slice_size);

	for (int i = 0; i < num_slices; i++) {
	if (task_list->thread_id == 0)
		resp_len += sprintf(response + len, "--,");
	else
		resp_len += sprintf(response + len, "%d,", task_list->thread_id);
	}

	if (resp_len >= len)
		resp_len = len;
  
	err = copy_to_user(buf, response, resp_len);

	if (err)
		return -EFAULT;

	return resp_len
}


ssize_t communication_write(struct file *file, const char __user * buf, size_t count, loff_t *ppos)
{
	int err;
	char message_buffer[MAX_MESS_LEN + 1];
	char* message_buffer_initial = message_buffer;
	char* found;
	long int slice_id, task_id;

	if(count > MAX_MESS_LEN)
		return -1;
	
	err = copy_from_user(message_buffer, buf, count);
  	if (err) {
    	return -EFAULT;
	}

	message_buffer[count] = '\0';


	while( (found = strsep(&message_buffer, ":;")) != NULL ) {

		if(strncmp("slice_size", found, 10) == 0 && *message_buffer == ':') {
			
			slice_size = strtol(message_buffer, &message_buffer, 10);
			
			if (slice_size <= 0)
				return -EFAULT;
		} else if (strncmp("num_slices", found, 10) == 0 && *message_buffer == ':' && num_slices == 0) {

			slice_size = strtol(message_buffer, &message_buffer, 10);
			
			if (num_slices <= 0)
				return -EFAULT;

			task_list = kmalloc(num_slices*sizeof(struct scehdule_node), GFP_KERNEL);
			for (int i = 0; i < num_slices; i++) {
				task_list->thread_id = 0;
				task_list->slot_number = i;
			}


			if (task_list == NULL)
				return -EFAULT;

		} else if ((strncmp("slice", found, 5) == 0)) {

			slice_id = strtol(message_buffer, &message_buffer, 10);

			if (slice_id <= 0 || slice_id > num_slices || *message_buffer == ',')
				return -EFAULT;

			task_id = strtol(message_buffer, &message_buffer, 10);

			if (task_id <= 0)
				return -EFAULT;

			task_list[slice_id] = task_id;

		} 
		else {
			return -EFAULT;
		}
	}

  return message_buffer - message_buffer_initial;
}


static struct file_operations communication_fops = {
  .owner =        THIS_MODULE,
  .read =         communication_read,
  .open =         communication_open,
  .release =      communication_close,
  .write =        communication_write,
};

static struct miscdevice communication_device = {
  MISC_DYNAMIC_MINOR, "scheduler_setting", &communication_fops
};




static int testmodule_init(void)
{
	int res;

	res = misc_register(&parking_device);
	res = misc_register(&communication_device);


	printk("Misc Register returned %d\n", res);

	return 0;
}

static void testmodule_exit(void)
{
	misc_deregister(&parking_device);
	misc_deregister(&communication_device);
}

module_init(testmodule_init);
module_exit(testmodule_exit);
