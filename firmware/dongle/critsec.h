#ifndef CRITSEC_H
#define CRITSEC_H

/**
 * \brief Defines a variable to represent a critical section.
 *
 * \param[in] v the variable to declare.
 */
#define CRITSEC_DECLARE(v) uint8_t v

/**
 * \brief Enters a critical section.
 *
 * \param[in] v the critical section to enter.
 */
#define CRITSEC_ENTER(v) do { (v) = INTCON & 0x80; INTCONbits.GIEH = 0; } while (0)

/**
 * \brief Leaves a critical section.
 *
 * \param[in] v the critical section to leave.
 */
#define CRITSEC_LEAVE(v) do { INTCON |= (v); } while (0)

#endif

