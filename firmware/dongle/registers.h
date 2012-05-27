#ifndef REGISTERS_H
#define REGISTERS_H

#include "stdint.h"

#define REGISTER(addr) (*(volatile uint32_t *) (addr))

#define FSMC_BASE 0xA0000000
#define FSMC_REG(off) REGISTER(FSMC_BASE + (off))
#define FSMC_BCR1 FSMC_REG(0x000)
#define FSMC_BCR2 FSMC_REG(0x008)
#define FSMC_BCR3 FSMC_REG(0x010)
#define FSMC_BCR4 FSMC_REG(0x018)
#define FSMC_BTR1 FSMC_REG(0x004)
#define FSMC_BTR2 FSMC_REG(0x00C)
#define FSMC_BTR3 FSMC_REG(0x014)
#define FSMC_BTR4 FSMC_REG(0x01C)
#define FSMC_BWTR1 FSMC_REG(0x104)
#define FSMC_BWTR2 FSMC_REG(0x10C)
#define FSMC_BWTR3 FSMC_REG(0x114)
#define FSMC_BWTR4 FSMC_REG(0x11C)
#define FSMC_PCR2 FSMC_REG(0x060)
#define FSMC_PCR3 FSMC_REG(0x080)
#define FSMC_PCR4 FSMC_REG(0x0A0)
#define FSMC_SR2 FSMC_REG(0x064)
#define FSMC_SR3 FSMC_REG(0x084)
#define FSMC_SR4 FSMC_REG(0x0A4)
#define FSMC_PMEM2 FSMC_REG(0x068)
#define FSMC_PMEM3 FSMC_REG(0x088)
#define FSMC_PMEM4 FSMC_REG(0x0A8)
#define FSMC_PATT2 FSMC_REG(0x06C)
#define FSMC_PATT3 FSMC_REG(0x08C)
#define FSMC_PATT4 FSMC_REG(0x0AC)
#define FSMC_PIO4 FSMC_REG(0x0B0)
#define FSMC_ECCR2 FSMC_REG(0x074)
#define FSMC_ECCR3 FSMC_REG(0x094)

#define RNG_BASE 0x50060800
#define RNG_REG(off) REGISTER(RNG_BASE + (off))
#define RNG_CR RNG_REG(0x00)
#define RNG_SR RNG_REG(0x04)
#define RNG_DR RNG_REG(0x08)

#define HASH_BASE 0x50060400
#define HASH_REG(off) REGISTER(HASH_BASE + (off))
#define HASH_CR HASH_REG(0x00)
#define HASH_DIN HASH_REG(0x04)
#define HASH_STR HASH_REG(0x08)
#define HASH_HR0 HASH_REG(0x0C)
#define HASH_HR1 HASH_REG(0x10)
#define HASH_HR2 HASH_REG(0x14)
#define HASH_HR3 HASH_REG(0x18)
#define HASH_HR4 HASH_REG(0x1C)
#define HASH_IMR HASH_REG(0x20)
#define HASH_SR HASH_REG(0x24)
#define HASH_CSR0 HASH_REG(0xF8)
#define HASH_CSR_COUNT 51

#define CRYP_BASE 0x50060000
#define CRYP_REG(off) REGISTER(CRYP_BASE + (off))
#define CRYP_CR CRYP_REG(0x00)
#define CRYP_SR CRYP_REG(0x04)
#define CRYP_DR CRYP_REG(0x08)
#define CRYP_DOUT CRYP_REG(0x0C)
#define CRYP_DMACR CRYP_REG(0x10)
#define CRYP_IMSCR CRYP_REG(0x14)
#define CRYP_RISR CRYP_REG(0x18)
#define CRYP_MISR CRYP_REG(0x1C)
#define CRYP_K0LR CRYP_REG(0x20)
#define CRYP_K0RR CRYP_REG(0x24)
#define CRYP_K1LR CRYP_REG(0x28)
#define CRYP_K1RR CRYP_REG(0x2C)
#define CRYP_K2LR CRYP_REG(0x30)
#define CRYP_K2RR CRYP_REG(0x34)
#define CRYP_K3LR CRYP_REG(0x38)
#define CRYP_K3RR CRYP_REG(0x3C)
#define CRYP_IV0LR CRYP_REG(0x40)
#define CRYP_IV0RR CRYP_REG(0x44)
#define CRYP_IV1LR CRYP_REG(0x48)
#define CRYP_IV1RR CRYP_REG(0x4C)

#define DCMI_BASE 0x50050000
#define DCMI_REG(off) REGISTER(DCMI_BASE + (off))
#define DCMI_CR DCMI_REG(0x00)
#define DCMI_SR DCMI_REG(0x04)
#define DCMI_RIS DCMI_REG(0x08)
#define DCMI_IER DCMI_REG(0x0C)
#define DCMI_MIS DCMI_REG(0x10)
#define DCMI_ICR DCMI_REG(0x14)
#define DCMI_ESCR DCMI_REG(0x18)
#define DCMI_ESUR DCMI_REG(0x1C)
#define DCMI_CWSTRT DCMI_REG(0x20)
#define DCMI_CWSIZE DCMI_REG(0x24)
#define DCMI_DR DCMI_REG(0x28)

