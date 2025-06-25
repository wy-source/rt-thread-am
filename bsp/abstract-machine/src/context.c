#include <am.h>
#include <klib.h>
#include <rtthread.h>

typedef struct {
  void (*entry)(void *);
  void *parameter;
  void (*exit)(void);
  rt_ubase_t to;
  rt_ubase_t from;
  rt_ubase_t to_flag;
} Ee;


static Context* ev_handler(Event e, Context *c) {
  struct rt_thread *current = rt_thread_self();
  Ee *ee = (Ee *)(current->user_data);
  switch (e.event) {
    case EVENT_YIELD: // Handle yield event
      printf("Got event yield %d\n", e.event);
      break;
    case EVENT_IRQ_TIMER: // Handle timer event
      printf("Got event timer %d\n", e.event);
      break;
    default: printf("Unhandled event ID = %d\n", e.event); assert(0);
  }

  if (!ee->to_flag) {
    *(Context **)(ee->from) = c;
  }
  c = *(Context **)(ee->to);

  return c;
}

void __am_cte_init() {
  cte_init(ev_handler);
}

void rt_hw_context_switch_to(rt_ubase_t to) {
  struct rt_thread *current = rt_thread_self();
  rt_ubase_t user_data_tmp = current->user_data;
  rt_ubase_t context_p = *(rt_ubase_t *)to;
  Ee *ee = (Ee *) (context_p + sizeof(Context));
  ee->to = to;
  ee->to_flag = 1;
  current->user_data = (rt_ubase_t) ee;
  yield();
  current->user_data = user_data_tmp;
}

void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to) {
  struct rt_thread *current = rt_thread_self();
  rt_ubase_t user_data_tmp = current->user_data;
  rt_ubase_t context_p = *(rt_ubase_t *)to;
  Ee *ee = (Ee *) (context_p + sizeof(Context));
  ee->to = to;
  ee->from = from;
  ee->to_flag = 0;
  current->user_data = (rt_ubase_t) ee;
  yield();
  current->user_data = user_data_tmp;
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
  ee->from = 0;
  ee->to = 0;
  ee->to_flag = 0;

  stack_area = (Area) { 0, (void *)((uintptr_t)stack & ~(sizeof(uintptr_t)-1))};
  return (rt_uint8_t *)kcontext(stack_area, wrapper, ee);
}
