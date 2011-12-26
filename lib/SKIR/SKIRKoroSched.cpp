#include <llvm/Support/raw_ostream.h>

#include "SKIR/SKIRRuntime.h"
#include "SKIRRuntimeGraph.h"
#include "SKIRKoroSched.h"
#include "SKIR_kernel_t.h"

#include "llvm/Support/CommandLine.h"

#include "SKIRTiming.h"

namespace llvm {

bool DisableKoroSteal;
static cl::opt<bool, true>
FakeDisableKoroSteal("disable-coro-steal",
		     cl::desc("disable coroutine stealing under single threaded execution"),
		     cl::location(DisableKoroSteal), cl::init(false));

bool DisableKoroElim;
static cl::opt<bool, true>
FakeDisableKoroElim("disable-coro-elim",
		   cl::desc("disable coroutine elimination"),
		   cl::location(DisableKoroElim), cl::init(false));

// kernel_map[key]
kernel_t *
SKIRKoroSched::kernel_map_find(SKIRRuntimeKernel *key)
{
    kernel_map_t::const_accessor a;
    if (kernel_map.find(a,key))
	return a->second;
    return NULL;
}

// kernel_map[key] = value
void
SKIRKoroSched::kernel_map_insert(SKIRRuntimeKernel *key, kernel_t *value)
{
    kernel_map_t::accessor a;
    if (!kernel_map.find(a,key))
	kernel_map.insert(a, key);
    a->second = value;
}

void
SKIRKoroSched::callKernel(SKIRRuntimeKernel *rtk)
{
    kernel_t *k = 0;
    k = new kernel_t(*rtk);
    assert(k);
    kernel_map_insert(rtk, k);
    k->active();
    runq.push(rtk);
    start();
}

void
SKIRKoroSched::waitKernel(SKIRRuntimeKernel *rtk)
{
    kernel_t *k = kernel_map_find(rtk);
    assert(k);
    k->wait(DONE);
}

void
SKIRKoroSched::pauseKernel(SKIRRuntimeKernel *rtk)
{
    kernel_t *k = kernel_map_find(rtk);
    assert(k);
    k->pause();
}

void
SKIRKoroSched::unPauseKernel(SKIRRuntimeKernel *rtk)
{
    kernel_t *k = kernel_map_find(rtk);
    assert(k);
    k->unpause();
}

struct skir_koro_thread {
    SKIRKoroSched &s;
    skir_koro_thread(SKIRKoroSched &sched) : s(sched) {}
    void operator()() { s.run(); }
};

void
SKIRKoroSched::start(void)
{
    if (running.compare_and_swap(1,0) == 0) {
	main_thread = new tbb::tbb_thread(*(new skir_koro_thread(*this)));
	assert(main_thread);
    }
}

void
SKIRKoroSched::stop(void)
{
    if (!main_thread) return;

    if (running.compare_and_swap(0,1) == 1) {
	assert(main_thread && main_thread->joinable() && "not running");
	main_thread->join();
	main_thread = NULL;
    }

    SKIRRuntimeKernel *k;
    while (runq.try_pop(k))
	removeKernel(k);
}

// stop executing rtk, and remove internal state associated with it
void
SKIRKoroSched::removeKernel(SKIRRuntimeKernel *rtk)
{
    kernel_t *k = kernel_map_find(rtk);
    if (!k) return;

    // pause (lock) the kernel so it won't get picked for execution
    k->pause();

    // remove the kernel from the internal list
    kernel_map_insert(rtk, NULL);

    // release the pause thread
    k->done();
    
    // once we can get the lock, it's safe to delete k
    {
	kernel_lock_t::scoped_lock lock;
	lock.acquire(k->lock);
	k->running = 0;
    }
    
    delete k;
}

//
//
//

extern "C" {

struct koroutine 
{
    void *saved_sp;  // 0 current stack pointer of koroutine
    void *saved_pc;  // 4,8 where to resume to

    void *yield_pc;  // 8,16  yield address
    void *yield_sp;  // 12,24 saved stack pointer of caller

    void *fn;           // 16,32 workfn
    void *stack_base;   // 20,40 starting address of koroutine stack
    size_t stack_size;  // 24,48 size of koroutine stack
};

}

#if 0
extern "C" void __SKIRRT_yield32(void *from_rtk, void *to_rtk);
asm(							  \
    ".text\n"						  \
    ".global __SKIRRT_yield32\n"			  \
    ".type __SKIRRT_yield32 @function\n"		  \
    ".align 16\n"					  \
  "__SKIRRT_yield32:\n"					  \
    "popl %ecx\n"					  \
    "movl (%esp), %edx\n"				  \
    "movl 4(%esp), %eax\n"				  \
    "movl 4(%edx), %edx\n"				  \
    "movl 0(%edx), %edx\n"				  \
    "pushl %ebp\n"					  \
    "pushl %ebx\n"					  \
    "pushl %esi\n"					  \
    "pushl %edi\n"					  \
    "movl %esp, 0(%edx)\n"				  \
    "movl %ecx, 4(%edx)\n"				  \
    "movl 8(%edx),  %ecx\n"				  \
    "movl 12(%edx), %esp\n"				  \
    "jmp *%ecx\n"					  \
    "ud2\n");

extern "C" void __SKIRRT_return32(void *rt_state, int ret_val);
asm(							  \
    ".text\n"						  \
    ".global __SKIRRT_return32\n"			  \
    ".type __SKIRRT_return32 @function\n"		  \
    ".align 16\n"					  \
  "__SKIRRT_return32:\n"				  \
    "popl %ecx\n"					  \
    "popl %edx\n"					  \
    "popl %eax\n"					  \
    "movl 0(%edx), %edx\n"				  \
    "movl 20(%edx), %ecx\n"				  \
    "addl 24(%edx), %ecx\n"				  \
    "subl $36, %ecx\n"					  \
    "movl %ecx, 0(%edx)\n"				  \
    "movl $0, 4(%ecx)\n"				  \
    "movl $0, 8(%ecx)\n"				  \
    "movl $0, 12(%ecx)\n"				  \
    "movl $0, 16(%ecx)\n"				  \
    "movl 16(%edx), %ecx\n"				  \
    "movl %ecx, 4(%edx)\n"				  \
    "movl 8(%edx),  %ecx\n"				  \
    "movl 12(%edx), %esp\n"				  \
    "jmp *%ecx\n"					  \
    "ud2\n");
#endif

extern "C" void __SKIRRT_yield64(void *from_rtk, void *to_rtk);
asm(							  \
    ".text\n"						  \
    ".global __SKIRRT_yield64\n"			  \
    ".type __SKIRRT_yield64 @function\n"		  \
    ".align 16\n"					  \
  "__SKIRRT_yield64:\n"					  \
    "popq %rcx\n"					  \
    "movq 8(%rdi), %rdi\n"				  \
    "movq 0(%rdi), %rdi\n"				  \
    "movq 24(%rdi), %r11\n"				  \
    "movq 48(%r11), %r10\n"				  \
    "pushq %rbx\n"					  \
    "pushq %rbp\n"					  \
    "pushq %r12\n"					  \
    "pushq %r13\n"					  \
    "pushq %r14\n"					  \
    "pushq %r15\n"					  \
    "movq %rsp, 0(%rdi)\n"				  \
    "movq %rcx, 8(%rdi)\n"				  \
    "movq  %r11, %rsp\n"				  \
    "popq  %r15\n"					  \
    "popq  %r14\n"					  \
    "popq  %r13\n"					  \
    "popq  %r12\n"					  \
    "popq  %rbp\n"					  \
    "popq  %rbx\n"					  \
    "movq  %rsi, %rax\n"				  \
    "addq  $8, %rsp\n"					  \
    "jmp *%r10\n"					  \
    "ud2\n");

extern "C" void __SKIRRT_return64(void *rt_state, int ret_val);
asm(							  \
    ".text\n"						  \
    ".global __SKIRRT_return64\n"			  \
    ".type __SKIRRT_return64 @function\n"		  \
    ".align 16\n"					  \
  "__SKIRRT_return64:\n"				  \
    "movq 0(%rdi), %rdi\n"				  \
    "movq 24(%rdi), %r11\n"				  \
    "movq 32(%rdi), %rdx\n"				  \
    "movq 40(%rdi), %rcx\n"				  \
    "addq 48(%rdi), %rcx\n"				  \
    "movq 48(%r11), %r10\n"				  \
    "subq $56, %rcx\n"					  \
    "movq %rcx, 0(%rdi)\n"				  \
    "movq %rdx, 8(%rdi)\n"				  \
    "movq %r11, %rsp\n" 				  \
    "popq %r15\n"					  \
    "popq %r14\n"					  \
    "popq %r13\n"					  \
    "popq %r12\n"					  \
    "popq %rbp\n"					  \
    "popq %rbx\n"					  \
    "movq %rsi, %rax\n"					  \
    "addq $8, %rsp\n"					  \
    "jmp *%r10\n"					  \
    "ud2\n");

#if 0
//
// koro_workfn_32:
//   load rt_state.stk (i.e. koroutine *koro) into EAX
//   save callee save regs
//   save address to yield to into koro->yield_pc
//   load koro->saved_sp into ESP
//   load koro->saved_pc into ECX
//   restore callee save reg for workfn
//   branch to workfn
extern "C" void *koro_workfn_32(void *rt_state, void *state, void *impl_ins, void *impl_outs);
asm(							  \
    ".text\n"						  \
    ".global koro_workfn_32\n"				  \
    ".type koro_workfn_32 @function\n"			  \
    ".align 16\n"					  \
  "koro_workfn_32:\n" 					  \
    "movl  4(%esp), %eax\n"				  \
    "movl  0(%eax), %eax\n"				  \
    "pushl %ebp\n"					  \
    "pushl %ebx\n"					  \
    "pushl %esi\n"					  \
    "pushl %edi\n"					  \
    "movl  $koro_workfn_32_yield_pt, 8(%eax)\n"		  \
    "movl  %esp, 12(%eax)\n"				  \
    "movl  0(%eax), %esp\n"				  \
    "movl  4(%eax), %ecx\n"				  \
    "popl  %edi\n"					  \
    "popl  %esi\n"					  \
    "popl  %ebx\n"					  \
    "popl  %ebp\n"					  \
    "jmp   *%ecx\n"					  \
    "ud2\n");

// koro_run_yield_pt: (entered from SKIRRT_return or SKIRRT_yield)
//   restore calle save reg for caller
//   return (return value must already be in EAX)
//
extern "C" void *koro_workfn_32_yield_pt(void);
asm(							  \
    ".text\n"						  \
    ".global koro_workfn_32_yield_pt\n"			  \
    ".type koro_workfn_32_yield_pt @function\n"		  \
    ".align 16\n"					  \
  "koro_workfn_32_yield_pt:\n"				  \
    "popl  %edi\n"					  \
    "popl  %esi\n"					  \
    "popl  %ebx\n"					  \
    "popl  %ebp\n"					  \
    "ret\n"						  \
    "ud2\n");
#endif

extern "C" void *koro_workfn_64(void *rt_state, void *state, void *impl_ins, void *impl_outs);
asm(							  \
    ".text\n"						  \
    ".global koro_workfn_64\n"				  \
    ".type koro_workfn_64 @function\n"			  \
    ".align 16\n"					  \
  "koro_workfn_64:\n" 					  \
    "movq  0(%rdi), %r10\n"				  \
    "movq  0(%r10), %r8\n"				  \
    "movq  8(%r10), %r11\n"				  \
    "pushq %rbx\n"					  \
    "pushq %rbp\n"					  \
    "pushq %r12\n"					  \
    "pushq %r13\n"					  \
    "pushq %r14\n"					  \
    "pushq %r15\n"					  \
    "movq  %rsp, 24(%r10)\n"				  \
    "movq  %r8, %rsp\n"					  \
    "popq  %r15\n"					  \
    "popq  %r14\n"					  \
    "popq  %r13\n"					  \
    "popq  %r12\n"					  \
    "popq  %rbp\n"					  \
    "popq  %rbx\n"					  \
    "jmp   *%r11\n"					  \
    "ud2\n");

void
SKIRKoroSched::runCodeGen(SKIRRuntimeKernel *rtk)
{
    // defined below
    genericCodeGen(sg, rtk);
}

void
SKIRKoroSched::genericCodeGen(SKIRRuntimeGraph *sg, SKIRRuntimeKernel *rtk)
{
    double t_begin;
    rdtod(t_begin);

    if (rtk->fpm)
	delete rtk->fpm;
    rtk->fpm = new FunctionPassManager(rtk->work->getParent());
    rtk->fpm->add(new TargetData(rtk->work->getParent()));

    if (!rtk->opt_only) {
	if (!rtk->is_fixed_rate || DisableKoroElim) {
	    SKIRRuntime::addSKIROuterLoopPass(rtk, "loop", "");
	    SKIRRuntime::addLLVMOpts(rtk);
	    SKIRRuntime::addSKIRStreamOptsPass(rtk);
	    SKIRRuntime::addSKIRInlineStreamsPass(rtk);
	    SKIRRuntime::addSKIRKoroPass(rtk);
	    SKIRRuntime::addLLVMOpts(rtk);

	    sg->codeGenKernel(rtk);
	    assert(rtk->workfn);

	    koroutine *koro;
	    rtk->rt_state->stk = koro = (koroutine*)malloc(sizeof(koroutine));

	    // setup new stack
	    koro->stack_size = 4096;
	    koro->saved_sp = koro->stack_base = malloc(koro->stack_size);
	    koro->saved_sp = (void*)((size_t)koro->saved_sp + koro->stack_size);

	    void *fp = reinterpret_cast<void*>(reinterpret_cast<size_t>(rtk->workfn));
	    koro->saved_pc = koro->fn = fp;
#if 0 // if 1 -> 32bit, could be broken
	    koro->yield_pc 
		= reinterpret_cast<void*>(reinterpret_cast<size_t>(koro_workfn_32_yield_pt));
#else
            // 64-bit
	    koro->yield_pc = 0;
#endif

	    koro->yield_sp = 0;

#define KORO_PUSH(data) {						\
		size_t d = (size_t)data;				\
		koro->saved_sp = (void*)((char*)koro->saved_sp - sizeof(d)); \
		memcpy(koro->saved_sp, &d, sizeof(d));			\
	    }

#if 0 // if 1 -> 32bit, could be broken
	    // push arguments to work function
	    KORO_PUSH(rtk->impl_outs);
	    KORO_PUSH(rtk->impl_ins);
	    KORO_PUSH(rtk->state);
	    KORO_PUSH(rtk->rt_state);
    
	    // push dummy return address
	    KORO_PUSH(-1);

	    // push dummy callee-save regs
	    KORO_PUSH(0);
	    KORO_PUSH(0);
	    KORO_PUSH(0);
	    KORO_PUSH(0);

	    rtk->workfn = koro_workfn_32;
#else
            // 64-bit
	    // push dummy return address
	    KORO_PUSH(-1);

	    // push dummy callee-save regs
	    KORO_PUSH(0);
	    KORO_PUSH(0);
	    KORO_PUSH(0);
	    KORO_PUSH(0);
	    KORO_PUSH(0);
	    KORO_PUSH(0);

	    rtk->workfn = koro_workfn_64;
#endif
	}
	else {
	    //errs() << "TBB: " << rtk->work->getName() << "\n";

	    assert(rtk->is_fixed_rate);
	    SKIRRuntime::addSKIROuterLoopPass(rtk, "nocheck", "nocheck");
	    SKIRRuntime::addLLVMOpts(rtk);
	    SKIRRuntime::addSKIRStreamOptsPass(rtk);
	    SKIRRuntime::addSKIRInlineStreamsPass(rtk);
	    SKIRRuntime::addLLVMOpts(rtk);

	    sg->codeGenKernel(rtk);
	}
    }
    else /* else rtk->opt_only */ {

	SKIRRuntime::addSKIRStreamOptsPass(rtk);
	SKIRRuntime::addSKIRInlineStreamsPass(rtk);
	SKIRRuntime::addLLVMOpts(rtk);
	
	sg->codeGenKernel(rtk);
    }