#define OTG_FS_BASE 0x50000000
#define OTG_FS_REG(off) REGISTER(OTG_FS_BASE + (off))
#define OTG_FS_GOTGCTL OTG_FS_REG(0x000)
#define OTG_FS_GOTGINT OTG_FS_REG(0x004)
#define OTG_FS_GAHBCFG OTG_FS_REG(0x008)
#define OTG_FS_GUSBCFG OTG_FS_REG(0x00C)
#define OTG_FS_GRSTCTL OTG_FS_REG(0x010)
#define OTG_FS_GINTSTS OTG_FS_REG(0x014)
#define OTG_FS_GINTMSK OTG_FS_REG(0x018)
#define OTG_FS_GRXSTSR OTG_FS_REG(0x01C)
#define OTG_FS_GRXSTSP OTG_FS_REG(0x020)
#define OTG_FS_GRXFSIZ OTG_FS_REG(0x024)
#define OTG_FS_HNPTXFSIZ OTG_FS_REG(0x028)
#define OTG_FS_DIEPTXF0 OTG_FS_REG(0x028)
#define OTG_FS_HNPTXSTS OTG_FS_REG(0x02C)
#define OTG_FS_GCCFG OTG_FS_REG(0x038)
#define OTG_FS_CID OTG_FS_REG(0x03C)
#define OTG_FS_HPTXFSIZ OTG_FS_REG(0x100)
#define OTG_FS_DIEPTXF1 OTG_FS_REG(0x104)
#define OTG_FS_DIEPTXF2 OTG_FS_REG(0x108)
#define OTG_FS_DIEPTXF3 OTG_FS_REG(0x10C)
#define OTG_FS_DIEPTXF_I(pipe) OTG_FS_REG(0x100 + (pipe) * 4)
#define OTG_FS_HCFG OTG_FS_REG(0x400)
#define OTG_FS_HFIR OTG_FS_REG(0x404)
#define OTG_FS_HFNUM OTG_FS_REG(0x408)
#define OTG_FS_HPTXSTS OTG_FS_REG(0x410)
#define OTG_FS_HAINT OTG_FS_REG(0x414)
#define OTG_FS_HAINTMSK OTG_FS_REG(0x418)
#define OTG_FS_HPRT OTG_FS_REG(0x440)
#define OTG_FS_HCCHAR0 OTG_FS_REG(0x500)
#define OTG_FS_HCCHAR1 OTG_FS_REG(0x520)
#define OTG_FS_HCCHAR2 OTG_FS_REG(0x540)
#define OTG_FS_HCCHAR3 OTG_FS_REG(0x560)
#define OTG_FS_HCCHAR4 OTG_FS_REG(0x580)
#define OTG_FS_HCCHAR5 OTG_FS_REG(0x5A0)
#define OTG_FS_HCCHAR6 OTG_FS_REG(0x5C0)
#define OTG_FS_HCCHAR7 OTG_FS_REG(0x5E0)
#define OTG_FS_HCINT0 OTG_FS_REG(0x508)
#define OTG_FS_HCINT1 OTG_FS_REG(0x528)
#define OTG_FS_HCINT2 OTG_FS_REG(0x548)
#define OTG_FS_HCINT3 OTG_FS_REG(0x568)
#define OTG_FS_HCINT4 OTG_FS_REG(0x588)
#define OTG_FS_HCINT5 OTG_FS_REG(0x5A8)
#define OTG_FS_HCINT6 OTG_FS_REG(0x5C8)
#define OTG_FS_HCINT7 OTG_FS_REG(0x5E8)
#define OTG_FS_HCINTMSK0 OTG_FS_REG(0x50C)
#define OTG_FS_HCINTMSK1 OTG_FS_REG(0x52C)
#define OTG_FS_HCINTMSK2 OTG_FS_REG(0x54C)
#define OTG_FS_HCINTMSK3 OTG_FS_REG(0x56C)
#define OTG_FS_HCINTMSK4 OTG_FS_REG(0x58C)
#define OTG_FS_HCINTMSK5 OTG_FS_REG(0x5AC)
#define OTG_FS_HCINTMSK6 OTG_FS_REG(0x5CC)
#define OTG_FS_HCINTMSK7 OTG_FS_REG(0x5EC)
#define OTG_FS_HCTSIZ0 OTG_FS_REG(0x510)
#define OTG_FS_HCTSIZ1 OTG_FS_REG(0x530)
#define OTG_FS_HCTSIZ2 OTG_FS_REG(0x550)
#define OTG_FS_HCTSIZ3 OTG_FS_REG(0x570)
#define OTG_FS_HCTSIZ4 OTG_FS_REG(0x590)
#define OTG_FS_HCTSIZ5 OTG_FS_REG(0x5B0)
#define OTG_FS_HCTSIZ6 OTG_FS_REG(0x5D0)
#define OTG_FS_HCTSIZ7 OTG_FS_REG(0x5F0)
#define OTG_FS_DCFG OTG_FS_REG(0x800)
#define OTG_FS_DCTL OTG_FS_REG(0x804)
#define OTG_FS_DSTS OTG_FS_REG(0x808)
#define OTG_FS_DIEPMSK OTG_FS_REG(0x810)
#define OTG_FS_DOEPMSK OTG_FS_REG(0x814)
#define OTG_FS_DAINT OTG_FS_REG(0x818)
#define OTG_FS_DAINTMSK OTG_FS_REG(0x81C)
#define OTG_FS_DVBUSDIS OTG_FS_REG(0x828)
#define OTG_FS_DVBUSPULSE OTG_FS_REG(0x82C)
#define OTG_FS_DIEPEMPMSK OTG_FS_REG(0x834)
#define OTG_FS_DIEPCTL0 OTG_FS_REG(0x900)
#define OTG_FS_DIEPCTL1 OTG_FS_REG(0x920)
#define OTG_FS_DIEPCTL2 OTG_FS_REG(0x940)
#define OTG_FS_DIEPCTL3 OTG_FS_REG(0x960)
#define OTG_FS_DIEPCTL_I(pipe) OTG_FS_REG(0x900 + (pipe) * 0x20)
#define OTG_FS_DTXFSTS0 OTG_FS_REG(0x918)
#define OTG_FS_DTXFSTS1 OTG_FS_REG(0x938)
#define OTG_FS_DTXFSTS2 OTG_FS_REG(0x958)
#define OTG_FS_DTXFSTS3 OTG_FS_REG(0x978)
#define OTG_FS_DTXFSTS_I(pipe) OTG_FS_REG(0x918 + (pipe) * 0x20)
#define OTG_FS_DOEPCTL0 OTG_FS_REG(0xB00)
#define OTG_FS_DOEPCTL1 OTG_FS_REG(0xB20)
#define OTG_FS_DOEPCTL2 OTG_FS_REG(0xB40)
#define OTG_FS_DOEPCTL3 OTG_FS_REG(0xB60)
#define OTG_FS_DOEPCTL_I(pipe) OTG_FS_REG(0xB00 + (pipe) * 0x20)
#define OTG_FS_DIEPINT0 OTG_FS_REG(0x908)
#define OTG_FS_DIEPINT1 OTG_FS_REG(0x928)
#define OTG_FS_DIEPINT2 OTG_FS_REG(0x948)
#define OTG_FS_DIEPINT3 OTG_FS_REG(0x968)
#define OTG_FS_DIEPINT_I(pipe) OTG_FS_REG(0x908 + (pipe) * 0x20)
#define OTG_FS_DOEPINT0 OTG_FS_REG(0xB08)
#define OTG_FS_DOEPINT1 OTG_FS_REG(0xB28)
#define OTG_FS_DOEPINT2 OTG_FS_REG(0xB48)
#define OTG_FS_DOEPINT3 OTG_FS_REG(0xB68)
#define OTG_FS_DOEPINT_I(pipe) OTG_FS_REG(0xB08 + (pipe) * 0x20)
#define OTG_FS_DIEPTSIZ0 OTG_FS_REG(0x910)
#define OTG_FS_DIEPTSIZ1 OTG_FS_REG(0x930)
#define OTG_FS_DIEPTSIZ2 OTG_FS_REG(0x950)
#define OTG_FS_DIEPTSIZ3 OTG_FS_REG(0x970)
#define OTG_FS_DIEPTSIZ_I(pipe) OTG_FS_REG(0x910 + (pipe) * 0x20)
#define OTG_FS_DOEPTSIZ0 OTG_FS_REG(0xB10)
#define OTG_FS_DOEPTSIZ1 OTG_FS_REG(0xB30)
#define OTG_FS_DOEPTSIZ2 OTG_FS_REG(0xB50)
#define OTG_FS_DOEPTSIZ3 OTG_FS_REG(0xB70)
#define OTG_FS_DOEPTSIZ_I(pipe) OTG_FS_REG(0xB10 + (pipe) * 0x20)
#define OTG_FS_PCGCCTL OTG_FS_REG(0xE00)

#define OTG_HS_BASE 0x40040000
#define OTG_HS_REG(off) REGISTER(OTG_HS_BASE + (off))

#define DMA2_BASE 0x40026400
#define DMA2_REG(off) REGISTER(DMA2_BASE + (off))
#define DMA2_LISR DMA2_REG(0x00)
#define DMA2_HISR DMA2_REG(0x04)
#define DMA2_LIFCR DMA2_REG(0x08)
#define DMA2_HIFCR DMA2_REG(0x0C)
#define DMA2_S0CR DMA2_REG(0x10)
#define DMA2_S0NDTR DMA2_REG(0x14)
#define DMA2_S0PAR DMA2_REG(0x18)
#define DMA2_S0M0AR DMA2_REG(0x1C)
#define DMA2_S0M1AR DMA2_REG(0x20)
#define DMA2_S0FCR DMA2_REG(0x24)
#define DMA2_S1CR DMA2_REG(0x28)
#define DMA2_S1NDTR DMA2_REG(0x2C)
#define DMA2_S1PAR DMA2_REG(0x30)
#define DMA2_S1M0AR DMA2_REG(0x34)
#define DMA2_S1M1AR DMA2_REG(0x38)
#define DMA2_S1FCR DMA2_REG(0x3C)
#define DMA2_S2CR DMA2_REG(0x40)
#define DMA2_S2NDTR DMA2_REG(0x44)
#define DMA2_S2PAR DMA2_REG(0x48)
#define DMA2_S2M0AR DMA2_REG(0x4C)
#define DMA2_S2M1AR DMA2_REG(0x50)
#define DMA2_S2FCR DMA2_REG(0x54)
#define DMA2_S3CR DMA2_REG(0x58)
#define DMA2_S3NDTR DMA2_REG(0x5C)
#define DMA2_S3PAR DMA2_REG(0x60)
#define DMA2_S3M0AR DMA2_REG(0x64)
#define DMA2_S3M1AR DMA2_REG(0x68)
#define DMA2_S3FCR DMA2_REG(0x6C)
#define DMA2_S4CR DMA2_REG(0x70)
#define DMA2_S4NDTR DMA2_REG(0x74)
#define DMA2_S4PAR DMA2_REG(0x78)
#define DMA2_S4M0AR DMA2_REG(0x7C)
#define DMA2_S4M1AR DMA2_REG(0x80)
#define DMA2_S4FCR DMA2_REG(0x84)
#define DMA2_S5CR DMA2_REG(0x88)
#define DMA2_S5NDTR DMA2_REG(0x8C)
#define DMA2_S5PAR DMA2_REG(0x90)
#define DMA2_S5M0AR DMA2_REG(0x94)
#define DMA2_S5M1AR DMA2_REG(0x98)
#define DMA2_S5FCR DMA2_REG(0x9C)
#define DMA2_S6CR DMA2_REG(0xA0)
#define DMA2_S6NDTR DMA2_REG(0xA4)
#define DMA2_S6PAR DMA2_REG(0xA8)
#define DMA2_S6M0AR DMA2_REG(0xAC)
#define DMA2_S6M1AR DMA2_REG(0xB0)
#define DMA2_S6FCR DMA2_REG(0xB4)
#define DMA2_S7CR DMA2_REG(0xB8)
#define DMA2_S7NDTR DMA2_REG(0xBC)
#define DMA2_S7PAR DMA2_REG(0xC0)
#define DMA2_S7M0AR DMA2_REG(0xC4)
#define DMA2_S7M1AR DMA2_REG(0xC8)
#define DMA2_S7FCR DMA2_REG(0xCC)

