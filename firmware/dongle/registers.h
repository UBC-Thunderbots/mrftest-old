#ifndef REGISTERS_H
#define REGISTERS_H

#include "stdint.h"

#define BIT(name, position) \
	static const unsigned int name ## _SHIFT __attribute__((unused)) = (position); \
	static const uint32_t name __attribute__((unused)) = 1 << (position);

#define BITS(name, position, length) \
	static const unsigned int name ## _SHIFT __attribute__((unused)) = (position); \
	static const uint32_t name ## _MSKU __attribute__((unused)) = (1 << (length)) - 1; \
	static const uint32_t name ## _MSK __attribute__((unused)) = ((1 << (length)) - 1) << (position); \
	static uint32_t name(uint32_t value) __attribute__((unused)); \
	static uint32_t name(uint32_t value) { return value << (position); } \
	static uint32_t name ## _X(uint32_t value) __attribute__((unused)); \
	static uint32_t name ## _X(uint32_t value) { return (value >> (position)) & name ## _MSKU ; }

#define BITSS(name, position, width) \
	static unsigned int name ## _SHIFT(unsigned int i) __attribute__((unused)); \
	static unsigned int name ## _SHIFT(unsigned int i) { return (position) + i * (width); } \
	static uint32_t name ## _MSKU(unsigned int i) __attribute__((unused)); \
	static uint32_t name ## _MSKU(unsigned int i __attribute__((unused))) { return (1 << (width)) - 1; } \
	static uint32_t name ## _MSK(unsigned int i) __attribute__((unused)); \
	static uint32_t name ## _MSK(unsigned int i) { return name ## _MSKU(i) << name ## _SHIFT(i); } \
	static uint32_t name(unsigned int i, uint32_t value) __attribute__((unused)); \
	static uint32_t name(unsigned int i, uint32_t value) { return value << name ## _SHIFT(i); } \
	static uint32_t name ## _X(unsigned int i, uint32_t value) __attribute__((unused)); \
	static uint32_t name ## _X(unsigned int i, uint32_t value) { return (value >> name ## _SHIFT(i)) & name ## _MSKU(i); }



extern volatile uint32_t OTG_FS_GOTGCTL;
BIT(SRQSCS, 0)
BIT(SRQ, 1)
BIT(HNGSCS, 8)
BIT(HNPRQ, 9)
BIT(HSHNPEN, 10)
BIT(DHNPEN, 11)
BIT(CIDSTS, 16)
BIT(DBCT, 17)
BIT(ASVLD, 18)
BIT(BSVLD, 19)

extern volatile uint32_t OTG_FS_GOTGINT;
BIT(SEDET, 2)
BIT(SRSSCHQ, 8)
BIT(HNSSCHG, 9)
BIT(HNGDET, 17)
BIT(ADTOCHQ, 18)
BIT(DBCDNE, 19)

extern volatile uint32_t OTG_FS_GAHBCFG;
BIT(GINTMSK, 0)
BIT(TXFELVL, 7)
BIT(PTXFELVL, 8)

extern volatile uint32_t OTG_FS_GUSBCFG;
BITS(TOCAL, 0, 3)
BIT(PHYSEL, 7)
BIT(SRPCAP, 8)
BIT(HNPCAP, 9)
BITS(TRDT, 10, 4)
BIT(FHMOD, 29)
BIT(FDMOD, 30)
BIT(CTXPKT, 31)

extern volatile uint32_t OTG_FS_GRSTCTL;
BIT(CSRST, 0)
BIT(HSRST, 1)
BIT(FCRST, 2)
BIT(RXFFLSH, 4)
BIT(TXFFLSH, 5)
BITS(GRSTCTL_TXFNUM, 6, 5)
BIT(AHBIDL, 31)

extern volatile uint32_t OTG_FS_GINTSTS;
BIT(CMOD, 0)
BIT(MMIS, 1)
BIT(OTGINT, 2)
BIT(SOF, 3)
BIT(RXFLVL, 4)
BIT(NPTXFE, 5)
BIT(GINAKEFF, 6)
BIT(GOUTNAKEFF, 7)
BIT(ESUSP, 10)
BIT(USBSUSP, 11)
BIT(USBRST, 12)
BIT(ENUMDNE, 13)
BIT(ISOODRP, 14)
BIT(EOPF, 15)
BIT(GINTSTS_IEPINT, 18)
BIT(GINTSTS_OEPINT, 19)
BIT(IISOIXFR, 20)
BIT(IPXFR, 21)
BIT(INCOMPISOOUT, 21)
BIT(HPRTINT, 24)
BIT(HCINT, 25)
BIT(PTXFE, 26)
BIT(CIDSCHG, 28)
BIT(DISCINT, 29)
BIT(SRQINT, 30)
BIT(WKUINT, 31)

extern volatile uint32_t OTG_FS_GINTMSK;
BIT(MMISM, 1)
//BIT(OTGINT, 2)
BIT(SOFM, 3)
BIT(RXFLVLM, 4)
BIT(NPTXFEM, 5)
BIT(GINAKEFFM, 6)
BIT(GOUTNAKEFFM, 7)
BIT(ESUSPM, 10)
BIT(USBSUSPM, 11)
BIT(USBRSTM, 12)
BIT(ENUMDNEM, 13)
BIT(ISOODRPM, 14)
BIT(EOPFM, 15)
BIT(EPMISM, 17)
BIT(GINTMSK_IEPINT, 18)
BIT(GINTMSK_OEPINT, 19)
BIT(IISOIXFRM, 20)
BIT(IPXFRM, 21)
BIT(IISOOXFRM, 21)
BIT(PRTIM, 24)
BIT(HCIM, 25)
BIT(PTXFEM, 26)
BIT(CIDSCHGM, 28)
//BIT(DISCINT, 29)
BIT(SRQIM, 30)
BIT(WUIM, 31)

extern volatile uint32_t OTG_FS_GRXSTSR;
extern volatile uint32_t OTG_FS_GRXSTSP;
BITS(CHNUM, 0, 4)
BITS(BCNT, 4, 11)
BITS(GRXSTSP_DPID, 15, 2)
BITS(PKTSTS, 17, 4)
BITS(GRXSTSP_EPNUM, 0, 4)
//BITS(BCNT, 4, 11)
//BITS(GRXSTSP_DPID, 15, 2)
//BITS(PKTSTS, 17, 4)
BITS(FRMNUM, 21, 4)

extern volatile uint32_t OTG_FS_GRXFSIZ;
BITS(RXFD, 0, 16)

extern volatile uint32_t OTG_FS_HNPTXFSIZ;
BITS(NPTXFSA, 0, 16)
BITS(NPTXFD, 16, 16)

extern volatile uint32_t OTG_FS_DIEPTXF0;
BITS(TX0FSA, 0, 16)
BITS(TX0FD, 16, 16)

extern volatile uint32_t OTG_FS_HNPTXSTS;
BITS(NPTXFSAV, 0, 16)
BITS(NPTQXSAV, 16, 8)
BITS(NPTXQTOP, 24, 7)

extern volatile uint32_t OTG_FS_GCCFG;
BIT(PWRDWN, 16)
BIT(VBUSASEN, 18)
BIT(VBUSBSEN, 19)
BIT(SOFOUTEN, 20)
BIT(NOVBUSSENS, 21)

extern volatile uint32_t OTG_FS_CID;

extern volatile uint32_t OTG_FS_HPTXFSIZ;
BITS(PTXSA, 0, 16)
BITS(PTXFD, 16, 16)

extern volatile uint32_t OTG_FS_DIEPTXF1;
extern volatile uint32_t OTG_FS_DIEPTXF2;
extern volatile uint32_t OTG_FS_DIEPTXF3;
BITS(INEPTXSA, 0, 16)
BITS(INEPTXFD, 16, 16)

extern volatile uint32_t OTG_FS_HCFG;
BITS(FSLSPCS, 0, 2)
BIT(FSLSS, 2)

extern volatile uint32_t OTG_FS_HFIR;
BITS(FRIVL, 0, 16)

extern volatile uint32_t OTG_FS_HFNUM;
BITS(FRNUM, 0, 16)
BITS(FTREM, 16, 16)

extern volatile uint32_t OTG_FS_HPTXSTS;
BITS(PTXFSAVL, 0, 16)
BITS(PTXQSAV, 16, 8)
BITS(PTXQTOP, 24, 8)

extern volatile uint32_t OTG_FS_HAINT;
BITS(HAINT, 0, 16)

extern volatile uint32_t OTG_FS_HAINTMSK;
BITS(HAINTM, 0, 16)

extern volatile uint32_t OTG_FS_HPRT;
BIT(PCSTS, 0)
BIT(PCDET, 1)
BIT(PENA, 2)
BIT(PENCHNG, 3)
BIT(POCA, 4)
BIT(POCCHNG, 5)
BIT(PRES, 6)
BIT(PSUSP, 7)
BIT(PRST, 8)
BITS(PLSTS, 10, 2)
BIT(PPWR, 12)
BITS(PTCTL, 13, 4)
BITS(PSPD, 17, 2)

extern volatile uint32_t OTG_FS_HCCHAR0;
extern volatile uint32_t OTG_FS_HCCHAR1;
extern volatile uint32_t OTG_FS_HCCHAR2;
extern volatile uint32_t OTG_FS_HCCHAR3;
extern volatile uint32_t OTG_FS_HCCHAR4;
extern volatile uint32_t OTG_FS_HCCHAR5;
extern volatile uint32_t OTG_FS_HCCHAR6;
extern volatile uint32_t OTG_FS_HCCHAR7;
BITS(MPSIZ, 0, 11)
BITS(HCCHAR_EPNUM, 11, 4)
BIT(EPDIR, 15)
BIT(LSDEV, 17)
BITS(EPTYP, 18, 2)
BITS(HCCHAR_MCNT, 20, 2)
BITS(HCCHAR_DAD, 22, 7)
BIT(ODDFRM, 29)
BIT(CHDIS, 30)
BIT(CHENA, 31)

extern volatile uint32_t OTG_FS_HCINT0;
extern volatile uint32_t OTG_FS_HCINT1;
extern volatile uint32_t OTG_FS_HCINT2;
extern volatile uint32_t OTG_FS_HCINT3;
extern volatile uint32_t OTG_FS_HCINT4;
extern volatile uint32_t OTG_FS_HCINT5;
extern volatile uint32_t OTG_FS_HCINT6;
extern volatile uint32_t OTG_FS_HCINT7;
BIT(XFRC, 0)
BIT(CHH, 1)
BIT(HCINT_STALL, 3)
BIT(NAK, 4)
BIT(ACK, 5)
BIT(TXERR, 7)
BIT(BBERR, 8)
BIT(FRMOR, 9)
BIT(DTERR, 10)

extern volatile uint32_t OTG_FS_HCINTMSK0;
extern volatile uint32_t OTG_FS_HCINTMSK1;
extern volatile uint32_t OTG_FS_HCINTMSK2;
extern volatile uint32_t OTG_FS_HCINTMSK3;
extern volatile uint32_t OTG_FS_HCINTMSK4;
extern volatile uint32_t OTG_FS_HCINTMSK5;
extern volatile uint32_t OTG_FS_HCINTMSK6;
extern volatile uint32_t OTG_FS_HCINTMSK7;
BIT(XFRCM, 0)
BIT(CHHM, 1)
BIT(STALLM, 3)
BIT(NAKM, 4)
BIT(ACKM, 5)
BIT(NYET, 6)
BIT(TXERRM, 7)
BIT(BBERRM, 8)
BIT(FRMORM, 9)
BIT(DTERRM, 10)

extern volatile uint32_t OTG_FS_HCTSIZ0;
extern volatile uint32_t OTG_FS_HCTSIZ1;
extern volatile uint32_t OTG_FS_HCTSIZ2;
extern volatile uint32_t OTG_FS_HCTSIZ3;
extern volatile uint32_t OTG_FS_HCTSIZ4;
extern volatile uint32_t OTG_FS_HCTSIZ5;
extern volatile uint32_t OTG_FS_HCTSIZ6;
extern volatile uint32_t OTG_FS_HCTSIZ7;
BITS(XFRSIZ, 0, 19)
BITS(PKTCNT, 19, 10)
BITS(HCTSIZ_DPID, 29, 2)

extern volatile uint32_t OTG_FS_DCFG;
BITS(DSPD, 0, 2)
BIT(NZLSOHSK, 2)
BITS(DCFG_DAD, 4, 7)
BITS(PFILVL, 11, 2)

extern volatile uint32_t OTG_FS_DCTL;
BIT(RWUSIG, 0)
BIT(SDIS, 1)
BIT(GINSTS, 2)
BIT(GONSTS, 3)
BITS(TCTL, 4, 3)
BIT(SGINAK, 7)
BIT(CGINAK, 8)
BIT(SGONAK, 9)
BIT(CGONAK, 10)
BIT(POPRGDNE, 11)

extern volatile uint32_t OTG_FS_DSTS;
BIT(SUSPSTS, 0)
BITS(ENUMSPD, 1, 2)
BIT(EERR, 3)
BITS(FNSOF, 8, 14)

extern volatile uint32_t OTG_FS_DIEPMSK;
//BIT(XFRCM, 0)
BIT(EPDM, 1)
BIT(TOM, 3)
BIT(ITTXFEMSK, 4)
BIT(INEPNMM, 5)
BIT(INEPNEM, 6)

extern volatile uint32_t OTG_FS_DOEPMSK;
//BIT(XFRCM, 0)
//BIT(EPDM, 1)
BIT(STUPM, 3)
BIT(OTEPDM, 4)

extern volatile uint32_t OTG_FS_DAINT;
BITS(DAINT_IEPINT, 0, 16)
BITS(DAINT_OEPINT, 16, 16)

extern volatile uint32_t OTG_FS_DAINTMSK;
BITS(IEPM, 0, 16)
BITS(OEPM, 16, 16)

extern volatile uint32_t OTG_FS_DVBUSDIS;
BITS(VBUSDT, 0, 16)

extern volatile uint32_t OTG_FS_DVBUSPULSE;
BITS(DVBUSP, 0, 12)

extern volatile uint32_t OTG_FS_DIEPEMPMSK;
BITS(INEPTXFEM, 0, 16)

extern volatile uint32_t OTG_FS_DIEPCTL0;
extern volatile uint32_t OTG_FS_DIEPCTL1;
extern volatile uint32_t OTG_FS_DIEPCTL2;
extern volatile uint32_t OTG_FS_DIEPCTL3;
//BITS(MPSIZ, 0, 11)
BIT(USBAEP, 15)
BIT(EONUM, 16)
BIT(DPID, 16)
BIT(NAKSTS, 17)
//BITS(EPTYP, 18, 2)
BIT(DIEPCTL_STALL, 21)
BITS(DIEPCTL_TXFNUM, 22, 4)
BIT(CNAK, 26)
BIT(SNAK, 27)
BIT(SD0PID, 28)
BIT(SEVNFRM, 28)
BIT(SODDFRM, 29)
BIT(EPDIS, 30)
BIT(EPENA, 31)

extern volatile uint32_t OTG_FS_DTXFSTS0;
extern volatile uint32_t OTG_FS_DTXFSTS1;
extern volatile uint32_t OTG_FS_DTXFSTS2;
extern volatile uint32_t OTG_FS_DTXFSTS3;
BITS(INEPTFSAV, 0, 16)

extern volatile uint32_t OTG_FS_DOEPCTL0;
extern volatile uint32_t OTG_FS_DOEPCTL1;
extern volatile uint32_t OTG_FS_DOEPCTL2;
extern volatile uint32_t OTG_FS_DOEPCTL3;
//BITS(MPSIZ, 0, 11)
//BIT(USBAEP, 15)
//BIT(EONUM, 16)
//BIT(DPID, 16)
//BIT(NAKSTS, 17)
//BITS(EPTYP, 18, 2)
BIT(SNPM, 20)
BIT(DOEPCTL_STALL, 21)
//BIT(CNAK, 26)
//BIT(SNAK, 27)
//BIT(SD0PID, 28)
//BIT(SEVNFRM, 28)
//BIT(SODDFRM, 29)
BIT(SD1PID, 29)
//BIT(EPDIS, 30)
//BIT(EPENA, 31)

extern volatile uint32_t OTG_FS_DIEPINT0;
extern volatile uint32_t OTG_FS_DIEPINT1;
extern volatile uint32_t OTG_FS_DIEPINT2;
extern volatile uint32_t OTG_FS_DIEPINT3;
//BIT(XFRC, 0)
BIT(EPDISD, 1)
BIT(TOC, 3)
BIT(ITTXFE, 4)
BIT(INEPNE, 6)
BIT(TXFE, 7)

extern volatile uint32_t OTG_FS_DOEPINT0;
extern volatile uint32_t OTG_FS_DOEPINT1;
extern volatile uint32_t OTG_FS_DOEPINT2;
extern volatile uint32_t OTG_FS_DOEPINT3;
//BIT(XFRC, 0)
//BIT(EPDISD, 1)
BIT(STUP, 3)
BIT(OTEPDIS, 4)
BIT(B2BSTUP, 6)

extern volatile uint32_t OTG_FS_DIEPTSIZ0;
extern volatile uint32_t OTG_FS_DIEPTSIZ1;
extern volatile uint32_t OTG_FS_DIEPTSIZ2;
extern volatile uint32_t OTG_FS_DIEPTSIZ3;
//BITS(XFRSIZ, 0, 19)
//BITS(PKTCNT, 19, 10)
BITS(DIEPTSIZ_MCNT, 29, 2)
BITS(STUPCNT, 29, 2)

extern volatile uint32_t OTG_FS_DOEPTSIZ0;
extern volatile uint32_t OTG_FS_DOEPTSIZ1;
extern volatile uint32_t OTG_FS_DOEPTSIZ2;
extern volatile uint32_t OTG_FS_DOEPTSIZ3;
//BITS(XFRSIZ, 0, 19)
//BITS(PKTCNT, 19, 10)
//BITS(STUPCNT, 29, 2)
BITS(RXDPID, 29, 2)

extern volatile uint32_t OTG_FS_PCGCCTL;
BIT(STPPCLK, 0)
BIT(GATEHCLK, 1)
BIT(PHYSUSP, 4)

extern volatile uint32_t OTG_FS_FIFO[4][0x1000 / 4];



extern volatile uint32_t FLASH_ACR;
#define FLASH_ACR_R 0xFFFFE0F8
BITS(LATENCY, 0, 3)
BIT(PRFTEN, 8)
BIT(ICEN, 9)
BIT(DCEN, 10)
BIT(ICRST, 11)
BIT(DCRST, 12)

extern volatile uint32_t FLASH_KEYR;
extern volatile uint32_t FLASH_OPTKEYR;

extern volatile uint32_t FLASH_SR;
BIT(EOP, 0)
BIT(OPERR, 1)
BIT(WRPERR, 4)
BIT(PGAERR, 5)
BIT(PGPERR, 6)
BIT(PGSERR, 7)
BIT(BSY, 16)

extern volatile uint32_t FLASH_CR;
BIT(PG, 0)
BIT(SER, 1)
BIT(MER, 2)
BITS(SNB, 3, 4)
BITS(PSIZE, 8, 2)
BIT(STRT, 16)
BIT(EOPIE, 24)
BIT(ERRIE, 25)
BIT(LOCK, 31)

extern volatile uint32_t FLASH_OPTCR;
BIT(OPTLOCK, 0)
BIT(OPTSTRT, 1)
BITS(BOR_LEV, 2, 2)
BIT(WDG_SW, 5)
BIT(NRST_STOP, 6)
BIT(NRST_STDBY, 7)
BITS(RDP, 8, 8)
BITS(NWRP, 16, 12)



extern volatile uint32_t RCC_CR;
BIT(HSION, 0)
BIT(HSIRDY, 1)
BITS(HSITRIM, 3, 5)
BITS(HSICAL, 8, 8)
BIT(HSEON, 16)
BIT(HSERDY, 17)
BIT(HSEBYP, 18)
BIT(CSSON, 19)
BIT(PLLON, 24)
BIT(PLLRDY, 25)
BIT(PLLI2SON, 26)
BIT(PLLI2SRDY, 27)

extern volatile uint32_t RCC_PLLCFGR;
BITS(PLLM, 0, 5)
BITS(PLLN, 6, 9)
BITS(PLLP, 16, 2)
BIT(PLLSRC, 22)
BITS(PLLQ, 24, 4)

extern volatile uint32_t RCC_CFGR;
BITS(SW, 0, 2)
BITS(SWS, 2, 2)
BITS(HPRE, 4, 4)
BITS(PPRE1, 10, 3)
BITS(PPRE2, 13, 3)
BITS(RTCPRE, 16, 5)
BITS(MCO1, 21, 2)
BIT(I2SSRC, 23)
BITS(MCO1PRE, 24, 3)
BITS(MCO2PRE, 27, 3)
BITS(MCO2, 30, 2)

extern volatile uint32_t RCC_CIR;
BIT(LSIRDYF, 0)
BIT(LSERDYF, 1)
BIT(HSIRDYF, 2)
BIT(HSERDYF, 3)
BIT(PLLRDYF, 4)
BIT(PLLI2SRDYF, 5)
BIT(CSSF, 7)
BIT(LSIRDYIE, 8)
BIT(LSERDYIE, 9)
BIT(HSIRDYIE, 10)
BIT(HSERDYIE, 11)
BIT(PLLRDYIE, 12)
BIT(PLLI2SRDYIE, 13)
BIT(LSIRDYC, 16)
BIT(LSERDYC, 17)
BIT(HSIRDYC, 18)
BIT(HSERDYC, 19)
BIT(PLLRDYC, 20)
BIT(PLLI2SRDYC, 21)
BIT(CSSC, 23)

extern volatile uint32_t RCC_AHB1RSTR;
extern volatile uint32_t RCC_AHB2RSTR;
extern volatile uint32_t RCC_AHB3RSTR;
extern volatile uint32_t RCC_APB1RSTR;
extern volatile uint32_t RCC_APB2RSTR;
extern volatile uint32_t RCC_AHB1ENR;
extern volatile uint32_t RCC_AHB2ENR;
extern volatile uint32_t RCC_AHB3ENR;
extern volatile uint32_t RCC_APB1ENR;
extern volatile uint32_t RCC_APB2ENR;
extern volatile uint32_t RCC_AHB1LPENR;
extern volatile uint32_t RCC_AHB2LPENR;
extern volatile uint32_t RCC_AHB3LPENR;
extern volatile uint32_t RCC_APB1LPENR;
extern volatile uint32_t RCC_APB2LPENR;

extern volatile uint32_t RCC_BDCR;
BIT(LSEON, 0)
BIT(LSERDY, 1)
BIT(LSEBYP, 2)
BITS(RTCSEL, 8, 2)
BIT(RTCEN, 15)
BIT(BDRST, 16)

extern volatile uint32_t RCC_CSR;
BIT(LSION, 0)
BIT(LSIRDY, 1)
BIT(RMVF, 24)
BIT(BORRSTF, 25)
BIT(PINRSTF, 26)
BIT(PORRSTF, 27)
BIT(SFTRSTF, 28)
BIT(IWDGRSTF, 29)
BIT(WWDGRSTF, 30)
BIT(LPWRRSTF, 31)

extern volatile uint32_t RCC_SSCGR;
BITS(MODPER, 0, 13)
BITS(INCSTEP, 13, 15)
BIT(SPREADSEL, 30)
BIT(SSCGEN, 31)

extern volatile uint32_t RCC_PLLI2SCFGR;
BITS(PLLI2SN, 6, 8)
BITS(PLLI2SR, 28, 3)



extern volatile uint32_t GPIOI_MODER;
extern volatile uint32_t GPIOI_OTYPER;
extern volatile uint32_t GPIOI_OSPEEDR;
extern volatile uint32_t GPIOI_PUPDR;
extern volatile uint32_t GPIOI_IDR;
extern volatile uint32_t GPIOI_ODR;
extern volatile uint32_t GPIOI_BSRR;
extern volatile uint32_t GPIOI_LCKR;
extern volatile uint32_t GPIOI_AFRL;
extern volatile uint32_t GPIOI_AFRH;

extern volatile uint32_t GPIOH_MODER;
extern volatile uint32_t GPIOH_OTYPER;
extern volatile uint32_t GPIOH_OSPEEDR;
extern volatile uint32_t GPIOH_PUPDR;
extern volatile uint32_t GPIOH_IDR;
extern volatile uint32_t GPIOH_ODR;
extern volatile uint32_t GPIOH_BSRR;
extern volatile uint32_t GPIOH_LCKR;
extern volatile uint32_t GPIOH_AFRL;
extern volatile uint32_t GPIOH_AFRH;

extern volatile uint32_t GPIOG_MODER;
extern volatile uint32_t GPIOG_OTYPER;
extern volatile uint32_t GPIOG_OSPEEDR;
extern volatile uint32_t GPIOG_PUPDR;
extern volatile uint32_t GPIOG_IDR;
extern volatile uint32_t GPIOG_ODR;
extern volatile uint32_t GPIOG_BSRR;
extern volatile uint32_t GPIOG_LCKR;
extern volatile uint32_t GPIOG_AFRL;
extern volatile uint32_t GPIOG_AFRH;

extern volatile uint32_t GPIOF_MODER;
extern volatile uint32_t GPIOF_OTYPER;
extern volatile uint32_t GPIOF_OSPEEDR;
extern volatile uint32_t GPIOF_PUPDR;
extern volatile uint32_t GPIOF_IDR;
extern volatile uint32_t GPIOF_ODR;
extern volatile uint32_t GPIOF_BSRR;
extern volatile uint32_t GPIOF_LCKR;
extern volatile uint32_t GPIOF_AFRL;
extern volatile uint32_t GPIOF_AFRH;

extern volatile uint32_t GPIOE_MODER;
extern volatile uint32_t GPIOE_OTYPER;
extern volatile uint32_t GPIOE_OSPEEDR;
extern volatile uint32_t GPIOE_PUPDR;
extern volatile uint32_t GPIOE_IDR;
extern volatile uint32_t GPIOE_ODR;
extern volatile uint32_t GPIOE_BSRR;
extern volatile uint32_t GPIOE_LCKR;
extern volatile uint32_t GPIOE_AFRL;
extern volatile uint32_t GPIOE_AFRH;

extern volatile uint32_t GPIOD_MODER;
extern volatile uint32_t GPIOD_OTYPER;
extern volatile uint32_t GPIOD_OSPEEDR;
extern volatile uint32_t GPIOD_PUPDR;
extern volatile uint32_t GPIOD_IDR;
extern volatile uint32_t GPIOD_ODR;
extern volatile uint32_t GPIOD_BSRR;
extern volatile uint32_t GPIOD_LCKR;
extern volatile uint32_t GPIOD_AFRL;
extern volatile uint32_t GPIOD_AFRH;

extern volatile uint32_t GPIOC_MODER;
extern volatile uint32_t GPIOC_OTYPER;
extern volatile uint32_t GPIOC_OSPEEDR;
extern volatile uint32_t GPIOC_PUPDR;
extern volatile uint32_t GPIOC_IDR;
extern volatile uint32_t GPIOC_ODR;
extern volatile uint32_t GPIOC_BSRR;
extern volatile uint32_t GPIOC_LCKR;
extern volatile uint32_t GPIOC_AFRL;
extern volatile uint32_t GPIOC_AFRH;

extern volatile uint32_t GPIOB_MODER;
extern volatile uint32_t GPIOB_OTYPER;
extern volatile uint32_t GPIOB_OSPEEDR;
extern volatile uint32_t GPIOB_PUPDR;
extern volatile uint32_t GPIOB_IDR;
extern volatile uint32_t GPIOB_ODR;
extern volatile uint32_t GPIOB_BSRR;
extern volatile uint32_t GPIOB_LCKR;
extern volatile uint32_t GPIOB_AFRL;
extern volatile uint32_t GPIOB_AFRH;

extern volatile uint32_t GPIOA_MODER;
extern volatile uint32_t GPIOA_OTYPER;
extern volatile uint32_t GPIOA_OSPEEDR;
extern volatile uint32_t GPIOA_PUPDR;
extern volatile uint32_t GPIOA_IDR;
extern volatile uint32_t GPIOA_ODR;
extern volatile uint32_t GPIOA_BSRR;
extern volatile uint32_t GPIOA_LCKR;
extern volatile uint32_t GPIOA_AFRL;
extern volatile uint32_t GPIOA_AFRH;

BITSS(MODER, 0, 2)
BITS(OTYPER, 0, 16)
BITSS(OSPEEDR, 0, 2)
BITSS(PUPDR, 0, 2)
BITS(IDR, 0, 16)
BITS(ODR, 0, 16)
#define GPIO_BS(i) (1 << (i))
#define GPIO_BR(i) (1 << ((i) + 16))
BITS(LCKR, 0, 16)
BITSS(AFRL, 0, 4)
BITSS(AFRH, 0, 4)



extern volatile uint32_t TIM11_CR1;
extern volatile uint32_t TIM11_SMCR;
extern volatile uint32_t TIM11_DIER;
extern volatile uint32_t TIM11_SR;
extern volatile uint32_t TIM11_EGR;
extern volatile uint32_t TIM11_CCMR1;
extern volatile uint32_t TIM11_CCER;
extern volatile uint32_t TIM11_CNT;
extern volatile uint32_t TIM11_PSC;
extern volatile uint32_t TIM11_ARR;
extern volatile uint32_t TIM11_CCR1;
extern volatile uint32_t TIM11_OR;

extern volatile uint32_t TIM10_CR1;
extern volatile uint32_t TIM10_SMCR;
extern volatile uint32_t TIM10_DIER;
extern volatile uint32_t TIM10_SR;
extern volatile uint32_t TIM10_EGR;
extern volatile uint32_t TIM10_CCMR1;
extern volatile uint32_t TIM10_CCER;
extern volatile uint32_t TIM10_CNT;
extern volatile uint32_t TIM10_PSC;
extern volatile uint32_t TIM10_ARR;
extern volatile uint32_t TIM10_CCR1;

extern volatile uint32_t TIM9_CR1;
extern volatile uint32_t TIM9_CR2;
extern volatile uint32_t TIM9_SMCR;
extern volatile uint32_t TIM9_DIER;
extern volatile uint32_t TIM9_SR;
extern volatile uint32_t TIM9_EGR;
extern volatile uint32_t TIM9_CCMR1;
extern volatile uint32_t TIM9_CCER;
extern volatile uint32_t TIM9_CNT;
extern volatile uint32_t TIM9_PSC;
extern volatile uint32_t TIM9_ARR;
extern volatile uint32_t TIM9_CCR1;
extern volatile uint32_t TIM9_CCR2;

extern volatile uint32_t EXTI_IMR;
extern volatile uint32_t EXTI_EMR;
extern volatile uint32_t EXTI_RTSR;
extern volatile uint32_t EXTI_FTSR;
extern volatile uint32_t EXTI_SWIER;
extern volatile uint32_t EXTI_PR;

extern volatile uint32_t SYSCFG_MEMRMP;
extern volatile uint32_t SYSCFG_PMC;
extern volatile uint32_t SYSCFG_EXTICR[4];
extern volatile uint32_t SYSCFG_CMPCR;

extern volatile uint32_t SPI1_CR1;
extern volatile uint32_t SPI1_CR2;
extern volatile uint32_t SPI1_SR;
extern volatile uint32_t SPI1_DR;
extern volatile uint32_t SPI1_CRCPR;
extern volatile uint32_t SPI1_RXCRCR;
extern volatile uint32_t SPI1_TXCRCR;
extern volatile uint32_t SPI1_I2SCFGR;
extern volatile uint32_t SPI1_I2SPR;

extern volatile uint32_t ADC1_SR;
extern volatile uint32_t ADC1_CR1;
extern volatile uint32_t ADC1_CR2;
extern volatile uint32_t ADC1_SMPR1;
extern volatile uint32_t ADC1_SMPR2;
extern volatile uint32_t ADC1_JOFR1;
extern volatile uint32_t ADC1_JOFR2;
extern volatile uint32_t ADC1_JOFR3;
extern volatile uint32_t ADC1_JOFR4;
extern volatile uint32_t ADC1_HTR;
extern volatile uint32_t ADC1_LTR;
extern volatile uint32_t ADC1_SQR1;
extern volatile uint32_t ADC1_SQR2;
extern volatile uint32_t ADC1_SQR3;
extern volatile uint32_t ADC1_JSQR;
extern volatile uint32_t ADC1_JDR1;
extern volatile uint32_t ADC1_JDR2;
extern volatile uint32_t ADC1_JDR3;
extern volatile uint32_t ADC1_JDR4;
extern volatile uint32_t ADC1_DR;

extern volatile uint32_t ADC2_SR;
extern volatile uint32_t ADC2_CR1;
extern volatile uint32_t ADC2_CR2;
extern volatile uint32_t ADC2_SMPR1;
extern volatile uint32_t ADC2_SMPR2;
extern volatile uint32_t ADC2_JOFR1;
extern volatile uint32_t ADC2_JOFR2;
extern volatile uint32_t ADC2_JOFR3;
extern volatile uint32_t ADC2_JOFR4;
extern volatile uint32_t ADC2_HTR;
extern volatile uint32_t ADC2_LTR;
extern volatile uint32_t ADC2_SQR1;
extern volatile uint32_t ADC2_SQR2;
extern volatile uint32_t ADC2_SQR3;
extern volatile uint32_t ADC2_JSQR;
extern volatile uint32_t ADC2_JDR1;
extern volatile uint32_t ADC2_JDR2;
extern volatile uint32_t ADC2_JDR3;
extern volatile uint32_t ADC2_JDR4;
extern volatile uint32_t ADC2_DR;

extern volatile uint32_t ADC3_SR;
extern volatile uint32_t ADC3_CR1;
extern volatile uint32_t ADC3_CR2;
extern volatile uint32_t ADC3_SMPR1;
extern volatile uint32_t ADC3_SMPR2;
extern volatile uint32_t ADC3_JOFR1;
extern volatile uint32_t ADC3_JOFR2;
extern volatile uint32_t ADC3_JOFR3;
extern volatile uint32_t ADC3_JOFR4;
extern volatile uint32_t ADC3_HTR;
extern volatile uint32_t ADC3_LTR;
extern volatile uint32_t ADC3_SQR1;
extern volatile uint32_t ADC3_SQR2;
extern volatile uint32_t ADC3_SQR3;
extern volatile uint32_t ADC3_JSQR;
extern volatile uint32_t ADC3_JDR1;
extern volatile uint32_t ADC3_JDR2;
extern volatile uint32_t ADC3_JDR3;
extern volatile uint32_t ADC3_JDR4;
extern volatile uint32_t ADC3_DR;

extern volatile uint32_t ADC_CSR;
extern volatile uint32_t ADC_CCR;
extern volatile uint32_t ADC_CDR;

extern volatile uint32_t TIM8_CR1;
extern volatile uint32_t TIM8_CR2;
extern volatile uint32_t TIM8_SMCR;
extern volatile uint32_t TIM8_DIER;
extern volatile uint32_t TIM8_SR;
extern volatile uint32_t TIM8_EGR;
extern volatile uint32_t TIM8_CCMR1;
extern volatile uint32_t TIM8_CCMR2;
extern volatile uint32_t TIM8_CCER;
extern volatile uint32_t TIM8_CNT;
extern volatile uint32_t TIM8_PSC;
extern volatile uint32_t TIM8_ARR;
extern volatile uint32_t TIM8_RCR;
extern volatile uint32_t TIM8_CCR1;
extern volatile uint32_t TIM8_CCR2;
extern volatile uint32_t TIM8_CCR3;
extern volatile uint32_t TIM8_CCR4;
extern volatile uint32_t TIM8_BDTR;
extern volatile uint32_t TIM8_DCR;
extern volatile uint32_t TIM8_DMAR;

extern volatile uint32_t TIM1_CR1;
extern volatile uint32_t TIM1_CR2;
extern volatile uint32_t TIM1_SMCR;
extern volatile uint32_t TIM1_DIER;
extern volatile uint32_t TIM1_SR;
extern volatile uint32_t TIM1_EGR;
extern volatile uint32_t TIM1_CCMR1;
extern volatile uint32_t TIM1_CCMR2;
extern volatile uint32_t TIM1_CCER;
extern volatile uint32_t TIM1_CNT;
extern volatile uint32_t TIM1_PSC;
extern volatile uint32_t TIM1_ARR;
extern volatile uint32_t TIM1_RCR;
extern volatile uint32_t TIM1_CCR1;
extern volatile uint32_t TIM1_CCR2;
extern volatile uint32_t TIM1_CCR3;
extern volatile uint32_t TIM1_CCR4;
extern volatile uint32_t TIM1_BDTR;
extern volatile uint32_t TIM1_DCR;
extern volatile uint32_t TIM1_DMAR;

extern volatile uint32_t PWR_CR;
extern volatile uint32_t PWR_CSR;

extern volatile uint32_t SPI3_CR1;
extern volatile uint32_t SPI3_CR2;
extern volatile uint32_t SPI3_SR;
extern volatile uint32_t SPI3_DR;
extern volatile uint32_t SPI3_CRCPR;
extern volatile uint32_t SPI3_RXCRCR;
extern volatile uint32_t SPI3_TXCRCR;
extern volatile uint32_t SPI3_I2SCFGR;
extern volatile uint32_t SPI3_I2SPR;

extern volatile uint32_t SPI2_CR1;
extern volatile uint32_t SPI2_CR2;
extern volatile uint32_t SPI2_SR;
extern volatile uint32_t SPI2_DR;
extern volatile uint32_t SPI2_CRCPR;
extern volatile uint32_t SPI2_RXCRCR;
extern volatile uint32_t SPI2_TXCRCR;
extern volatile uint32_t SPI2_I2SCFGR;
extern volatile uint32_t SPI2_I2SPR;

extern volatile uint32_t TIM14_CR1;
extern volatile uint32_t TIM14_SMCR;
extern volatile uint32_t TIM14_DIER;
extern volatile uint32_t TIM14_SR;
extern volatile uint32_t TIM14_EGR;
extern volatile uint32_t TIM14_CCMR1;
extern volatile uint32_t TIM14_CCER;
extern volatile uint32_t TIM14_CNT;
extern volatile uint32_t TIM14_PSC;
extern volatile uint32_t TIM14_ARR;
extern volatile uint32_t TIM14_CCR1;

extern volatile uint32_t TIM13_CR1;
extern volatile uint32_t TIM13_SMCR;
extern volatile uint32_t TIM13_DIER;
extern volatile uint32_t TIM13_SR;
extern volatile uint32_t TIM13_EGR;
extern volatile uint32_t TIM13_CCMR1;
extern volatile uint32_t TIM13_CCER;
extern volatile uint32_t TIM13_CNT;
extern volatile uint32_t TIM13_PSC;
extern volatile uint32_t TIM13_ARR;
extern volatile uint32_t TIM13_CCR1;

extern volatile uint32_t TIM12_CR1;
extern volatile uint32_t TIM12_CR2;
extern volatile uint32_t TIM12_SMCR;
extern volatile uint32_t TIM12_DIER;
extern volatile uint32_t TIM12_SR;
extern volatile uint32_t TIM12_EGR;
extern volatile uint32_t TIM12_CCMR1;
extern volatile uint32_t TIM12_CCER;
extern volatile uint32_t TIM12_CNT;
extern volatile uint32_t TIM12_PSC;
extern volatile uint32_t TIM12_ARR;
extern volatile uint32_t TIM12_CCR1;
extern volatile uint32_t TIM12_CCR2;

extern volatile uint32_t TIM7_CR1;
extern volatile uint32_t TIM7_CR2;
extern volatile uint32_t TIM7_DIER;
extern volatile uint32_t TIM7_SR;
extern volatile uint32_t TIM7_EGR;
extern volatile uint32_t TIM7_CNT;
extern volatile uint32_t TIM7_PSC;
extern volatile uint32_t TIM7_ARR;

extern volatile uint32_t TIM6_CR1;
extern volatile uint32_t TIM6_CR2;
extern volatile uint32_t TIM6_DIER;
extern volatile uint32_t TIM6_SR;
extern volatile uint32_t TIM6_EGR;
extern volatile uint32_t TIM6_CNT;
extern volatile uint32_t TIM6_PSC;
extern volatile uint32_t TIM6_ARR;

extern volatile uint32_t TIM5_CR1;
extern volatile uint32_t TIM5_CR2;
extern volatile uint32_t TIM5_SMCR;
extern volatile uint32_t TIM5_DIER;
extern volatile uint32_t TIM5_SR;
extern volatile uint32_t TIM5_EGR;
extern volatile uint32_t TIM5_CCMR1;
extern volatile uint32_t TIM5_CCMR2;
extern volatile uint32_t TIM5_CCER;
extern volatile uint32_t TIM5_CNT;
extern volatile uint32_t TIM5_PSC;
extern volatile uint32_t TIM5_ARR;
extern volatile uint32_t TIM5_CCR1;
extern volatile uint32_t TIM5_CCR2;
extern volatile uint32_t TIM5_CCR3;
extern volatile uint32_t TIM5_CCR4;
extern volatile uint32_t TIM5_DCR;
extern volatile uint32_t TIM5_DMAR;
extern volatile uint32_t TIM5_OR;

extern volatile uint32_t TIM4_CR1;
extern volatile uint32_t TIM4_CR2;
extern volatile uint32_t TIM4_SMCR;
extern volatile uint32_t TIM4_DIER;
extern volatile uint32_t TIM4_SR;
extern volatile uint32_t TIM4_EGR;
extern volatile uint32_t TIM4_CCMR1;
extern volatile uint32_t TIM4_CCMR2;
extern volatile uint32_t TIM4_CCER;
extern volatile uint32_t TIM4_CNT;
extern volatile uint32_t TIM4_PSC;
extern volatile uint32_t TIM4_ARR;
extern volatile uint32_t TIM4_CCR1;
extern volatile uint32_t TIM4_CCR2;
extern volatile uint32_t TIM4_CCR3;
extern volatile uint32_t TIM4_CCR4;
extern volatile uint32_t TIM4_DCR;
extern volatile uint32_t TIM4_DMAR;

extern volatile uint32_t TIM3_CR1;
extern volatile uint32_t TIM3_CR2;
extern volatile uint32_t TIM3_SMCR;
extern volatile uint32_t TIM3_DIER;
extern volatile uint32_t TIM3_SR;
extern volatile uint32_t TIM3_EGR;
extern volatile uint32_t TIM3_CCMR1;
extern volatile uint32_t TIM3_CCMR2;
extern volatile uint32_t TIM3_CCER;
extern volatile uint32_t TIM3_CNT;
extern volatile uint32_t TIM3_PSC;
extern volatile uint32_t TIM3_ARR;
extern volatile uint32_t TIM3_CCR1;
extern volatile uint32_t TIM3_CCR2;
extern volatile uint32_t TIM3_CCR3;
extern volatile uint32_t TIM3_CCR4;
extern volatile uint32_t TIM3_DCR;
extern volatile uint32_t TIM3_DMAR;

extern volatile uint32_t TIM2_CR1;
extern volatile uint32_t TIM2_CR2;
extern volatile uint32_t TIM2_SMCR;
extern volatile uint32_t TIM2_DIER;
extern volatile uint32_t TIM2_SR;
extern volatile uint32_t TIM2_EGR;
extern volatile uint32_t TIM2_CCMR1;
extern volatile uint32_t TIM2_CCMR2;
extern volatile uint32_t TIM2_CCER;
extern volatile uint32_t TIM2_CNT;
extern volatile uint32_t TIM2_PSC;
extern volatile uint32_t TIM2_ARR;
extern volatile uint32_t TIM2_CCR1;
extern volatile uint32_t TIM2_CCR2;
extern volatile uint32_t TIM2_CCR3;
extern volatile uint32_t TIM2_CCR4;
extern volatile uint32_t TIM2_DCR;
extern volatile uint32_t TIM2_DMAR;
extern volatile uint32_t TIM2_OR;

extern volatile uint32_t U_ID_L;
extern volatile uint32_t U_ID_M;
extern volatile uint32_t U_ID_H;
extern volatile uint32_t FLASH_SIZE;

extern volatile uint32_t SCS_ACTLR;
extern volatile uint32_t SCS_STCSR;
extern volatile uint32_t SCS_STRVR;
extern volatile uint32_t SCS_STCVR;
extern volatile uint32_t SCS_STCR;
extern volatile uint32_t SCS_CPUID;
extern volatile uint32_t SCS_ICSR;
extern volatile uint32_t SCS_VTOR;
extern volatile uint32_t SCS_AIRCR;
extern volatile uint32_t SCS_SCR;
extern volatile uint32_t SCS_CCR;
extern volatile uint32_t SCS_SHPR1;
extern volatile uint32_t SCS_SHPR2;
extern volatile uint32_t SCS_SHPR3;
extern volatile uint32_t SCS_SHCSR;
extern volatile uint32_t SCS_CFSR;
extern volatile uint32_t SCS_HFSR;
extern volatile uint32_t SCS_DFSR;
extern volatile uint32_t SCS_MMFAR;
extern volatile uint32_t SCS_BFAR;
extern volatile uint32_t SCS_AFSR;
extern volatile uint32_t SCS_ID_PFR0;
extern volatile uint32_t SCS_ID_PFR1;
extern volatile uint32_t SCS_ID_DFR0;
extern volatile uint32_t SCS_ID_AFR0;
extern volatile uint32_t SCS_MMFR0;
extern volatile uint32_t SCS_MMFR1;
extern volatile uint32_t SCS_MMFR2;
extern volatile uint32_t SCS_MMFR3;
extern volatile uint32_t SCS_ISAR0;
extern volatile uint32_t SCS_ISAR1;
extern volatile uint32_t SCS_ISAR2;
extern volatile uint32_t SCS_ISAR3;
extern volatile uint32_t SCS_ISAR4;
extern volatile uint32_t SCS_CPACR;
extern volatile uint32_t SCS_STIR;
extern volatile uint32_t MPU_TYPE;
extern volatile uint32_t MPU_CTRL;
extern volatile uint32_t MPU_RNR;
extern volatile uint32_t MPU_RBAR;
extern volatile uint32_t MPU_RASR;
extern volatile uint32_t MPU_RBAR_A1;
extern volatile uint32_t MPU_RASR_A1;
extern volatile uint32_t MPU_RBAR_A2;
extern volatile uint32_t MPU_RASR_A2;
extern volatile uint32_t MPU_RBAR_A3;
extern volatile uint32_t MPU_RASR_A3;
extern volatile uint32_t NVIC_ICTR;
extern volatile uint32_t NVIC_ISER[8];
extern volatile uint32_t NVIC_ICER[8];
extern volatile uint32_t NVIC_ISPR[8];
extern volatile uint32_t NVIC_ICPR[8];
extern volatile uint32_t NVIC_IABR[8];
extern volatile uint32_t NVIC_IPR[60];
extern volatile uint32_t FPU_FPCCR;
extern volatile uint32_t FPU_FPCAR;
extern volatile uint32_t FPU_FPDSCR;
extern volatile uint32_t FPU_MVFR0;
extern volatile uint32_t FPU_MVFR1;

#undef BIT
#undef BITS
#undef BITSS

#endif

