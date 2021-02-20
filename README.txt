This module implements a "cyclic executive" scheduling module: 
a static schedule (specifying when to schedule some tasks, and specifying the task ID for each task) can be set by writing
to a misc device; then, when the schedule is started, the module will schedule/deschedule each thread at the right time.

Instructions for the usage of this kernel module:

-	Modify your Linux kernel source code as explained in kernel_modifications.txt
	This is because we need to use some of the functions that are not exported in the official version.
-	Compile linux kernel
-	Compile this module: 
		make KERNEL_DIR=<path_to_linux_kernel_dir>
-	Run the example on qemu:
		make run
-	When Linux is running, execute the following command:
		sudo sh /test/test_run
	This will run the test

Instructions to modify the test settings:

	To change the test setup, you need to change the source code inside test_main.c:

		To modify the number of slots or their lenght, change the values of the NUM_SLICES and SLICE_SIZE macros
		To modify the thread number and allocation,ì:
		-	Change the value of the NUM_THREAD macro
		-	Add or remove one of the lines that looks like this
				ret |= pthread_create(&mythread[1], NULL , thread_body_common, (void*)4);
			The last parameter is the slot number where the new thread is going to execute
			The first is the element inside the threads' vector. Change the pointer correctly to give its place to each thread.


