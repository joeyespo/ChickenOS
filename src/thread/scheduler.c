/*	ChickenOS - thread/scheduler.c
 *	Very basic scheduler, called on every timer interrupt  
 */
#include <common.h>
#include <kernel/thread.h>
#include <thread/tss.h> 
#include <kernel/memory.h>
#include <device/console.h>
#include <kernel/interrupt.h>
#include <mm/vm.h>
#include <thread/syscall.h>
#include <stdio.h>

thread_t *table[MAX_THREADS];
int thread_cnt = 1;
int thread_ptr = 0;
struct list;
struct list {
	struct list *next;
	void *data;
};

void thread_scheduler_init(thread_t *kernel_thread)
{
	kmemset(table, 0, MAX_THREADS*sizeof(thread_t*));
	table[0] = kernel_thread;
	thread_set_ready(kernel_thread);
	tss_init();
}

void thread_scheduler(registers_t *regs)
{
	uint32_t _esp = 0xfeedface;
	thread_t *cur = thread_current();
	thread_t *next = thread_next();
//	if(next == NULL)
		next = cur;
	//i believe that signals go here:
	//if(next->signal_pending != 0)
	//signal(regs, next);
	
	cur->sp = (uint8_t *)regs->ESP - 4;
	cur->regs = regs;
	_esp = (uint32_t)next->sp;

	tss_update((uintptr_t)next + STACK_SIZE);
	printf("Switching to pid %i from pid %i esp %x regs->esp %x %x\n\n",
		next->pid, cur->pid,_esp, regs->esp,  regs->eip, next->regs->eip);

	printf("resg %x next->regs %x %X %X	%X %X\n\n", regs, next->regs, cur, next, cur->sp, _esp);
	dump_regs(regs);
	printf("\n");
	dump_regs(next->regs);
	printf("\n");

//	dump_regs(next->regs);
//	next->regs->ss = 0x1b;

	//have to reset timer interrupt here
	extern void pic_send_end(int irq);
	pic_send_end(0);

	asm volatile(
					"mov %0,%%esp\n"
					"jmp intr_return"
					:: "r"(_esp)
				);
}

/* throw an int 32, manually invoking the timer interrupt */
/* can we just call the scheduler using the saved refs in 
 * thread struct?
 */
void thread_yield()
{
	asm volatile("int $32");
}

void thread_set_ready(thread_t *thread)
{
	THREAD_ASSERT(thread);
	thread->status = THREAD_READY;
}

void thread_queue(thread_t *thread)
{
	THREAD_ASSERT(thread);
	table[thread_cnt] = thread;
	thread_cnt++;
}

thread_t *thread_dequeue()
{
	thread_t *ret = thread_current();

	return ret;
}

//TODO: Implement this as a heap-based priority queue
thread_t *thread_next()
{
//	thread_t *cur = thread_current();
	if(thread_ptr > thread_cnt)
	thread_ptr = 0;

	return table[thread_ptr++];
}


void thread_exit()
{
	asm volatile("cli");
	thread_t *cur = thread_current();
	//keep a tmp pointer to next process
	//which the scheduler uses to get the next process
	//need to set a value in the thread_t to tell
	//the scheduler to do this
	
	//if we aren't the original kernel thread, free the stack
	if(cur->pid != 0)
	{
		//FIXME: This needs to be done differently
//		pallocn_free(cur, STACK_PAGES);	

	}
	cur->status = THREAD_DEAD;
	asm volatile("sti");
	thread_yield();

}


