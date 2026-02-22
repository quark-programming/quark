#include "tty.h"

bool global_is_tty = false;

void init_tty() {
#ifdef _WIN32
    global_is_tty = _isatty(_fileno(stdout));
#elif defined(__unix__) || defined(__MACH__)
    global_is_tty = isatty(STDOUT_FILENO);
#endif
}
