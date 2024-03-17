#ifndef SOUL_ARGS_H
#define SOUL_ARGS_H

typedef char *var_list;

#define STACK_SIZE_FN(t) (sizeof(t) <= sizeof(char *) ? sizeof(char *) : sizeof(t))
#define VAR_START_FN(ap, v) (ap = (var_list)&v + sizeof(char *))
#define VAR_ARG_FN(ap, t) (*(t *)((ap += STACK_SIZE_FN(t)) - STACK_SIZE_FN(t)))
#define VAR_END_FN(ap) (ap = (var_list)0)

#endif
