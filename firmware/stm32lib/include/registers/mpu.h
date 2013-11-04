#ifndef STM32LIB_REGISTERS_MPU_H
#define STM32LIB_REGISTERS_MPU_H

/**
 * \file
 *
 * \brief Defines the memory protection unit registers.
 */

#define MPU_BASE 0xE000ED90

typedef struct {
	unsigned SEPARATE : 1;
	unsigned : 7;
	unsigned DREGION : 8;
	unsigned IREGION : 8;
	unsigned : 8;
} MPU_TYPE_t;
#define MPU_TYPE (*(const volatile MPU_TYPE_t *) (MPU_BASE + 0x00))

typedef struct {
	unsigned ENABLE : 1;
	unsigned HFNMIENA : 1;
	unsigned PRIVDEFENA : 1;
	unsigned : 29;
} MPU_CTRL_t;
#define MPU_CTRL (*(volatile MPU_CTRL_t *) (MPU_BASE + 0x04))

typedef struct {
	unsigned REGION : 8;
	unsigned : 24;
} MPU_RNR_t;
#define MPU_RNR (*(volatile MPU_RNR_t *) (MPU_BASE + 0x08))

typedef struct {
	unsigned REGION : 4;
	unsigned VALID : 1;
	unsigned ADDR : 27;
} MPU_RBAR_t;
#define MPU_RBAR (*(volatile MPU_RBAR_t *) (MPU_BASE + 0x0C))

typedef struct {
	unsigned ENABLE : 1;
	unsigned SIZE : 5;
	unsigned : 2;
	unsigned SRD : 8;
	unsigned B : 1;
	unsigned C : 1;
	unsigned S : 1;
	unsigned TEX : 3;
	unsigned : 2;
	unsigned AP : 3;
	unsigned : 1;
	unsigned XN : 1;
	unsigned : 3;
} MPU_RASR_t;
#define MPU_RASR (*(volatile MPU_RASR_t *) (MPU_BASE + 0x10))

#endif

