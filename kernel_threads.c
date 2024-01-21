#include "kernel_thread.h"      // some debug functions
#include <linux/kernel.h>       // do_exit()
#include <linux/threads.h>      // allow_signal
#include <linux/kthread.h>      // kthread_create
#include <linux/delay.h>        // ssleep()

// task structs, cpu id's as integer
static struct task_struct *worker_task, *default_task;
static int get_current_cpu, set_current_cpu;
#define WORKER_THREAD_DELAY 4
#define DEFAULT_THREAD_DELAY 6

// worker task main constructor
// it will forever after while() loop
// arguments from when task creating with kthread_creat()
static int worker_task_handler_fn(void *arguments) {
	// this macro will allow to stop thread from userspace or kernelspace
	allow_signal(SIGKILL);

	// while(true),while(1==1),for(;;) loops will can't receive signal for stopping thread
	while(!kthread_should_stop()) {
		PINFO("Worker thread executing on system CPU:%d \n",get_cpu());
		ssleep(WORKER_THREAD_DELAY);

		// you must check signals in forever loops everytime
		// if signal_pending function capture SIGKILL signal, then thread will exit
		if (signal_pending(worker_task)) break;
	}
	// do_exit is same as exit(0) function, but must be used with threads

	// Kernel Thread has higher priority than user thread because Kernel threads
	// are used to provide privileged services to applications.
	do_exit(0);
	PERR("Worker task exiting\n");

	return 0;
}

// default task main constructor
// this task have highest real time priority
// it will forever after while() loop
// arguments from when task creating with kthread_creat()
static int default_task_handler_fn(void *arguments) {
	// here is your world, you can declare your variables,structs
	// this macro will allow to stop thread from userspace or kernelspace
	allow_signal(SIGKILL);

	//while(true),while(1==1),for(;;) loops will can't receive signal for stopping thread
	while(!kthread_should_stop()) {
		PINFO("Default thread executing on system CPU:%d \n",get_cpu());
		ssleep(DEFAULT_THREAD_DELAY);

		// you must check signals in forever loops everytime
		// if signal_pending function capture SIGKILL signal, then thread will exit
	    if (signal_pending(default_task)) break;
	}
	PERR("Default task exiting\n");

	// do_exit is same as exit(0) function, but must be used with threads
	do_exit(0);

	return 0;
}

// run only once when module installed
// returns signal status with int parameter
static int __init kernel_thread_init(void) {
	// scheduler priority structs to set task priority
	struct sched_param task_sched_params = {.sched_priority = MAX_RT_PRIO};

	task_sched_params.sched_priority = 90;

	PINFO("Initializing kernel mode thread example module\n");
	PINFO("Creating Threads\n");

	// get current cpu to bind over task
	get_current_cpu = get_cpu();
	PDEBUG("Getting current CPU %d to binding worker thread\n", get_current_cpu);

	// initialize worker task with arguments, thread_name and cpu
	// you can see thread name using ps aux command on userspace
	worker_task = kthread_create(worker_task_handler_fn,
			(void*)"arguments as char pointer","thread_name");
	kthread_bind(worker_task,get_current_cpu);

	if(worker_task)
		PINFO("Worker task created successfully\n");
	else
		PINFO("Worker task error while creating\n");

	// initialize default task with arguments, thread_name and another cpu
	// you can see thread name using ps aux command on userspace
	set_current_cpu = 2;
	PDEBUG("Getting current CPU %d to binding default thread\n",
											set_current_cpu);

	default_task = kthread_create(default_task_handler_fn,
				(void*)"arguments as char pointer","thread_name");
	kthread_bind(default_task,set_current_cpu);

	// set scheduler priority to default task
	sched_setscheduler(default_task, SCHED_FIFO, &task_sched_params);

	// tasks are now process, start them
	wake_up_process(worker_task);
	wake_up_process(default_task);

	// check task if they are started succesfully
	if(worker_task)
		PINFO("Worker thread running\n");
	else
		PERR("Worker task can't start\n");

	if(default_task)
		PINFO("Default thread running\n");
	else
		PERR("Default task can't start\n");

	return 0;
}

// run only once when module removed
// returns signal status with int parameter
static void __exit kernel_thread_exit(void) {	
	PERR("Module removing from kernel, threads stopping\n");

	// this functions will send SIGKILL's to stop threads when module removing
	if(worker_task)
		kthread_stop(worker_task);

	PERR("Worker task stopped\n");

	if(default_task)
		kthread_stop(default_task);

	PERR("Default task stopped\n");

	PINFO("Bye Bye\n");
}

// set initial and exit callback functions to module macro
module_init(kernel_thread_init);
module_exit(kernel_thread_exit);

