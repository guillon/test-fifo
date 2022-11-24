#ifndef _UTILS_H_
#define _UTILS_H_

extern double dtime();
extern int dsleep(double delay);
extern int verbose(int debug, const char *name, int line, const char *msg, ...);

#define VERBOSE(debug, msg, ...) ((void)((debug) > 0 ? verbose(debug, __FILE__, __LINE__, msg, __VA_ARGS__): 0))

#endif