#define DMA1_BASE 0x40026000
#define DMA1_REG(off) REGISTER(DMA1_BASE + (off))
#define DMA1_LISR DMA1_REG(0x00)
#define DMA1_HISR DMA1_REG(0x04)
#define DMA1_LIFCR DMA1_REG(0x08)
#define DMA1_HIFCR DMA1_REG(0x0C)
#define DMA1_S0CR DMA1_REG(0x10)
#define DMA1_S0NDTR DMA1_REG(0x14)
#define DMA1_S0PAR DMA1_REG(0x18)
#define DMA1_S0M0AR DMA1_REG(0x1C)
#define DMA1_S0M1AR DMA1_REG(0x20)
#define DMA1_S0FCR DMA1_REG(0x24)
#define DMA1_S1CR DMA1_REG(0x28)
#define DMA1_S1NDTR DMA1_REG(0x2C)
#define DMA1_S1PAR DMA1_REG(0x30)
#define DMA1_S1M0AR DMA1_REG(0x34)
#define DMA1_S1M1AR DMA1_REG(0x38)
#define DMA1_S1FCR DMA1_REG(0x3C)
#define DMA1_S2CR DMA1_REG(0x40)
#define DMA1_S2NDTR DMA1_REG(0x44)
#define DMA1_S2PAR DMA1_REG(0x48)
#define DMA1_S2M0AR DMA1_REG(0x4C)
#define DMA1_S2M1AR DMA1_REG(0x50)
#define DMA1_S2FCR DMA1_REG(0x54)
#define DMA1_S3CR DMA1_REG(0x58)
#define DMA1_S3NDTR DMA1_REG(0x5C)
#define DMA1_S3PAR DMA1_REG(0x60)
#define DMA1_S3M0AR DMA1_REG(0x64)
#define DMA1_S3M1AR DMA1_REG(0x68)
#define DMA1_S3FCR DMA1_REG(0x6C)
#define DMA1_S4CR DMA1_REG(0x70)
#define DMA1_S4NDTR DMA1_REG(0x74)
#define DMA1_S4PAR DMA1_REG(0x78)
#define DMA1_S4M0AR DMA1_REG(0x7C)
#define DMA1_S4M1AR DMA1_REG(0x80)
#define DMA1_S4FCR DMA1_REG(0x84)
#define DMA1_S5CR DMA1_REG(0x88)
#define DMA1_S5NDTR DMA1_REG(0x8C)
#define DMA1_S5PAR DMA1_REG(0x90)
#define DMA1_S5M0AR DMA1_REG(0x94)
#define DMA1_S5M1AR DMA1_REG(0x98)
#define DMA1_S5FCR DMA1_REG(0x9C)
#define DMA1_S6CR DMA1_REG(0xA0)
#define DMA1_S6NDTR DMA1_REG(0xA4)
#define DMA1_S6PAR DMA1_REG(0xA8)
#define DMA1_S6M0AR DMA1_REG(0xAC)
#define DMA1_S6M1AR DMA1_REG(0xB0)
#define DMA1_S6FCR DMA1_REG(0xB4)
#define DMA1_S7CR DMA1_REG(0xB8)
#define DMA1_S7NDTR DMA1_REG(0xBC)
#define DMA1_S7PAR DMA1_REG(0xC0)
#define DMA1_S7M0AR DMA1_REG(0xC4)
#define DMA1_S7M1AR DMA1_REG(0xC8)
#define DMA1_S7FCR DMA1_REG(0xCC)

#define FLASH_BASE 0x40023C00
#define FLASH_REG(off) REGISTER(FLASH_BASE + (off))
#define FLASH_ACR FLASH_REG(0x00)
#define FLASH_KEYR FLASH_REG(0x04)
#define FLASH_OPTKEYR FLASH_REG(0x08)
#define FLASH_SR FLASH_REG(0x0C)
#define FLASH_CR FLASH_REG(0x10)
#define FLASH_OPTCR FLASH_REG(0x14)

#define RCC_BASE 0x40023800
#define RCC_REG(off) REGISTER(RCC_BASE + (off))
#define RCC_CR RCC_REG(0x00)
#define RCC_PLLCFGR RCC_REG(0x04)
#define RCC_CFGR RCC_REG(0x08)
#define RCC_CIR RCC_REG(0x0C)
#define RCC_AHB1RSTR RCC_REG(0x10)
#define RCC_AHB2RSTR RCC_REG(0x14)
#define RCC_AHB3RSTR RCC_REG(0x18)
#define RCC_APB1RSTR RCC_REG(0x20)
#define RCC_APB2RSTR RCC_REG(0x24)
#define RCC_AHB1ENR RCC_REG(0x30)
#define RCC_AHB2ENR RCC_REG(0x34)
#define RCC_AHB3ENR RCC_REG(0x38)
#define RCC_APB1ENR RCC_REG(0x40)
#define RCC_APB2ENR RCC_REG(0x44)
#define RCC_AHB1LPENR RCC_REG(0x50)
#define RCC_AHB2LPENR RCC_REG(0x54)
#define RCC_AHB3LPENR RCC_REG(0x58)
#define RCC_APB1LPENR RCC_REG(0x60)
#define RCC_APB2LPENR RCC_REG(0x64)
#define RCC_BDCR RCC_REG(0x70)
#define RCC_CSR RCC_REG(0x74)
#define RCC_SSCGR RCC_REG(0x80)
#define RCC_PLLI2SCFGR RCC_REG(0x84)

#define CRC_BASE 0x40023000
#define CRC_REG(off) REGISTER(CRC_BASE + (off))
#define CRC_DR CRC_REG(0x00)
#define CRC_IDR CRC_REG(0x04)
#define CRC_CR CRC_REG(0x08)

#define GPIOI_BASE 0x40022000
#define GPIOI_REG(off) REGISTER(GPIOI_BASE + (off))
#define GPIOI_MODER GPIOI_REG(0x00)
#define GPIOI_OTYPER GPIOI_REG(0x04)
#define GPIOI_OSPEEDR GPIOI_REG(0x08)
#define GPIOI_PUPDR GPIOI_REG(0x0C)
#define GPIOI_IDR GPIOI_REG(0x10)
#define GPIOI_ODR GPIOI_REG(0x14)
#define GPIOI_BSRR GPIOI_REG(0x18)
#define GPIOI_LCKR GPIOI_REG(0x1C)
#define GPIOI_AFRL GPIOI_REG(0x20)
#define GPIOI_AFRH GPIOI_REG(0x24)

#define GPIOH_BASE 0x40021C00
#define GPIOH_REG(off) REGISTER(GPIOH_BASE + (off))
#define GPIOH_MODER GPIOH_REG(0x00)
#define GPIOH_OTYPER GPIOH_REG(0x04)
#define GPIOH_OSPEEDR GPIOH_REG(0x08)
#define GPIOH_PUPDR GPIOH_REG(0x0C)
#define GPIOH_IDR GPIOH_REG(0x10)
#define GPIOH_ODR GPIOH_REG(0x14)
#define GPIOH_BSRR GPIOH_REG(0x18)
#define GPIOH_LCKR GPIOH_REG(0x1C)
#define GPIOH_AFRL GPIOH_REG(0x20)
#define GPIOH_AFRH GPIOH_REG(0x24)

#define GPIOG_BASE 0x40021800
#define GPIOG_REG(off) REGISTER(GPIOG_BASE + (off))
#define GPIOG_MODER GPIOG_REG(0x00)
#define GPIOG_OTYPER GPIOG_REG(0x04)
#define GPIOG_OSPEEDR GPIOG_REG(0x08)
#define GPIOG_PUPDR GPIOG_REG(0x0C)
#define GPIOG_IDR GPIOG_REG(0x10)
#define GPIOG_ODR GPIOG_REG(0x14)
#define GPIOG_BSRR GPIOG_REG(0x18)
#define GPIOG_LCKR GPIOG_REG(0x1C)
#define GPIOG_AFRL GPIOG_REG(0x20)
#define GPIOG_AFRH GPIOG_REG(0x24)

#define GPIOF_BASE 0x40021400
#define GPIOF_REG(off) REGISTER(GPIOF_BASE + (off))
#define GPIOF_MODER GPIOF_REG(0x00)
#define GPIOF_OTYPER GPIOF_REG(0x04)
#define GPIOF_OSPEEDR GPIOF_REG(0x08)
#define GPIOF_PUPDR GPIOF_REG(0x0C)
#define GPIOF_IDR GPIOF_REG(0x10)
#define GPIOF_ODR GPIOF_REG(0x14)
#define GPIOF_BSRR GPIOF_REG(0x18)
#define GPIOF_LCKR GPIOF_REG(0x1C)
#define GPIOF_AFRL GPIOF_REG(0x20)
#define GPIOF_AFRH GPIOF_REG(0x24)

