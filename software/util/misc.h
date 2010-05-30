#ifndef UTIL_MISC_H
#define UTIL_MISC_H

#include <sys/socket.h>

/**
 * Gets the value of the system constant MAP_FAILED without provoking warnings
 * about old C-style casts.
 *
 * \return MAP_FAILED
 */
void *get_map_failed();

/**
 * Executes the system macro CMSG_FIRSTHDR without provoking warnings about old
 * C-style casts.
 */
cmsghdr *cmsg_firsthdr(msghdr *msgh);

/**
 * Executes the system macro CMSG_SPACE without provoking warnings about old
 * C-style casts.
 */
size_t cmsg_space(size_t length);

/**
 * Executes the system macro CMSG_LEN without provoking warnings about old
 * C-style casts.
 */
size_t cmsg_len(size_t length);

/**
 * Executes the system macro CMSG_DATA without provoking warnings about old
 * C-style casts, and also converting the return type to void *.
 */
void *cmsg_data(cmsghdr *cmsg);

/**
 * Executes the system macro WIFEXITED without provoking warnings about old
 * C-style casts, and also converting the return type to bool.
 */
bool wifexited(int status);

/**
 * Executes the system macro WEXITSTATUS without provoking warnings about old
 * C-style casts.
 */
int wexitstatus(int status);

#endif