    assert(rtk->workfn);

    double t_end;
    rdtod(t_end);
    rtk->total_jit_time += (t_end - t_begin);
}


void
SKIRKoroSched::run()
{
    SKIRRuntimeKernel *next_rtk = 0;
    while (running == 1)
    {
	kernel_t *k = 0;
	SKIRRuntimeKernel *rtk = next_rtk;
	next_rtk = 0;
	if (!rtk) runq.try_pop(rtk);

	if (rtk) {
	    // make sure k doesn't reference garbage (e.g. removed kernel)
	    k = kernel_map_find(rtk);
	    if (!k) continue;

	    kernel_lock_t::scoped_lock lock;
	    if (lock.try_acquire(k->lock)) {
		if (k->is_active()) {
		    // generate workfn if needed
		    if (!k->rt_kernel.workfn) {
			k->rt_kernel.sched->runCodeGen(&k->rt_kernel);
			assert(k->rt_kernel.workfn);
		    }
		    
		    // execute work function until it returns non-zero
		    SKIRRuntimeKernel *b = k->work();
		    //while (b == 0)
		    //b = k->work();

		    if (b == (SKIRRuntimeKernel *)1) {
			k->done();
			kernel_map_insert(rtk, NULL);
		    } else {
			next_rtk = DisableKoroSteal ? 0 : b;
		    }
		}
		if (!k->is_done() && (next_rtk != rtk))
		    runq.push(rtk);
	    } else {
		// if there's only one kernel in the scheduler,
		// we'll only not get the lock if the kernel is paused,
		// so we should yield if this happens
		if (runq.empty()) tbb::this_tbb_thread::yield();
		next_rtk = rtk;
	    }
	} else {
	    // runq is empty, yield
	    tbb::this_tbb_thread::yield();
	}

    } // while (running == 1)
}


}//namespace llvm