#define GPIOE_BASE 0x40021000
#define GPIOE_REG(off) REGISTER(GPIOE_BASE + (off))
#define GPIOE_MODER GPIOE_REG(0x00)
#define GPIOE_OTYPER GPIOE_REG(0x04)
#define GPIOE_OSPEEDR GPIOE_REG(0x08)
#define GPIOE_PUPDR GPIOE_REG(0x0C)
#define GPIOE_IDR GPIOE_REG(0x10)
#define GPIOE_ODR GPIOE_REG(0x14)
#define GPIOE_BSRR GPIOE_REG(0x18)
#define GPIOE_LCKR GPIOE_REG(0x1C)
#define GPIOE_AFRL GPIOE_REG(0x20)
#define GPIOE_AFRH GPIOE_REG(0x24)

#define GPIOD_BASE 0x40020C00
#define GPIOD_REG(off) REGISTER(GPIOD_BASE + (off))
#define GPIOD_MODER GPIOD_REG(0x00)
#define GPIOD_OTYPER GPIOD_REG(0x04)
#define GPIOD_OSPEEDR GPIOD_REG(0x08)
#define GPIOD_PUPDR GPIOD_REG(0x0C)
#define GPIOD_IDR GPIOD_REG(0x10)
#define GPIOD_ODR GPIOD_REG(0x14)
#define GPIOD_BSRR GPIOD_REG(0x18)
#define GPIOD_LCKR GPIOD_REG(0x1C)
#define GPIOD_AFRL GPIOD_REG(0x20)
#define GPIOD_AFRH GPIOD_REG(0x24)

#define GPIOC_BASE 0x40020800
#define GPIOC_REG(off) REGISTER(GPIOC_BASE + (off))
#define GPIOC_MODER GPIOC_REG(0x00)
#define GPIOC_OTYPER GPIOC_REG(0x04)
#define GPIOC_OSPEEDR GPIOC_REG(0x08)
#define GPIOC_PUPDR GPIOC_REG(0x0C)
#define GPIOC_IDR GPIOC_REG(0x10)
#define GPIOC_ODR GPIOC_REG(0x14)
#define GPIOC_BSRR GPIOC_REG(0x18)
#define GPIOC_LCKR GPIOC_REG(0x1C)
#define GPIOC_AFRL GPIOC_REG(0x20)
#define GPIOC_AFRH GPIOC_REG(0x24)

#define GPIOB_BASE 0x40020400
#define GPIOB_REG(off) REGISTER(GPIOB_BASE + (off))
#define GPIOB_MODER GPIOB_REG(0x00)
#define GPIOB_OTYPER GPIOB_REG(0x04)
#define GPIOB_OSPEEDR GPIOB_REG(0x08)
#define GPIOB_PUPDR GPIOB_REG(0x0C)
#define GPIOB_IDR GPIOB_REG(0x10)
#define GPIOB_ODR GPIOB_REG(0x14)
#define GPIOB_BSRR GPIOB_REG(0x18)
#define GPIOB_LCKR GPIOB_REG(0x1C)
#define GPIOB_AFRL GPIOB_REG(0x20)
#define GPIOB_AFRH GPIOB_REG(0x24)

#define GPIOA_BASE 0x40020000
#define GPIOA_REG(off) REGISTER(GPIOA_BASE + (off))
#define GPIOA_MODER GPIOA_REG(0x00)
#define GPIOA_OTYPER GPIOA_REG(0x04)
#define GPIOA_OSPEEDR GPIOA_REG(0x08)
#define GPIOA_PUPDR GPIOA_REG(0x0C)
#define GPIOA_IDR GPIOA_REG(0x10)
#define GPIOA_ODR GPIOA_REG(0x14)
#define GPIOA_BSRR GPIOA_REG(0x18)
#define GPIOA_LCKR GPIOA_REG(0x1C)
#define GPIOA_AFRL GPIOA_REG(0x20)
#define GPIOA_AFRH GPIOA_REG(0x24)

#define TIM11_BASE 0x40014800
#define TIM11_REG(off) REGISTER(TIM11_BASE + (off))
#define TIM11_CR1 TIM11_REG(0x00)
#define TIM11_SMCR TIM11_REG(0x08)
#define TIM11_DIER TIM11_REG(0x0C)
#define TIM11_SR TIM11_REG(0x10)
#define TIM11_EGR TIM11_REG(0x14)
#define TIM11_CCMR1 TIM11_REG(0x18)
#define TIM11_CCER TIM11_REG(0x20)
#define TIM11_CNT TIM11_REG(0x24)
#define TIM11_PSC TIM11_REG(0x28)
#define TIM11_ARR TIM11_REG(0x2C)
#define TIM11_CCR1 TIM11_REG(0x34)
#define TIM11_OR TIM11_REG(0x50)

#define TIM10_BASE 0x40014400
#define TIM10_REG(off) REGISTER(TIM10_BASE + (off))
#define TIM10_CR1 TIM10_REG(0x00)
#define TIM10_SMCR TIM10_REG(0x08)
#define TIM10_DIER TIM10_REG(0x0C)
#define TIM10_SR TIM10_REG(0x10)
#define TIM10_EGR TIM10_REG(0x14)
#define TIM10_CCMR1 TIM10_REG(0x18)
#define TIM10_CCER TIM10_REG(0x20)
#define TIM10_CNT TIM10_REG(0x24)
#define TIM10_PSC TIM10_REG(0x28)
#define TIM10_ARR TIM10_REG(0x2C)
#define TIM10_CCR1 TIM10_REG(0x34)

#define TIM9_BASE 0x40014000
#define TIM9_REG(off) REGISTER(TIM9_BASE + (off))
#define TIM9_CR1 TIM9_REG(0x00)
#define TIM9_CR2 TIM9_REG(0x04)
#define TIM9_SMCR TIM9_REG(0x08)
#define TIM9_DIER TIM9_REG(0x0C)
#define TIM9_SR TIM9_REG(0x10)
#define TIM9_EGR TIM9_REG(0x14)
#define TIM9_CCMR1 TIM9_REG(0x18)
#define TIM9_CCER TIM9_REG(0x20)
#define TIM9_CNT TIM9_REG(0x24)
#define TIM9_PSC TIM9_REG(0x28)
#define TIM9_ARR TIM9_REG(0x2C)
#define TIM9_CCR1 TIM9_REG(0x34)
#define TIM9_CCR2 TIM9_REG(0x38)

#define EXTI_BASE 0x40013C00
#define EXTI_REG(off) REGISTER(EXTI_BASE + (off))
#define EXTI_IMR EXTI_REG(0x00)
#define EXTI_EMR EXTI_REG(0x04)
#define EXTI_RTSR EXTI_REG(0x08)
#define EXTI_FTSR EXTI_REG(0x0C)
#define EXTI_SWIER EXTI_REG(0x10)
#define EXTI_PR EXTI_REG(0x14)

#define SYSCFG_BASE 0x40013800
#define SYSCFG_REG(off) REGISTER(SYSCFG_BASE + (off))
#define SYSCFG_MEMRMP SYSCFG_REG(0x00)
#define SYSCFG_PMC SYSCFG_REG(0x04)
#define SYSCFG_EXTICR1 SYSCFG_REG(0x08)
#define SYSCFG_EXTICR2 SYSCFG_REG(0x0C)
#define SYSCFG_EXTICR3 SYSCFG_REG(0x10)
#define SYSCFG_EXTICR4 SYSCFG_REG(0x14)
#define SYSCFG_CMPCR SYSCFG_REG(0x20)

#define SPI1_BASE 0x40013000
#define SPI1_REG(off) REGISTER(SPI1_BASE + (off))
#define SPI1_CR1 SPI1_REG(0x00)
#define SPI1_CR2 SPI1_REG(0x04)
#define SPI1_SR SPI1_REG(0x08)
#define SPI1_DR SPI1_REG(0x0C)
#define SPI1_CRCPR SPI1_REG(0x10)
#define SPI1_RXCRCR SPI1_REG(0x14)
#define SPI1_TXCRCR SPI1_REG(0x18)
#define SPI1_I2SCFGR SPI1_REG(0x1C)
#define SPI1_I2SPR SPI1_REG(0x20)

