#ifndef STM32LIB_GPIO_H
#define STM32LIB_GPIO_H

/**
 * \file
 *
 * \brief Provides utility functions for working with general purpose I/O ports.
 */

#include <registers/gpio.h>

/**
 * \brief Gets the mode of an I/O pin.
 *
 * \param[in] port the port
 *
 * \param[in] bit the bit
 *
 * \return the mode
 */
#define gpio_get_mode(port, bit) ((GPIO_MODE_t) (((port).MODER >> (2 * (bit))) & GPIO_MODE_MASK))

/**
 * \brief Sets the mode of an I/O pin.
 *
 * \param[in] port the port
 *
 * \param[in] bit the bit
 *
 * \param[in] mode the mode
 */
#define gpio_set_mode(port, bit, mode) \
	do { \
		(port).MODER = ((port).MODER & ~(GPIO_MODE_MASK << (2 * (bit)))) | ((mode) << (2 * (bit))); \
	} while (0)

/**
 * \brief Checks if an I/O pin is push-pull.
 *
 * \param[in] port the port
 *
 * \param[in] bit the bit
 *
 * \return \c true if the port is push-pull, or \c false if open-drain
 */
#define gpio_is_pp(port, bit) ((((port).OTYPER >> (bit)) & 1) == GPIO_OTYPE_PP)

/**
 * \brief Checks if an I/O pin is open-drain.
 *
 * \param[in] port the port
 *
 * \param[in] bit the bit
 *
 * \return \c true if the port is open-drain, or \c false if push-pull
 */
#define gpio_is_od(port, bit) ((((port).OTYPER >> (bit)) & 1) == GPIO_OTYPE_OD)

/**
 * \brief Sets an I/O pin to push-pull mode.
 *
 * \param[in] port the port
 *
 * \param[in] bit the bit
 */
#define gpio_set_pp(port, bit) \
	do { \
		(port).OTYPER = ((port).OTYPER & ~(GPIO_OTYPE_MASK << (bit))) | (GPIO_OTYPE_PP << (bit)); \
	} while (0)

/**
 * \brief Sets an I/O pin to open-drain mode.
 *
 * \param[in] port the port
 *
 * \param[in] bit the bit
 */
#define gpio_set_od(port, bit) \
	do { \
		(port).OTYPER = ((port).OTYPER & ~(GPIO_OTYPE_MASK << (bit))) | (GPIO_OTYPE_OD << (bit)); \
	} while (0)

/**
 * \brief Gets the drive speed of an I/O pin.
 *
 * \param[in] port the port
 *
 * \param[in] bit the bit
 *
 * \return the speed
 */
#define gpio_get_speed(port, bit) ((GPIO_OSPEED_t) (((port).OSPEEDR >> (2 * (bit))) & GPIO_OSPEED_MASK))

/**
 * \brief Sets the drive speed of an I/O pin.
 *
 * \param[in] port the port
 *
 * \param[in] bit the bit
 *
 * \param[in] speed the speed
 */
#define gpio_set_speed(port, bit, speed) \
	do { \
		(port).OSPEEDR = ((port).OSPEEDR & ~(GPIO_SPEED_MASK << (2 * (bit)))) | ((speed) << (2 * (bit))); \
	} while (0)

/**
 * \brief Gets the pull direction of an I/O pin.
 *
 * \param[in] port the port
 *
 * \param[in] bit the bit
 *
 * \return the pull direction
 */
#define gpio_get_pupd(port, bit) ((GPIO_PUPD_t) (((port).PUPDR >> (2 * (bit))) & GPIO_PUPD_MASK));

/**
 * \brief Sets the pull direction of an I/O pin.
 *
 * \param[in] port the port
 *
 * \param[in] bit the bit
 *
 * \param[in] pupd the pull direction
 */
#define gpio_set_pupd(port, bit, pupd) \
	do { \
		(port).PUPDR = ((port).PUPDR & ~(GPIO_PUPD_MASK << (2 * (bit)))) | ((pupd) << (2 * (bit))); \
	} while (0)

