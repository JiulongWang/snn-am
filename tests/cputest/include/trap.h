#ifndef __TRAP_H__
#define __TRAP_H__

#include <am.h>
#include <klib.h>
#include <klib-macros.h>

__attribute__((noinline))
void nemu_assert(int cond) {
  if (!cond) _halt(1);
}

double Exp(double);
double Fabs(double);

#endif