#define SDIO_BASE 0x40012C00
#define SDIO_REG(off) REGISTER(SDIO_BASE + (off))
#define SDIO_POWER SDIO_REG(0x00)
#define SDIO_CLKCR SDIO_REG(0x04)
#define SDIO_ARG SDIO_REG(0x08)
#define SDIO_CMD SDIO_REG(0x0C)
#define SDIO_RESPCMD SDIO_REG(0x10)
#define SDIO_RESP1 SDIO_REG(0x14)
#define SDIO_RESP2 SDIO_REG(0x18)
#define SDIO_RESP3 SDIO_REG(0x1C)
#define SDIO_RESP4 SDIO_REG(0x20)
#define SDIO_DTIMER SDIO_REG(0x24)
#define SDIO_DLEN SDIO_REG(0x28)
#define SDIO_DCTRL SDIO_REG(0x2C)
#define SDIO_DCOUNT SDIO_REG(0x30)
#define SDIO_STA SDIO_REG(0x34)
#define SDIO_ICR SDIO_REG(0x38)
#define SDIO_MASK SDIO_REG(0x3C)
#define SDIO_FIFOCNT SDIO_REG(0x48)
#define SDIO_FIFO SDIO_REG(0x80)

#define ADC_GROUP_BASE 0x40012000

#define ADC1_BASE (ADC_GROUP_BASE)
#define ADC1_REG(off) REGISTER(ADC1_BASE + (off))
#define ADC1_SR ADC1_REG(0x00)
#define ADC1_CR1 ADC1_REG(0x04)
#define ADC1_CR2 ADC1_REG(0x08)
#define ADC1_SMPR1 ADC1_REG(0x0C)
#define ADC1_SMPR2 ADC1_REG(0x10)
#define ADC1_JOFR1 ADC1_REG(0x14)
#define ADC1_JOFR2 ADC1_REG(0x18)
#define ADC1_JOFR3 ADC1_REG(0x1C)
#define ADC1_JOFR4 ADC1_REG(0x20)
#define ADC1_HTR ADC1_REG(0x24)
#define ADC1_LTR ADC1_REG(0x28)
#define ADC1_SQR1 ADC1_REG(0x2C)
#define ADC1_SQR2 ADC1_REG(0x30)
#define ADC1_SQR3 ADC1_REG(0x34)
#define ADC1_JSQR ADC1_REG(0x38)
#define ADC1_JDR1 ADC1_REG(0x3C)
#define ADC1_JDR2 ADC1_REG(0x40)
#define ADC1_JDR3 ADC1_REG(0x44)
#define ADC1_JDR4 ADC1_REG(0x48)
#define ADC1_DR ADC1_REG(0x4C)

#define ADC2_BASE (ADC_GROUP_BASE + 0x100)
#define ADC2_REG(off) REGISTER(ADC2_BASE + (off))
#define ADC2_SR ADC2_REG(0x00)
#define ADC2_CR1 ADC2_REG(0x04)
#define ADC2_CR2 ADC2_REG(0x08)
#define ADC2_SMPR1 ADC2_REG(0x0C)
#define ADC2_SMPR2 ADC2_REG(0x10)
#define ADC2_JOFR1 ADC2_REG(0x14)
#define ADC2_JOFR2 ADC2_REG(0x18)
#define ADC2_JOFR3 ADC2_REG(0x1C)
#define ADC2_JOFR4 ADC2_REG(0x20)
#define ADC2_HTR ADC2_REG(0x24)
#define ADC2_LTR ADC2_REG(0x28)
#define ADC2_SQR1 ADC2_REG(0x2C)
#define ADC2_SQR2 ADC2_REG(0x30)
#define ADC2_SQR3 ADC2_REG(0x34)
#define ADC2_JSQR ADC2_REG(0x38)
#define ADC2_JDR1 ADC2_REG(0x3C)
#define ADC2_JDR2 ADC2_REG(0x40)
#define ADC2_JDR3 ADC2_REG(0x44)
#define ADC2_JDR4 ADC2_REG(0x48)
#define ADC2_DR ADC2_REG(0x4C)

#define ADC3_BASE (ADC_GROUP_BASE + 0x200)
#define ADC3_REG(off) REGISTER(ADC3_BASE + (off))
#define ADC3_SR ADC3_REG(0x00)
#define ADC3_CR1 ADC3_REG(0x04)
#define ADC3_CR2 ADC3_REG(0x08)
#define ADC3_SMPR1 ADC3_REG(0x0C)
#define ADC3_SMPR2 ADC3_REG(0x10)
#define ADC3_JOFR1 ADC3_REG(0x14)
#define ADC3_JOFR2 ADC3_REG(0x18)
#define ADC3_JOFR3 ADC3_REG(0x1C)
#define ADC3_JOFR4 ADC3_REG(0x20)
#define ADC3_HTR ADC3_REG(0x24)
#define ADC3_LTR ADC3_REG(0x28)
#define ADC3_SQR1 ADC3_REG(0x2C)
#define ADC3_SQR2 ADC3_REG(0x30)
#define ADC3_SQR3 ADC3_REG(0x34)
#define ADC3_JSQR ADC3_REG(0x38)
#define ADC3_JDR1 ADC3_REG(0x3C)
#define ADC3_JDR2 ADC3_REG(0x40)
#define ADC3_JDR3 ADC3_REG(0x44)
#define ADC3_JDR4 ADC3_REG(0x48)
#define ADC3_DR ADC3_REG(0x4C)

#define ADC_CBASE (ADC_GROUP_BASE + 0x300)
#define ADC_CREG(off) REGISTER(ADC_COMMON_BASE + (off))
#define ADC_CSR ADC_CREG(0x00)
#define ADC_CCR ADC_CREG(0x04)
#define ADC_CDR ADC_CREG(0x08)

#define USART6_BASE 0x40011400
#define USART6_REG(off) REGISTER(USART6_BASE + (off))
#define USART6_SR USART6_REG(0x00)
#define USART6_DR USART6_REG(0x04)
#define USART6_BRR USART6_REG(0x08)
#define USART6_CR1 USART6_REG(0x0C)
#define USART6_CR2 USART6_REG(0x10)
#define USART6_CR3 USART6_REG(0x14)
#define USART6_GTPR USART6_REG(0x18)

#define USART1_BASE 0x40011000
#define USART1_REG(off) REGISTER(USART1_BASE + (off))
#define USART1_SR USART1_REG(0x00)
#define USART1_DR USART1_REG(0x04)
#define USART1_BRR USART1_REG(0x08)
#define USART1_CR1 USART1_REG(0x0C)
#define USART1_CR2 USART1_REG(0x10)
#define USART1_CR3 USART1_REG(0x14)
#define USART1_GTPR USART1_REG(0x18)

#define TIM8_BASE 0x40010400
#define TIM8_REG(off) REGISTER(TIM8_BASE + (off))
#define TIM8_CR1 TIM8_REG(0x00)
#define TIM8_CR2 TIM8_REG(0x04)
#define TIM8_SMCR TIM8_REG(0x08)
#define TIM8_DIER TIM8_REG(0x0C)
#define TIM8_SR TIM8_REG(0x10)
#define TIM8_EGR TIM8_REG(0x14)
#define TIM8_CCMR1 TIM8_REG(0x18)
#define TIM8_CCMR2 TIM8_REG(0x1C)
#define TIM8_CCER TIM8_REG(0x20)
#define TIM8_CNT TIM8_REG(0x24)
#define TIM8_PSC TIM8_REG(0x28)
#define TIM8_ARR TIM8_REG(0x2C)
#define TIM8_RCR TIM8_REG(0x30)
#define TIM8_CCR1 TIM8_REG(0x34)
#define TIM8_CCR2 TIM8_REG(0x38)
#define TIM8_CCR3 TIM8_REG(0x3C)
#define TIM8_CCR4 TIM8_REG(0x40)
#define TIM8_BDTR TIM8_REG(0x44)
#define TIM8_DCR TIM8_REG(0x48)
#define TIM8_DMAR TIM8_REG(0x4C)

#define TIM1_BASE 0x40010000
#define TIM1_REG(off) REGISTER(TIM1_BASE + (off))
#define TIM1_CR1 TIM1_REG(0x00)
#define TIM1_CR2 TIM1_REG(0x04)
#define TIM1_SMCR TIM1_REG(0x08)
#define TIM1_DIER TIM1_REG(0x0C)
#define TIM1_SR TIM1_REG(0x10)
#define TIM1_EGR TIM1_REG(0x14)
#define TIM1_CCMR1 TIM1_REG(0x18)
#define TIM1_CCMR2 TIM1_REG(0x1C)
#define TIM1_CCER TIM1_REG(0x20)
#define TIM1_CNT TIM1_REG(0x24)
#define TIM1_PSC TIM1_REG(0x28)
#define TIM1_ARR TIM1_REG(0x2C)
#define TIM1_RCR TIM1_REG(0x30)
#define TIM1_CCR1 TIM1_REG(0x34)
#define TIM1_CCR2 TIM1_REG(0x38)
#define TIM1_CCR3 TIM1_REG(0x3C)
#define TIM1_CCR4 TIM1_REG(0x40)
#define TIM1_BDTR TIM1_REG(0x44)
#define TIM1_DCR TIM1_REG(0x48)
#define TIM1_DMAR TIM1_REG(0x4C)

