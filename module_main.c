#include <linux/poll.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/types.h>

MODULE_AUTHOR("Matteo Zini");
MODULE_DESCRIPTION("Scheduler");
MODULE_LICENSE("GPL");

#define MAX_MESS_LEN		70
#define MAX_RESPONSE_LEN	70


/*************** DATA STRUCTURES *******************/


struct schedule_node {
    int slot_number;
	int thread_id;
};


/*************** GLOBAL VARIABLES ******************/

long int slice_size;
long int num_slices;

struct schedule_node* task_list;


/*************** UTILITY FUNCTIONS *****************/

long int strtol(const char* str, char** endptr, int base)
{
	long int res;
	int err, i;
	char temp_buf[10];

	for (i = 0; str[i] >= '0' && str[i] <= '9'; i++)
		temp_buf[i] = str[i];

	temp_buf[i] = '\0';
	
	err = kstrtol (temp_buf, base, &res);
	if (err) 
		return -1;

	//endptr setting (Do this at the end beacuse it could be a pointer to str!)
	*endptr = (char*)str + i;
	
	return res;
}




/*************** SCHEDULER FUNCTIONS *****************/


static struct task_struct* sched_thread_descr;


int scheduler_body(void* arg)
{
	uint64_t time_cycle_start, delay_us;
	int i;
	struct task_struct* task;

	printk("The scheduler is running!");

 	while (!kthread_should_stop()) {
		time_cycle_start = ktime_get_ns();
		
		for (i = 0; i < num_slices; i++) {
			if(task_list[i].thread_id != 0) {

				task = pid_task(find_vpid(task_list[i].thread_id), PIDTYPE_PID);
				wake_up_process(task);
			}

			delay_us = ( (time_cycle_start + (i+1)*1000*slice_size) - ktime_get_ns() ) / 1000;
			usleep_range(delay_us, delay_us);
		}
		
		
		printk("The scheduler is hardly working for you");
		msleep(1000);
	}

	printk("The scheduler is terminating!");

	return 0;
}


int start_scheduler(void)
{
	printk("Starting the scheduler...");
	sched_thread_descr = kthread_run(scheduler_body, NULL, "scheduler_thread");

	if (IS_ERR(sched_thread_descr)) {
    	printk("Error creating kernel thread!\n");

    	return PTR_ERR(sched_thread_descr);
	}

	return 0;
}

void stop_scheduler(void)
{
	int res;

	printk("Stopping the scheduler...");
  
	res = kthread_stop(sched_thread_descr);

	printk("Schduler killed with result: %d\n", res);
}











/******* PARKING DEVICE **********/

static int parking_open(struct inode *inode, struct file *file)
{  
	printk("Parking device open!\n");

	return 0;
}

static int parking_close(struct inode *inode, struct file *file)
{
	printk("Parking device close!\n");

	return 0;
}

ssize_t parking_read(struct file *file, char __user *buf, size_t len, loff_t *ppos)
{
	//pid and tgid difference and relation w/ user space equivalent terms: https://stackoverflow.com/questions/13002444/list-all-threads-within-the-current-process
	printk("Task %d is now sleeping\n", current->pid);

	set_current_state(TASK_UNINTERRUPTIBLE);
    schedule();

	return 0;
}

static struct file_operations parking_fops = {
	.owner =        THIS_MODULE,
	.read =         parking_read,
	.open =         parking_open,
	.release =      parking_close,
};

static struct miscdevice parking_device = {
	MISC_DYNAMIC_MINOR, "scheduler_parking", &parking_fops
};



/******* COMMUNICATION DEVICE **********/

static int communication_open(struct inode *inode, struct file *file)
{
	printk("Communication device open!\n");

	return 0;
}

static int communication_close(struct inode *inode, struct file *file)
{
	printk("Communication device close!\n");

	return 0;
}

ssize_t communication_read(struct file *file, char __user *buf, size_t len, loff_t *ppos)
{
	char response[MAX_RESPONSE_LEN];
	int resp_len = 0;
	int err, i;

	if (num_slices <= 0 || slice_size <= 0)
		return -EFAULT;

	resp_len += sprintf(response, "num_slices: %ld; slice_size: %ld\n", num_slices, slice_size);

	for (i = 0; i < num_slices; i++) {

		if (task_list[i].thread_id == 0)
			resp_len += sprintf(response + resp_len, "--,");
		else
			resp_len += sprintf(response + resp_len, "%d,", task_list[i].thread_id);
	}

	if (resp_len >= len)
		resp_len = len;
  
	err = copy_to_user(buf, response, resp_len);

	if (err)
		return -EFAULT;

	return resp_len;
}


ssize_t communication_write(struct file *file, const char __user * buf, size_t count, loff_t *ppos)
{
	int err, i;
	char message_buffer[MAX_MESS_LEN + 1];
	char* message_buffer_ptr = message_buffer;
	char* found;
	long int slice_id, task_id;

	if(count > MAX_MESS_LEN)
		return -1;
	
	err = copy_from_user(message_buffer, buf, count);
  	if (err) {
    	return -EFAULT;
	}

	message_buffer[count] = '\0';


	while( (found = strsep(&message_buffer_ptr, ":"), message_buffer_ptr != NULL) ) {

		if(strncmp("slice_size", found, 10) == 0) {

			slice_size = strtol(message_buffer_ptr, &message_buffer_ptr, 10);
			
			if (slice_size <= 0)
				return -EFAULT;

		} else if (strncmp("num_slices", found, 10) == 0 && num_slices == 0) {

			num_slices = strtol(message_buffer_ptr, &message_buffer_ptr, 10);

			if (num_slices <= 0)
				return -EFAULT;

			task_list = kmalloc(num_slices*sizeof(struct schedule_node), GFP_KERNEL);
			for (i = 0; i < num_slices; i++) {
				task_list[i].thread_id = 0;
				task_list[i].slot_number = i;
			}

			if (task_list == NULL)
				return -EFAULT;

		} else if ((strncmp("slice", found, 5) == 0)) {
			
			slice_id = strtol(message_buffer_ptr, &message_buffer_ptr, 10);

			if (slice_id < 0 || slice_id >= num_slices || *message_buffer_ptr != ',')
				return -EFAULT;

			message_buffer_ptr++;

			task_id = strtol(message_buffer_ptr, &message_buffer_ptr, 10);

			if (task_id <= 0)
				return -EFAULT;

			task_list[slice_id].thread_id = task_id;
		
		} else if ((strncmp("ctrl", found, 5) == 0)) {
			found = strsep(&message_buffer_ptr, ";");

			if (found == NULL)
				return -EFAULT;

			if (strcmp(found, "start") == 0)
				start_scheduler();
			else if (strcmp(found, "stop") == 0)
				stop_scheduler();
			else
				return -EFAULT;
			
			//skip final part because it has been already done
			continue;
		
		} else {
			return -EFAULT;
		}

		//check that next option is separed by a ';'
		if(*message_buffer_ptr != ';')
			break;

		message_buffer_ptr++;
	}

	return message_buffer_ptr - message_buffer;
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
