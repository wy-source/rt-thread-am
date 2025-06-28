#include <am.h>
#include <klib.h>
#include <rtthread.h>

typedef struct {
  void (*entry)(void *);
  void *parameter;
  void (*exit)(void);
} Ee;

typedef struct {
  rt_ubase_t to;
  rt_ubase_t from;
  rt_ubase_t to_flag;
} Ft;

rt_ubase_t to_g;
rt_ubase_t from_g;
rt_ubase_t to_flag_g;

static Context* ev_handler(Event e, Context *c) {
  //struct rt_thread *current = rt_thread_self();
  //Ft *ft = (Ft *)(current->user_data);
  switch (e.event) {
    case EVENT_YIELD: // Handle yield event
      printf("Got event yield %d\n", e.event);
      break;
    case EVENT_IRQ_TIMER: // Handle timer event
      //printf("Got event timer %d\n", e.event);
      break;
    default: printf("Unhandled event ID = %d\n", e.event); assert(0);
  }

  if (!to_flag_g)
    *(Context **)(from_g) = c;
  to_flag_g = 0;
  c = *(Context **)(to_g);
  // if (!ft->to_flag) {
  //   *(Context **)(ft->from) = c;
  // }
  // c = *(Context **)(ft->to);
  // memset(ft, 0, sizeof(Ft)); // restore ft back to stack

  return c;
}

void __am_cte_init() {
  cte_init(ev_handler);
}

void rt_hw_context_switch_to(rt_ubase_t to) {
  to_g = to;
  to_flag_g = 1;
  yield();
  // struct rt_thread *current = rt_thread_self();
  // rt_ubase_t user_data_tmp = current->user_data;
  // rt_ubase_t context_p = *(rt_ubase_t *)to;
  // Ft *ft = (Ft *) (context_p - sizeof(Ft));
  // ft->to = to;
  // ft->to_flag = 1;
  // current->user_data = (rt_ubase_t) ft;
  // yield();
  // current->user_data = user_data_tmp;
}

void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to) {
  to_g = to;
  from_g = from;
  to_flag_g = 0;
  yield();
  // struct rt_thread *current = rt_thread_self();
  // rt_ubase_t user_data_tmp = current->user_data;
  // rt_ubase_t context_p = *(rt_ubase_t *)to;
  // Ft *ft = (Ft *) (context_p - sizeof(Ft));
  // ft->to = to;
  // ft->from = from;
  // ft->to_flag = 0;
  // current->user_data = (rt_ubase_t) ft;
  // yield();
  // current->user_data = user_data_tmp;
}

void rt_hw_context_switch_interrupt(void *context, rt_ubase_t from, rt_ubase_t to, struct rt_thread *to_thread) {
  assert(0);
}

static void wrapper(void *ee) {
  assert(ee != NULL);
  Ee *ee_t = (Ee *)ee;
  ee_t->entry(ee_t->parameter);
  ee_t->exit();
}

rt_uint8_t *rt_hw_stack_init(void *tentry, void *parameter, rt_uint8_t *stack_addr, void *texit) {
  Area stack_area;
  Ee *ee;

  rt_uint8_t *stack = (rt_uint8_t *)((uintptr_t)stack_addr & ~(sizeof(uintptr_t)-1));
  stack -= sizeof(Ee);

  ee = (Ee *)stack;
  ee->entry = tentry;
  ee->exit = texit;
  ee->parameter = parameter;

  stack_area = (Area) { 0, (void *)((uintptr_t)stack & ~(sizeof(uintptr_t)-1))};
  return (rt_uint8_t *)kcontext(stack_area, wrapper, ee);
}
