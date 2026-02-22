#ifndef TTY_H
#define TTY_H

#include <stdbool.h>

#ifdef _WIN32
#include <io.h>
#elif defined(__unix__) || defined(__MACH__)
#include <unistd.h>
#endif

extern bool global_is_tty;

void init_tty();

#define iftty(if, else) (global_is_tty ? if : else)

#endif