#define DAC_BASE 0x40007400
#define DAC_REG(off) REGISTER(DAC_BASE + (off))
#define DAC_CR DAC_REG(0x00)
#define DAC_SWTRIGR DAC_REG(0x04)
#define DAC_DHR12R1 DAC_REG(0x08)
#define DAC_DHR12L1 DAC_REG(0x0C)
#define DAC_DHR8R1 DAC_REG(0x10)
#define DAC_DHR12R2 DAC_REG(0x14)
#define DAC_DHR12L2 DAC_REG(0x18)
#define DAC_DHR8R2 DAC_REG(0x1C)
#define DAC_DHR12RD DAC_REG(0x20)
#define DAC_DHR12LD DAC_REG(0x24)
#define DAC_DHR8RD DAC_REG(0x28)
#define DAC_DOR1 DAC_REG(0x2C)
#define DAC_DOR2 DAC_REG(0x30)
#define DAC_SR DAC_REG(0x34)

#define PWR_BASE 0x40007000
#define PWR_REG(off) REGISTER(PWR_BASE + (off))
#define PWR_CR PWR_REG(0x00)
#define PWR_CSR PWR_REG(0x04)

#define CAN2_BASE 0x40006800

#define CAN1_BASE 0x40006400

#define I2C3_BASE 0x40005C00
#define I2C3_REG(off) REGISTER(I2C3_BASE + (off))
#define I2C3_CR1 I2C3_REG(0x00)
#define I2C3_CR2 I2C3_REG(0x04)
#define I2C3_OAR1 I2C3_REG(0x08)
#define I2C3_OAR2 I2C3_REG(0x0C)
#define I2C3_DR I2C3_REG(0x10)
#define I2C3_SR1 I2C3_REG(0x14)
#define I2C3_SR2 I2C3_REG(0x18)
#define I2C3_CCR I2C3_REG(0x1C)
#define I2C3_TRISE I2C3_REG(0x20)

#define I2C2_BASE 0x40005800
#define I2C2_REG(off) REGISTER(I2C2_BASE + (off))
#define I2C2_CR1 I2C2_REG(0x00)
#define I2C2_CR2 I2C2_REG(0x04)
#define I2C2_OAR1 I2C2_REG(0x08)
#define I2C2_OAR2 I2C2_REG(0x0C)
#define I2C2_DR I2C2_REG(0x10)
#define I2C2_SR1 I2C2_REG(0x14)
#define I2C2_SR2 I2C2_REG(0x18)
#define I2C2_CCR I2C2_REG(0x1C)
#define I2C2_TRISE I2C2_REG(0x20)

#define I2C1_BASE 0x40005400
#define I2C1_REG(off) REGISTER(I2C1_BASE + (off))
#define I2C1_CR1 I2C1_REG(0x00)
#define I2C1_CR2 I2C1_REG(0x04)
#define I2C1_OAR1 I2C1_REG(0x08)
#define I2C1_OAR2 I2C1_REG(0x0C)
#define I2C1_DR I2C1_REG(0x10)
#define I2C1_SR1 I2C1_REG(0x14)
#define I2C1_SR2 I2C1_REG(0x18)
#define I2C1_CCR I2C1_REG(0x1C)
#define I2C1_TRISE I2C1_REG(0x20)

#define UART5_BASE 0x40005000
#define UART5_REG(off) REGISTER(UART5_BASE + (off))
#define UART5_SR UART5_REG(0x00)
#define UART5_DR UART5_REG(0x04)
#define UART5_BRR UART5_REG(0x08)
#define UART5_CR1 UART5_REG(0x0C)
#define UART5_CR2 UART5_REG(0x10)
#define UART5_CR3 UART5_REG(0x14)
#define UART5_GTPR UART5_REG(0x18)

#define UART4_BASE 0x40004C00
#define UART4_REG(off) REGISTER(UART4_BASE + (off))
#define UART4_SR UART4_REG(0x00)
#define UART4_DR UART4_REG(0x04)
#define UART4_BRR UART4_REG(0x08)
#define UART4_CR1 UART4_REG(0x0C)
#define UART4_CR2 UART4_REG(0x10)
#define UART4_CR3 UART4_REG(0x14)
#define UART4_GTPR UART4_REG(0x18)

#define USART3_BASE 0x40004800
#define USART3_REG(off) REGISTER(USART3_BASE + (off))
#define USART3_SR USART3_REG(0x00)
#define USART3_DR USART3_REG(0x04)
#define USART3_BRR USART3_REG(0x08)
#define USART3_CR1 USART3_REG(0x0C)
#define USART3_CR2 USART3_REG(0x10)
#define USART3_CR3 USART3_REG(0x14)
#define USART3_GTPR USART3_REG(0x18)

#define USART2_BASE 0x40004400
#define USART2_REG(off) REGISTER(USART2_BASE + (off))
#define USART2_SR USART2_REG(0x00)
#define USART2_DR USART2_REG(0x04)
#define USART2_BRR USART2_REG(0x08)
#define USART2_CR1 USART2_REG(0x0C)
#define USART2_CR2 USART2_REG(0x10)
#define USART2_CR3 USART2_REG(0x14)
#define USART2_GTPR USART2_REG(0x18)

#define I2S3ext_BASE 0x40004000

#define SPI3_BASE 0x40003C00
#define SPI3_REG(off) REGISTER(SPI3_BASE + (off))
#define SPI3_CR1 SPI3_REG(0x00)
#define SPI3_CR2 SPI3_REG(0x04)
#define SPI3_SR SPI3_REG(0x08)
#define SPI3_DR SPI3_REG(0x0C)
#define SPI3_CRCPR SPI3_REG(0x10)
#define SPI3_RXCRCR SPI3_REG(0x14)
#define SPI3_TXCRCR SPI3_REG(0x18)
#define SPI3_I2SCFGR SPI3_REG(0x1C)
#define SPI3_I2SPR SPI3_REG(0x20)

#define SPI2_BASE 0x40003800
#define SPI2_REG(off) REGISTER(SPI2_BASE + (off))
#define SPI2_CR1 SPI2_REG(0x00)
#define SPI2_CR2 SPI2_REG(0x04)
#define SPI2_SR SPI2_REG(0x08)
#define SPI2_DR SPI2_REG(0x0C)
#define SPI2_CRCPR SPI2_REG(0x10)
#define SPI2_RXCRCR SPI2_REG(0x14)
#define SPI2_TXCRCR SPI2_REG(0x18)
#define SPI2_I2SCFGR SPI2_REG(0x1C)
#define SPI2_I2SPR SPI2_REG(0x20)

#define I2S2ext_BASE 0x40003400

#define IWDG_BASE 0x40003000
#define IWDG_REG(off) REGISTER(IWDG_BASE + (off))
#define IWDG_KR IWDG_REG(0x00)
#define IWDG_PR IWDG_REG(0x04)
#define IWDG_RLR IWDG_REG(0x08)
#define IWDG_SR IWDG_REG(0x0C)

#define WWDG_BASE 0x40002C00
#define WWDG_REG(off) REGISTER(WWDG_BASE + (off))
#define WWDG_CR WWDG_REG(0x00)
#define WWDG_CFR WWDG_REG(0x04)
#define WWDG_SR WWDG_REG(0x08)

#define RTC_BASE 0x40002800

#define TIM14_BASE 0x40002000
#define TIM14_REG(off) REGISTER(TIM14_BASE + (off))
#define TIM14_CR1 TIM14_REG(0x00)
#define TIM14_SMCR TIM14_REG(0x08)
#define TIM14_DIER TIM14_REG(0x0C)
#define TIM14_SR TIM14_REG(0x10)
#define TIM14_EGR TIM14_REG(0x14)
#define TIM14_CCMR1 TIM14_REG(0x18)
#define TIM14_CCER TIM14_REG(0x20)
#define TIM14_CNT TIM14_REG(0x24)
#define TIM14_PSC TIM14_REG(0x28)
#define TIM14_ARR TIM14_REG(0x2C)
#define TIM14_CCR1 TIM14_REG(0x34)

