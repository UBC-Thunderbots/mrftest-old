#ifndef UNUSED_H
#define UNUSED_H

#ifdef __GNUC__
#define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#else
#define UNUSED(x) x
#endif

#endif

