#ifndef ASSERT_H
#define ASSERT_H

#ifdef NDEBUG
#define assert(cond)
#else
extern void abort(void);
#define assert(cond) do { if (!(cond)) { abort(); } } while(0)
#endif

#endif