#define TIM13_BASE 0x40001C00
#define TIM13_REG(off) REGISTER(TIM13_BASE + (off))
#define TIM13_CR1 TIM13_REG(0x00)
#define TIM13_SMCR TIM13_REG(0x08)
#define TIM13_DIER TIM13_REG(0x0C)
#define TIM13_SR TIM13_REG(0x10)
#define TIM13_EGR TIM13_REG(0x14)
#define TIM13_CCMR1 TIM13_REG(0x18)
#define TIM13_CCER TIM13_REG(0x20)
#define TIM13_CNT TIM13_REG(0x24)
#define TIM13_PSC TIM13_REG(0x28)
#define TIM13_ARR TIM13_REG(0x2C)
#define TIM13_CCR1 TIM13_REG(0x34)

#define TIM12_BASE 0x40001800
#define TIM12_REG(off) REGISTER(TIM12_BASE + (off))
#define TIM12_CR1 TIM12_REG(0x00)
#define TIM12_CR2 TIM12_REG(0x04)
#define TIM12_SMCR TIM12_REG(0x08)
#define TIM12_DIER TIM12_REG(0x0C)
#define TIM12_SR TIM12_REG(0x10)
#define TIM12_EGR TIM12_REG(0x14)
#define TIM12_CCMR1 TIM12_REG(0x18)
#define TIM12_CCER TIM12_REG(0x20)
#define TIM12_CNT TIM12_REG(0x24)
#define TIM12_PSC TIM12_REG(0x28)
#define TIM12_ARR TIM12_REG(0x2C)
#define TIM12_CCR1 TIM12_REG(0x34)
#define TIM12_CCR2 TIM12_REG(0x38)

#define TIM7_BASE 0x40001400
#define TIM7_REG(off) REGISTER(TIM7_BASE + (off))
#define TIM7_CR1 TIM7_REG(0x00)
#define TIM7_CR2 TIM7_REG(0x04)
#define TIM7_DIER TIM7_REG(0x0C)
#define TIM7_SR TIM7_REG(0x10)
#define TIM7_EGR TIM7_REG(0x14)
#define TIM7_CNT TIM7_REG(0x24)
#define TIM7_PSC TIM7_REG(0x28)
#define TIM7_ARR TIM7_REG(0x2C)

#define TIM6_BASE 0x40001000
#define TIM6_REG(off) REGISTER(TIM6_BASE + (off))
#define TIM6_CR1 TIM6_REG(0x00)
#define TIM6_CR2 TIM6_REG(0x04)
#define TIM6_DIER TIM6_REG(0x0C)
#define TIM6_SR TIM6_REG(0x10)
#define TIM6_EGR TIM6_REG(0x14)
#define TIM6_CNT TIM6_REG(0x24)
#define TIM6_PSC TIM6_REG(0x28)
#define TIM6_ARR TIM6_REG(0x2C)

#define TIM5_BASE 0x40000C00
#define TIM5_REG(off) REGISTER(TIM5_BASE + (off))
#define TIM5_CR1 TIM5_REG(0x00)
#define TIM5_CR2 TIM5_REG(0x04)
#define TIM5_SMCR TIM5_REG(0x08)
#define TIM5_DIER TIM5_REG(0x0C)
#define TIM5_SR TIM5_REG(0x10)
#define TIM5_EGR TIM5_REG(0x14)
#define TIM5_CCMR1 TIM5_REG(0x18)
#define TIM5_CCMR2 TIM5_REG(0x1C)
#define TIM5_CCER TIM5_REG(0x20)
#define TIM5_CNT TIM5_REG(0x24)
#define TIM5_PSC TIM5_REG(0x28)
#define TIM5_ARR TIM5_REG(0x2C)
#define TIM5_CCR1 TIM5_REG(0x34)
#define TIM5_CCR2 TIM5_REG(0x38)
#define TIM5_CCR3 TIM5_REG(0x3C)
#define TIM5_CCR4 TIM5_REG(0x40)
#define TIM5_DCR TIM5_REG(0x48)
#define TIM5_DMAR TIM5_REG(0x4C)
#define TIM5_OR TIM5_REG(0x50)

#define TIM4_BASE 0x40000800
#define TIM4_REG(off) REGISTER(TIM4_BASE + (off))
#define TIM4_CR1 TIM4_REG(0x00)
#define TIM4_CR2 TIM4_REG(0x04)
#define TIM4_SMCR TIM4_REG(0x08)
#define TIM4_DIER TIM4_REG(0x0C)
#define TIM4_SR TIM4_REG(0x10)
#define TIM4_EGR TIM4_REG(0x14)
#define TIM4_CCMR1 TIM4_REG(0x18)
#define TIM4_CCMR2 TIM4_REG(0x1C)
#define TIM4_CCER TIM4_REG(0x20)
#define TIM4_CNT TIM4_REG(0x24)
#define TIM4_PSC TIM4_REG(0x28)
#define TIM4_ARR TIM4_REG(0x2C)
#define TIM4_CCR1 TIM4_REG(0x34)
#define TIM4_CCR2 TIM4_REG(0x38)
#define TIM4_CCR3 TIM4_REG(0x3C)
#define TIM4_CCR4 TIM4_REG(0x40)
#define TIM4_DCR TIM4_REG(0x48)
#define TIM4_DMAR TIM4_REG(0x4C)

#define TIM3_BASE 0x40000400
#define TIM3_REG(off) REGISTER(TIM3_BASE + (off))
#define TIM3_CR1 TIM3_REG(0x00)
#define TIM3_CR2 TIM3_REG(0x04)
#define TIM3_SMCR TIM3_REG(0x08)
#define TIM3_DIER TIM3_REG(0x0C)
#define TIM3_SR TIM3_REG(0x10)
#define TIM3_EGR TIM3_REG(0x14)
#define TIM3_CCMR1 TIM3_REG(0x18)
#define TIM3_CCMR2 TIM3_REG(0x1C)
#define TIM3_CCER TIM3_REG(0x20)
#define TIM3_CNT TIM3_REG(0x24)
#define TIM3_PSC TIM3_REG(0x28)
#define TIM3_ARR TIM3_REG(0x2C)
#define TIM3_CCR1 TIM3_REG(0x34)
#define TIM3_CCR2 TIM3_REG(0x38)
#define TIM3_CCR3 TIM3_REG(0x3C)
#define TIM3_CCR4 TIM3_REG(0x40)
#define TIM3_DCR TIM3_REG(0x48)
#define TIM3_DMAR TIM3_REG(0x4C)

#define TIM2_BASE 0x40000000
#define TIM2_REG(off) REGISTER(TIM2_BASE + (off))
#define TIM2_CR1 TIM2_REG(0x00)
#define TIM2_CR2 TIM2_REG(0x04)
#define TIM2_SMCR TIM2_REG(0x08)
#define TIM2_DIER TIM2_REG(0x0C)
#define TIM2_SR TIM2_REG(0x10)
#define TIM2_EGR TIM2_REG(0x14)
#define TIM2_CCMR1 TIM2_REG(0x18)
#define TIM2_CCMR2 TIM2_REG(0x1C)
#define TIM2_CCER TIM2_REG(0x20)
#define TIM2_CNT TIM2_REG(0x24)
#define TIM2_PSC TIM2_REG(0x28)
#define TIM2_ARR TIM2_REG(0x2C)
#define TIM2_CCR1 TIM2_REG(0x34)
#define TIM2_CCR2 TIM2_REG(0x38)
#define TIM2_CCR3 TIM2_REG(0x3C)
#define TIM2_CCR4 TIM2_REG(0x40)
#define TIM2_DCR TIM2_REG(0x48)
#define TIM2_DMAR TIM2_REG(0x4C)
#define TIM2_OR TIM2_REG(0x50)