/**
 * \brief Gets the alternate function attached to an I/O pin.
 *
 * \param[in] port the port
 *
 * \param[in] bit the bit
 *
 * \return the alternate function
 */
#define gpio_get_af(port, bit) \
	((bit) <= 7 ? \
	(((port).AFRL >> (4 * (bit))) & 15) : \
	(((port).AFRH >> (4 * ((bit) - 8))) & 15))

/**
 * \brief Selects an alternate function on an I/O pin.
 *
 * \param[in] port the port
 *
 * \param[in] bit the bit
 *
 * \param[in] af the alternate function
 */
#define gpio_set_af(port, bit, af) \
	do { \
		unsigned int shift_dist = (bit) <= 7 ? 4 * (bit) : 4 * ((bit) - 8); \
		if ((bit) <= 7) { \
			(port).AFRL = ((port).AFRL & ~(((uint32_t) 16) << shift_dist)) | ((af) << shift_dist); \
		} else { \
			(port).AFRH = ((port).AFRH & ~(((uint32_t) 16) << shift_dist)) | ((af) << shift_dist); \
		} \
	} while (0)

/**
 * \brief Gets the state of an input pin.
 *
 * \param[in] port the port
 *
 * \param[in] bit the bit
 *
 * \return the state, \c true or \c false
 */
#define gpio_get_input(port, bit) (!!((port).IDR & (1 << (bit))))

/**
 * \brief Gets the state of an output driver.
 *
 * \param[in] port the port
 *
 * \param[in] bit the bit
 *
 * \return the state, \c true or \c false
 */
#define gpio_get_output(port, bit) (!!((port).ODR & (1 << (bit))))

/**
 * \brief Inverts an output driver.
 *
 * \param[in] port the port
 *
 * \param[in] bit the bit
 */
#define gpio_toggle(port, bit) \
	do { \
		(port).ODR ^= 1 << (bit); \
	} while (0)

/**
 * \brief Sets and resets a subset of the bits of a port.
 *
 * If a single bit is asked to both set and reset, it is set.
 *
 * \param[in] port the port
 *
 * \param[in] set the bitmask of bits to set
 *
 * \param[in] reset the bitmask of bits to clear
 */
#define gpio_set_reset_mask(port, set, reset) \
	do { \
		GPIO_BSRR_t tmp = { .BS = (set), .BR = (reset) }; \
		(port).BSRR = tmp; \
	} while (0)

/**
 * \brief Resets a single I/O pin.
 *
 * \param[in] port the port
 *
 * \param[in] bit the bit
 */
#define gpio_reset(port, bit) gpio_set_reset_mask(port, 0, 1 << (bit))

/**
 * \brief Sets a single I/O pin.
 *
 * \param[in] port the port
 *
 * \param[in] bit the bit
 */
#define gpio_set(port, bit) gpio_set_reset_mask(port, 1 << (bit), 0)

/**
 * \brief Sets or resets a single I/O pin.
 *
 * \param[in] port the port
 *
 * \param[in] bit the bit
 *
 * \param[in] level \c true to set the pin, or \c false to reset it
 */
#define gpio_set_output(port, bit, level) gpio_set_reset_mask(port, (level) ? (1 << (bit)) : 0, 1 << (bit))

/**
 * \brief Describes the initial state of a single I/O pin.
 */
typedef struct __attribute__((packed)) {
	GPIO_MODE_t mode : 2;
	GPIO_OTYPE_t otype : 1;
	GPIO_OSPEED_t ospeed : 2;
	GPIO_PUPD_t pupd : 2;
	unsigned od : 1;
	unsigned af : 4;
} gpio_init_pin_t;

/**
 * \brief Initializes all I/O ports.
 *
 * Once this function returns, all I/O pins are in their specified states and all ports are enabled in the RCC.
 *
 * \param[in] specs the specifications of the pins, indexed first by port and then by pin
 */
void gpio_init(const gpio_init_pin_t specs[4U][16U]);

#endif

