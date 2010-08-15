#ifndef UTIL_MISC_H
#define UTIL_MISC_H

#include <sys/socket.h>

/**
 * Gets the value of the system constant \c MAP_FAILED without provoking
 * warnings about old C-style casts.
 *
 * \return \c MAP_FAILED.
 */
void *get_map_failed();

/**
 * Executes the system macro \c CMSG_FIRSTHDR without provoking warnings about
 * old C-style casts.
 *
 * \param[in] msgh the ancillary message buffer to search.
 *
 * \return the first ancillary message header in the buffer.
 */
cmsghdr *cmsg_firsthdr(msghdr *msgh);

/**
 * Executes the system macro \c CMSG_SPACE without provoking warnings about old
 * C-style casts.
 *
 * \param[in] length the size of the payload of an ancillary message.
 *
 * \return the amount of space needed to store the ancillary message.
 */
size_t cmsg_space(size_t length);

/**
 * Executes the system macro \c CMSG_LEN without provoking warnings about old
 * C-style casts.
 *
 * \param[in] length the size of the payload of an ancillary message.
 *
 * \return the length to store in the header of the ancillary message.
 */
size_t cmsg_len(size_t length);

/**
 * Executes the system macro \c CMSG_DATA without provoking warnings about old
 * C-style casts, and also converting the return type to <code>void *</code>.
 *
 * \param[in] cmsg the ancillary message to examine.
 *
 * \return a pointer to the start of the payload of the message.
 */
void *cmsg_data(cmsghdr *cmsg);

/**
 * Executes the system macro \c WIFEXITED without provoking warnings about old
 * C-style casts, and also converting the return type to bool.
 *
 * \param[in] status the exist status as returned by \c waitpid().
 *
 * \return \c true if the status indicates that the process terminated normally,
 * or \c false if not.
 */
bool wifexited(int status);

/**
 * Executes the system macro \c WEXITSTATUS without provoking warnings about old
 * C-style casts.
 *
 * \param[in] status the exist status as returned by \c waitpid().
 *
 * \return the exit code from the process.
 */
int wexitstatus(int status);

#endif