#define SCS_BASE 0xE000E000
#define SCS_REG(off) REGISTER(SCS_BASE + (off))
#define SCS_ACTLR SCS_REG(0x008)
#define SCS_STCSR SCS_REG(0x010)
#define SCS_STRVR SCS_REG(0x014)
#define SCS_STCVR SCS_REG(0x018)
#define SCS_STCR SCS_REG(0x01C)
#define SCS_CPUID SCS_REG(0xD00)
#define SCS_ICSR SCS_REG(0xD04)
#define SCS_VTOR SCS_REG(0xD08)
#define SCS_AIRCR SCS_REG(0xD0C)
#define SCS_SCR SCS_REG(0xD10)
#define SCS_CCR SCS_REG(0xD14)
#define SCS_SHPR1 SCS_REG(0xD18)
#define SCS_SHPR2 SCS_REG(0xD1C)
#define SCS_SHPR3 SCS_REG(0xD20)
#define SCS_SHCSR SCS_REG(0xD24)
#define SCS_CFSR SCS_REG(0xD28)
#define SCS_HFSR SCS_REG(0xD2C)
#define SCS_DFSR SCS_REG(0xD30)
#define SCS_MMFAR SCS_REG(0xD34)
#define SCS_BFAR SCS_REG(0xD38)
#define SCS_AFSR SCS_REG(0xD3C)
#define SCS_ID_PFR0 SCS_REG(0xD40)
#define SCS_ID_PFR1 SCS_REG(0xD44)
#define SCS_ID_DFR0 SCS_REG(0xD48)
#define SCS_ID_AFR0 SCS_REG(0xD4C)
#define SCS_MMFR0 SCS_REG(0xD50)
#define SCS_MMFR1 SCS_REG(0xD54)
#define SCS_MMFR2 SCS_REG(0xD58)
#define SCS_MMFR3 SCS_REG(0xD5C)
#define SCS_ISAR0 SCS_REG(0xD60)
#define SCS_ISAR1 SCS_REG(0xD64)
#define SCS_ISAR2 SCS_REG(0xD68)
#define SCS_ISAR3 SCS_REG(0xD6C)
#define SCS_ISAR4 SCS_REG(0xD70)
#define SCS_CPACR SCS_REG(0xD88)
#define SCS_STIR SCS_REG(0xF00)
#define MPU_TYPE SCS_REG(0xD90)
#define MPU_CTRL SCS_REG(0xD94)
#define MPU_RNR SCS_REG(0xD98)
#define MPU_RBAR SCS_REG(0xD9C)
#define MPU_RASR SCS_REG(0xDA0)
#define MPU_RBAR_A1 SCS_REG(0xDA4)
#define MPU_RASR_A1 SCS_REG(0xDA8)
#define MPU_RBAR_A2 SCS_REG(0xDAC)
#define MPU_RASR_A2 SCS_REG(0xDB0)
#define MPU_RBAR_A3 SCS_REG(0xDB4)
#define MPU_RASR_A3 SCS_REG(0xDB8)
#define NVIC_ICTR SCS_REG(0x004)
#define NVIC_ISER0 SCS_REG(0x100)
#define NVIC_ISER1 SCS_REG(0x104)
#define NVIC_ISER2 SCS_REG(0x108)
#define NVIC_ISER3 SCS_REG(0x10C)
#define NVIC_ISER4 SCS_REG(0x110)
#define NVIC_ISER5 SCS_REG(0x114)
#define NVIC_ISER6 SCS_REG(0x118)
#define NVIC_ISER7 SCS_REG(0x11C)
#define NVIC_ICER0 SCS_REG(0x180)
#define NVIC_ICER1 SCS_REG(0x184)
#define NVIC_ICER2 SCS_REG(0x188)
#define NVIC_ICER3 SCS_REG(0x18C)
#define NVIC_ICER4 SCS_REG(0x190)
#define NVIC_ICER5 SCS_REG(0x194)
#define NVIC_ICER6 SCS_REG(0x198)
#define NVIC_ICER7 SCS_REG(0x19C)
#define NVIC_ISPR0 SCS_REG(0x200)
#define NVIC_ISPR1 SCS_REG(0x204)
#define NVIC_ISPR2 SCS_REG(0x208)
#define NVIC_ISPR3 SCS_REG(0x20C)
#define NVIC_ISPR4 SCS_REG(0x210)
#define NVIC_ISPR5 SCS_REG(0x214)
#define NVIC_ISPR6 SCS_REG(0x218)
#define NVIC_ISPR7 SCS_REG(0x21C)
#define NVIC_ICPR0 SCS_REG(0x280)
#define NVIC_ICPR1 SCS_REG(0x284)
#define NVIC_ICPR2 SCS_REG(0x288)
#define NVIC_ICPR3 SCS_REG(0x28C)
#define NVIC_ICPR4 SCS_REG(0x290)
#define NVIC_ICPR5 SCS_REG(0x294)
#define NVIC_ICPR6 SCS_REG(0x298)
#define NVIC_ICPR7 SCS_REG(0x29C)
#define NVIC_IABR0 SCS_REG(0x300)
#define NVIC_IABR1 SCS_REG(0x304)
#define NVIC_IABR2 SCS_REG(0x308)
#define NVIC_IABR3 SCS_REG(0x30C)
#define NVIC_IABR4 SCS_REG(0x310)
#define NVIC_IABR5 SCS_REG(0x314)
#define NVIC_IABR6 SCS_REG(0x318)
#define NVIC_IABR7 SCS_REG(0x31C)
#define NVIC_IPR0 SCS_REG(0x400)
#define NVIC_IPR1 SCS_REG(0x404)
#define NVIC_IPR2 SCS_REG(0x408)
#define NVIC_IPR3 SCS_REG(0x40C)
#define NVIC_IPR4 SCS_REG(0x410)
#define NVIC_IPR5 SCS_REG(0x414)
#define NVIC_IPR6 SCS_REG(0x418)
#define NVIC_IPR7 SCS_REG(0x41C)
#define NVIC_IPR8 SCS_REG(0x420)
#define NVIC_IPR9 SCS_REG(0x424)
#define NVIC_IPR10 SCS_REG(0x428)
#define NVIC_IPR11 SCS_REG(0x42C)
#define NVIC_IPR12 SCS_REG(0x430)
#define NVIC_IPR13 SCS_REG(0x434)
#define NVIC_IPR14 SCS_REG(0x438)
#define NVIC_IPR15 SCS_REG(0x43C)
#define NVIC_IPR16 SCS_REG(0x440)
#define NVIC_IPR17 SCS_REG(0x444)
#define NVIC_IPR18 SCS_REG(0x448)
#define NVIC_IPR19 SCS_REG(0x44C)
#define NVIC_IPR20 SCS_REG(0x450)
#define NVIC_IPR21 SCS_REG(0x454)
#define NVIC_IPR22 SCS_REG(0x458)
#define NVIC_IPR23 SCS_REG(0x45C)
#define NVIC_IPR24 SCS_REG(0x460)
#define NVIC_IPR25 SCS_REG(0x464)
#define NVIC_IPR26 SCS_REG(0x468)
#define NVIC_IPR27 SCS_REG(0x46C)
#define NVIC_IPR28 SCS_REG(0x470)
#define NVIC_IPR29 SCS_REG(0x474)
#define NVIC_IPR30 SCS_REG(0x478)
#define NVIC_IPR31 SCS_REG(0x47C)
#define NVIC_IPR32 SCS_REG(0x480)
#define NVIC_IPR33 SCS_REG(0x484)
#define NVIC_IPR34 SCS_REG(0x488)
#define NVIC_IPR35 SCS_REG(0x48C)
#define NVIC_IPR36 SCS_REG(0x490)
#define NVIC_IPR37 SCS_REG(0x494)
#define NVIC_IPR38 SCS_REG(0x498)
#define NVIC_IPR39 SCS_REG(0x49C)
#define NVIC_IPR40 SCS_REG(0x4A0)
#define NVIC_IPR41 SCS_REG(0x4A4)
#define NVIC_IPR42 SCS_REG(0x4A8)
#define NVIC_IPR43 SCS_REG(0x4AC)
#define NVIC_IPR44 SCS_REG(0x4B0)
#define NVIC_IPR45 SCS_REG(0x4B4)
#define NVIC_IPR46 SCS_REG(0x4B8)
#define NVIC_IPR47 SCS_REG(0x4BC)
#define NVIC_IPR48 SCS_REG(0x4C0)
#define NVIC_IPR49 SCS_REG(0x4C4)
#define NVIC_IPR50 SCS_REG(0x4C8)
#define NVIC_IPR51 SCS_REG(0x4CC)
#define NVIC_IPR52 SCS_REG(0x4D0)
#define NVIC_IPR53 SCS_REG(0x4D4)
#define NVIC_IPR54 SCS_REG(0x4D8)
#define NVIC_IPR55 SCS_REG(0x4DC)
#define NVIC_IPR56 SCS_REG(0x4E0)
#define NVIC_IPR57 SCS_REG(0x4E4)
#define NVIC_IPR58 SCS_REG(0x4E8)
#define NVIC_IPR59 SCS_REG(0x4EC)
#define FPU_FPCCR SCS_REG(0xF34)
#define FPU_FPCAR SCS_REG(0xF38)
#define FPU_FPDSCR SCS_REG(0xF3C)
#define FPU_MVFR0 SCS_REG(0xF40)
#define FPU_MVFR1 SCS_REG(0xF44)

#define DBGMCU_BASE 0xE0042000
#define DBGMCU_REG(off) REGISTER(DBGMCU_BASE + (off))
#define DBGMCU_IDCODE DBGMCU_REG(0x00)
#define DBGMCU_CR DBGMCU_REG(0x04)
#define DBGMCU_APB1_FZ DBGMCU_REG(0x08)
#define DBGMCU_APB2_FZ DBGMCU_REG(0x0C)

#endif

