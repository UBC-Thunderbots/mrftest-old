/**
 * \ingroup REG
 * \defgroup REGOTGFS USB on-the-go full speed
 * @{
 */
#ifndef STM32LIB_REGISTERS_OTG_FS_H
#define STM32LIB_REGISTERS_OTG_FS_H

#include <stdint.h>

#define OTG_FS_BASE 0x50000000

typedef struct {
	unsigned SRQSCS : 1;
	unsigned SRQ : 1;
	unsigned : 6;
	unsigned HNGSCS : 1;
	unsigned HNPRQ : 1;
	unsigned HSHNPEN : 1;
	unsigned DHNPEN : 1;
	unsigned : 4;
	unsigned CIDSTS : 1;
	unsigned DBCT : 1;
	unsigned ASVLD : 1;
	unsigned BSVLD : 1;
	unsigned : 12;
} OTG_FS_GOTGCTL_t;
_Static_assert(sizeof(OTG_FS_GOTGCTL_t) == 4U, "OTG_FS_GOTGCTL_t is wrong size");
#define OTG_FS_GOTGCTL (*(volatile OTG_FS_GOTGCTL_t *) (OTG_FS_BASE + 0x000))

typedef struct {
	unsigned : 2;
	unsigned SEDET : 1;
	unsigned : 5;
	unsigned SRSSCHG : 1;
	unsigned HNSSCHG : 1;
	unsigned : 7;
	unsigned HNGDET : 1;
	unsigned ADTOCHG : 1;
	unsigned DBCDNE : 1;
	unsigned : 12;
} OTG_FS_GOTGINT_t;
_Static_assert(sizeof(OTG_FS_GOTGINT_t) == 4U, "OTG_FS_GOTGINT_t is wrong size");
#define OTG_FS_GOTGINT (*(volatile OTG_FS_GOTGINT_t *) (OTG_FS_BASE + 0x004))

typedef struct {
	unsigned GINTMSK : 1;
	unsigned : 6;
	unsigned TXFELVL : 1;
	unsigned PTXFELVL : 1;
	unsigned : 23;
} OTG_FS_GAHBCFG_t;
_Static_assert(sizeof(OTG_FS_GAHBCFG_t) == 4U, "OTG_FS_GAHBCFG_t is wrong size");
#define OTG_FS_GAHBCFG (*(volatile OTG_FS_GAHBCFG_t *) (OTG_FS_BASE + 0x008))

typedef struct {
	unsigned TOCAL : 3;
	unsigned : 3;
	unsigned PHYSEL : 1;
	unsigned : 1;
	unsigned SRPCAP : 1;
	unsigned HNPCAP : 1;
	unsigned TRDT : 4;
	unsigned : 15;
	unsigned FHMOD : 1;
	unsigned FDMOD : 1;
	unsigned CTXPKT : 1;
} OTG_FS_GUSBCFG_t;
_Static_assert(sizeof(OTG_FS_GUSBCFG_t) == 4U, "OTG_FS_GUSBCFG_t is wrong size");
#define OTG_FS_GUSBCFG (*(volatile OTG_FS_GUSBCFG_t *) (OTG_FS_BASE + 0x00C))

typedef struct {
	unsigned CSRST : 1;
	unsigned HSRST : 1;
	unsigned FCRST : 1;
	unsigned : 1;
	unsigned RXFFLSH : 1;
	unsigned TXFFLSH : 1;
	unsigned TXFNUM : 5;
	unsigned : 20;
	unsigned AHBIDL : 1;
} OTG_FS_GRSTCTL_t;
_Static_assert(sizeof(OTG_FS_GRSTCTL_t) == 4U, "OTG_FS_GRSTCTL_t is wrong size");
#define OTG_FS_GRSTCTL (*(volatile OTG_FS_GRSTCTL_t *) (OTG_FS_BASE + 0x010))

typedef struct {
	unsigned CMOD : 1;
	unsigned MMIS : 1;
	unsigned OTGINT : 1;
	unsigned SOF : 1;
	unsigned RXFLVL : 1;
	unsigned NPTXFE : 1;
	unsigned GINAKEFF : 1;
	unsigned GOUTNAKEFF : 1;
	unsigned : 2;
	unsigned ESUSP : 1;
	unsigned USBSUSP : 1;
	unsigned USBRST : 1;
	unsigned ENUMDNE : 1;
	unsigned ISOODRP : 1;
	unsigned EOPF : 1;
	unsigned : 2;
	unsigned IEPINT : 1;
	unsigned OEPINT : 1;
	unsigned IISOIXFR : 1;
	unsigned IPXFR_INCOMPISOOUT : 1;
	unsigned : 2;
	unsigned HPRTINT : 1;
	unsigned HCINT : 1;
	unsigned PTXFE : 1;
	unsigned : 1;
	unsigned CIDSCHG : 1;
	unsigned DISCINT : 1;
	unsigned SRQINT : 1;
	unsigned WKUINT : 1;
} OTG_FS_GINTSTS_t;
_Static_assert(sizeof(OTG_FS_GINTSTS_t) == 4U, "OTG_FS_GINTSTS_t is wrong size");
#define OTG_FS_GINTSTS (*(volatile OTG_FS_GINTSTS_t *) (OTG_FS_BASE + 0x014))

typedef struct {
	unsigned : 1;
	unsigned MMISM : 1;
	unsigned OTGINT : 1;
	unsigned SOFM : 1;
	unsigned RXFLVLM : 1;
	unsigned NPTXFEM : 1;
	unsigned GINAKEFFM : 1;
	unsigned GONAKEFFM : 1;
	unsigned : 2;
	unsigned ESUSPM : 1;
	unsigned USBSUSPM : 1;
	unsigned USBRST : 1;
	unsigned ENUMDNEM : 1;
	unsigned ISOODRPM : 1;
	unsigned EOPFM : 1;
	unsigned : 1;
	unsigned EPMISM : 1;
	unsigned IEPINT : 1;
	unsigned OEPINT : 1;
	unsigned IISOIXFRM : 1;
	unsigned IPXFRM_IISOOXFRM : 1;
	unsigned : 2;
	unsigned PRTIM : 1;
	unsigned HCIM : 1;
	unsigned PTXFEM : 1;
	unsigned : 1;
	unsigned CIDSCHGM : 1;
	unsigned DISCINT : 1;
	unsigned SRQIM : 1;
	unsigned WKUIM : 1;
} OTG_FS_GINTMSK_t;
_Static_assert(sizeof(OTG_FS_GINTMSK_t) == 4U, "OTG_FS_GINTMSK_t is wrong size");
#define OTG_FS_GINTMSK (*(volatile OTG_FS_GINTMSK_t *) (OTG_FS_BASE + 0x018))

typedef struct {
	unsigned CHNUM : 4;
	unsigned BCNT : 11;
	unsigned DPID : 2;
	unsigned PKTSTS : 4;
	unsigned : 11;
} OTG_FS_GRXSTSR_host_t;
_Static_assert(sizeof(OTG_FS_GRXSTSR_host_t) == 4U, "OTG_FS_GRXSTSR_host_t is wrong size");
typedef struct {
	unsigned EPNUM : 4;
	unsigned BCNT : 11;
	unsigned DPID : 2;
	unsigned PKTSTS : 4;
	unsigned FRMNUM : 4;
	unsigned : 7;
} OTG_FS_GRXSTSR_device_t;
_Static_assert(sizeof(OTG_FS_GRXSTSR_device_t) == 4U, "OTG_FS_GRXSTSR_device_t is wrong size");
typedef union {
	OTG_FS_GRXSTSR_host_t host;
	OTG_FS_GRXSTSR_device_t device;
} OTG_FS_GRXSTSR_t;
_Static_assert(sizeof(OTG_FS_GRXSTSR_t) == 4U, "OTG_FS_GRXSTSR_t is wrong size");
#define OTG_FS_GRXSTSR (*(volatile OTG_FS_GRXSTSR_t *) (OTG_FS_BASE + 0x01C))
#define OTG_FS_GRXSTSP (*(volatile OTG_FS_GRXSTSR_t *) (OTG_FS_BASE + 0x020))

typedef struct {
	unsigned RXFD : 16;
	unsigned : 16;
} OTG_FS_GRXFSIZ_t;
_Static_assert(sizeof(OTG_FS_GRXFSIZ_t) == 4U, "OTG_FS_GRXFSIZ_t is wrong size");
#define OTG_FS_GRXFSIZ (*(volatile OTG_FS_GRXFSIZ_t *) (OTG_FS_BASE + 0x024))

typedef struct {
	unsigned NPTXFSA : 16;
	unsigned NPTXFD : 16;
} OTG_FS_HNPTXFSIZ_t;
_Static_assert(sizeof(OTG_FS_HNPTXFSIZ_t) == 4U, "OTG_FS_HNPTXFSIZ_t is wrong size");
#define OTG_FS_HNPTXFSIZ (*(volatile OTG_FS_HNPTXFSIZ_t *) (OTG_FS_BASE + 0x028))

typedef struct {
	unsigned TX0FSA : 16;
	unsigned TX0FD : 16;
} OTG_FS_DIEPTXF0_t;
_Static_assert(sizeof(OTG_FS_DIEPTXF0_t) == 4U, "OTG_FS_DIEPTXF0_t is wrong size");
#define OTG_FS_DIEPTXF0 (*(volatile OTG_FS_DIEPTXF0_t *) (OTG_FS_BASE + 0x028))

typedef struct {
	unsigned NPTXFSAV : 16;
	unsigned NTPQXSAV : 8;
	unsigned NPTXQTOP : 7;
	unsigned : 1;
} OTG_FS_HNPTXSTS_t;
_Static_assert(sizeof(OTG_FS_HNPTXSTS_t) == 4U, "OTG_FS_HNPTXSTS_t is wrong size");
#define OTG_FS_HNPTXSTS (*(volatile OTG_FS_HNPTXSTS_t *) (OTG_FS_BASE + 0x02C))

typedef struct {
	unsigned : 16;
	unsigned PWRDWN : 1;
	unsigned : 1;
	unsigned VBUSASEN : 1;
	unsigned VBUSBSEN : 1;
	unsigned SOFOUTEN : 1;
	unsigned NOVBUSSENS : 1;
	unsigned : 10;
} OTG_FS_GCCFG_t;
_Static_assert(sizeof(OTG_FS_GCCFG_t) == 4U, "OTG_FS_GCCFG_t is wrong size");
#define OTG_FS_GCCFG (*(volatile OTG_FS_GCCFG_t *) (OTG_FS_BASE + 0x038))

#define OTG_FS_CID (*(volatile uint32_t *) (OTG_FS_BASE + 0x03C))

typedef struct {
	unsigned PTXSA : 16;
	unsigned PTXFSIZ : 16;
} OTG_FS_HPTXFSIZ_t;
_Static_assert(sizeof(OTG_FS_HPTXFSIZ_t) == 4U, "OTG_FS_HPTXFSIZ_t is wrong size");
#define OTG_FS_HPTXFSIZ (*(volatile OTG_FS_HPTXFSIZ_t *) (OTG_FS_BASE + 0x100))

typedef struct {
	unsigned INEPTXSA : 16;
	unsigned INEPTXFD : 16;
} OTG_FS_DIEPTXFx_t;
_Static_assert(sizeof(OTG_FS_DIEPTXFx_t) == 4U, "OTG_FS_DIEPTXFx_t is wrong size");
// this array is one-based and therefore the first element is at 0x104
#define OTG_FS_DIEPTXF ((volatile OTG_FS_DIEPTXFx_t *) (OTG_FS_BASE + 0x100))

typedef struct {
	unsigned DSPD : 2;
	unsigned NZLSOHSK : 1;
	unsigned : 1;
	unsigned DAD : 7;
	unsigned PFIVL : 2;
	unsigned : 19;
} OTG_FS_DCFG_t;
_Static_assert(sizeof(OTG_FS_DCFG_t) == 4U, "OTG_FS_DCFG_t is wrong size");
#define OTG_FS_DCFG (*(volatile OTG_FS_DCFG_t *) (OTG_FS_BASE + 0x800))

typedef struct {
	unsigned RWUSIG : 1;
	unsigned SDIS : 1;
	unsigned GINSTS : 1;
	unsigned GONSTS : 1;
	unsigned TCTL : 3;
	unsigned SGINAK : 1;
	unsigned CGINAK : 1;
	unsigned SGONAK : 1;
	unsigned CGONAK : 1;
	unsigned POPRGDNE : 1;
	unsigned : 20;
} OTG_FS_DCTL_t;
_Static_assert(sizeof(OTG_FS_DCTL_t) == 4U, "OTG_FS_DCTL_t is wrong size");
#define OTG_FS_DCTL (*(volatile OTG_FS_DCTL_t *) (OTG_FS_BASE + 0x804))

typedef struct {
	unsigned SUSPSTS : 1;
	unsigned ENUMSPD : 2;
	unsigned EERR : 1;
	unsigned : 4;
	unsigned FNSOF : 14;
	unsigned : 10;
} OTG_FS_DSTS_t;
_Static_assert(sizeof(OTG_FS_DSTS_t) == 4U, "OTG_FS_DSTS_t is wrong size");
#define OTG_FS_DSTS (*(volatile OTG_FS_DSTS_t *) (OTG_FS_BASE + 0x808))

typedef struct {
	unsigned XFRCM : 1;
	unsigned EPDM : 1;
	unsigned : 1;
	unsigned TOM : 1;
	unsigned ITTXFEMSK : 1;
	unsigned INEPNMM : 1;
	unsigned INEPNEM : 1;
	unsigned : 25;
} OTG_FS_DIEPMSK_t;
_Static_assert(sizeof(OTG_FS_DIEPMSK_t) == 4U, "OTG_FS_DIEPMSK_t is wrong size");
#define OTG_FS_DIEPMSK (*(volatile OTG_FS_DIEPMSK_t *) (OTG_FS_BASE + 0x810))

typedef struct {
	unsigned XFRCM : 1;
	unsigned EPDM : 1;
	unsigned : 1;
	unsigned STUPM : 1;
	unsigned OTEPDM : 1;
	unsigned : 27;
} OTG_FS_DOEPMSK_t;
_Static_assert(sizeof(OTG_FS_DOEPMSK_t) == 4U, "OTG_FS_DOEPMSK_t is wrong size");
#define OTG_FS_DOEPMSK (*(volatile OTG_FS_DOEPMSK_t *) (OTG_FS_BASE + 0x814))

typedef struct {
	unsigned IEPINT : 16;
	unsigned OEPINT : 16;
} OTG_FS_DAINT_t;
_Static_assert(sizeof(OTG_FS_DAINT_t) == 4U, "OTG_FS_DAINT_t is wrong size");
#define OTG_FS_DAINT (*(volatile OTG_FS_DAINT_t *) (OTG_FS_BASE + 0x818))

typedef struct {
	unsigned IEPM : 16;
	unsigned OEPM : 16;
} OTG_FS_DAINTMSK_t;
_Static_assert(sizeof(OTG_FS_DAINTMSK_t) == 4U, "OTG_FS_DAINTMSK_t is wrong size");
#define OTG_FS_DAINTMSK (*(volatile OTG_FS_DAINTMSK_t *) (OTG_FS_BASE + 0x81C))

typedef struct {
	unsigned VBUSDT : 16;
	unsigned : 16;
} OTG_FS_DVBUSDIS_t;
_Static_assert(sizeof(OTG_FS_DVBUSDIS_t) == 4U, "OTG_FS_DVBUSDIS_t is wrong size");
#define OTG_FS_DVBUSDIS (*(volatile OTG_FS_DVBUSDIS_t *) (OTG_FS_BASE + 0x828))

typedef struct {
	unsigned DVBUSP : 12;
	unsigned : 20;
} OTG_FS_DVBUSPULSE_t;
_Static_assert(sizeof(OTG_FS_DVBUSPULSE_t) == 4U, "OTG_FS_DVBUSPULSE_t is wrong size");
#define OTG_FS_DVBUSPULSE (*(volatile OTG_FS_DVBUSPULSE_t *) (OTG_FS_BASE + 0x82C))

typedef struct {
	unsigned INEPTXFEM : 16;
	unsigned : 16;
} OTG_FS_DIEPEMPMSK_t;
_Static_assert(sizeof(OTG_FS_DIEPEMPMSK_t) == 4U, "OTG_FS_DIEPEMPMSK_t is wrong size");
#define OTG_FS_DIEPEMPMSK (*(volatile OTG_FS_DIEPEMPMSK_t *) (OTG_FS_BASE + 0x834))

typedef struct {
	unsigned MPSIZ : 2;
	unsigned : 13;
	unsigned USBAEP : 1;
	unsigned : 1;
	unsigned NAKSTS : 1;
	unsigned EPTYP : 2;
	unsigned : 1;
	unsigned STALL : 1;
	unsigned TXFNUM : 4;
	unsigned CNAK : 1;
	unsigned SNAK : 1;
	unsigned : 2;
	unsigned EPDIS : 1;
	unsigned EPENA : 1;
} OTG_FS_DIEPCTL0_t;
_Static_assert(sizeof(OTG_FS_DIEPCTL0_t) == 4U, "OTG_FS_DIEPCTL0_t is wrong size");
#define OTG_FS_DIEPCTL0 (*(volatile OTG_FS_DIEPCTL0_t *) (OTG_FS_BASE + 0x900))

typedef struct {
	unsigned XFRSIZ : 7;
	unsigned : 12;
	unsigned PKTCNT : 2;
	unsigned : 11;
} OTG_FS_DIEPTSIZ0_t;
_Static_assert(sizeof(OTG_FS_DIEPTSIZ0_t) == 4U, "OTG_FS_DIEPTSIZ0_t is wrong size");
#define OTG_FS_DIEPTSIZ0 (*(volatile OTG_FS_DIEPTSIZ0_t *) (OTG_FS_BASE + 0x910))

typedef struct {
	unsigned MPSIZ : 11;
	unsigned : 4;
	unsigned USBAEP : 1;
	unsigned EONUM_DPID : 1;
	unsigned NAKSTS : 1;
	unsigned EPTYP : 2;
	unsigned : 1;
	unsigned STALL : 1;
	unsigned TXFNUM : 4;
	unsigned CNAK : 1;
	unsigned SNAK : 1;
	unsigned SD0PID_SEVNFRM : 1;
	unsigned SODDFRM : 1;
	unsigned EPDIS : 1;
	unsigned EPENA : 1;
} OTG_FS_DIEPCTLx_t;
_Static_assert(sizeof(OTG_FS_DIEPCTLx_t) == 4U, "OTG_FS_DIEPCTLx_t is wrong size");
typedef struct {
	unsigned XFRC : 1;
	unsigned EPDISD : 1;
	unsigned : 1;
	unsigned TOC : 1;
	unsigned ITTXFE : 1;
	unsigned : 1;
	unsigned INEPNE : 1;
	unsigned TXFE : 1;
	unsigned : 24;
} OTG_FS_DIEPINTx_t;
_Static_assert(sizeof(OTG_FS_DIEPINTx_t) == 4U, "OTG_FS_DIEPINTx_t is wrong size");
typedef struct {
	unsigned XFRSIZ : 19;
	unsigned PKTCNT : 10;
	unsigned MCNT : 2;
	unsigned : 1;
} OTG_FS_DIEPTSIZx_t;
_Static_assert(sizeof(OTG_FS_DIEPTSIZx_t) == 4U, "OTG_FS_DIEPTSIZx_t is wrong size");
typedef struct {
	unsigned INEPTFSAV : 16;
	unsigned : 16;
} OTG_FS_DTXFSTSx_t;
_Static_assert(sizeof(OTG_FS_DTXFSTSx_t) == 4U, "OTG_FS_DTXFSTSx_t is wrong size");
typedef struct {
	OTG_FS_DIEPCTLx_t DIEPCTL;
	uint32_t res1;
	OTG_FS_DIEPINTx_t DIEPINT;
	uint32_t res3;
	OTG_FS_DIEPTSIZx_t DIEPTSIZ;
	uint32_t res5;
	OTG_FS_DTXFSTSx_t DTXFSTS;
	uint32_t res7;
} OTG_FS_DIEPx_wrapper_t;
_Static_assert(sizeof(OTG_FS_DIEPx_wrapper_t) == 0x20U, "OTG_FS_DIEPx_wrapper_t is wrong size");
// this array is one-based and therefore the first element is at 0x920
#define OTG_FS_DIEP ((volatile OTG_FS_DIEPx_wrapper_t *) (OTG_FS_BASE + 0x900))

typedef struct {
	unsigned MPSIZ : 2;
	unsigned : 13;
	unsigned USBAEP : 1;
	unsigned : 1;
	unsigned NAKSTS : 1;
	unsigned EPTYP : 2;
	unsigned SNPM : 1;
	unsigned STALL : 1;
	unsigned : 4;
	unsigned CNAK : 1;
	unsigned SNAK : 1;
	unsigned : 2;
	unsigned EPDIS : 1;
	unsigned EPENA : 1;
} OTG_FS_DOEPCTL0_t;
_Static_assert(sizeof(OTG_FS_DOEPCTL0_t) == 4U, "OTG_FS_DOEPCTL0_t is wrong size");
#define OTG_FS_DOEPCTL0 (*(volatile OTG_FS_DOEPCTL0_t *) (OTG_FS_BASE + 0xB00))

typedef struct {
	unsigned XFRSIZ : 7;
	unsigned : 12;
	unsigned PKTCNT : 1;
	unsigned : 9;
	unsigned STUPCNT : 2;
	unsigned : 1;
} OTG_FS_DOEPTSIZ0_t;
_Static_assert(sizeof(OTG_FS_DOEPTSIZ0_t) == 4U, "OTG_FS_DOEPTSIZ0_t is wrong size");
#define OTG_FS_DOEPTSIZ0 (*(volatile OTG_FS_DOEPTSIZ0_t *) (OTG_FS_BASE + 0xB10))

typedef struct {
	unsigned MPSIZ : 11;
	unsigned : 4;
	unsigned USBAEP : 1;
	unsigned EONUM_DPID : 1;
	unsigned NAKSTS : 1;
	unsigned EPTYP : 2;
	unsigned SNPM : 1;
	unsigned STALL : 1;
	unsigned : 4;
	unsigned CNAK : 1;
	unsigned SNAK : 1;
	unsigned SD0PID_SEVENFRM : 1;
	unsigned SODDFRM_SD1PID : 1;
	unsigned EPDIS : 1;
	unsigned EPENA : 1;
} OTG_FS_DOEPCTLx_t;
_Static_assert(sizeof(OTG_FS_DOEPCTLx_t) == 4U, "OTG_FS_DOEPCTLx_t is wrong size");
typedef struct {
	unsigned XFRC : 1;
	unsigned EPDISD : 1;
	unsigned : 1;
	unsigned STUP : 1;
	unsigned OTEPDIS : 1;
	unsigned : 1;
	unsigned B2BSTUP : 1;
	unsigned : 25;
} OTG_FS_DOEPINTx_t;
_Static_assert(sizeof(OTG_FS_DOEPINTx_t) == 4U, "OTG_FS_DOEPINTx_t is wrong size");
typedef struct {
	unsigned XFRSIZ : 19;
	unsigned PKTCNT : 10;
	unsigned RXDPID_STUPCNT : 2;
	unsigned : 1;
} OTG_FS_DOEPTSIZx_t;
_Static_assert(sizeof(OTG_FS_DOEPTSIZx_t) == 4U, "OTG_FS_DOEPTSIZx_t is wrong size");
typedef struct {
	OTG_FS_DOEPCTLx_t DOEPCTL;
	uint32_t reserved1;
	OTG_FS_DOEPINTx_t DOEPINT;
	uint32_t reserved3;
	OTG_FS_DOEPTSIZx_t DOEPTSIZ;
	uint32_t reserved5;
	uint32_t reserved6;
	uint32_t reserved7;
} OTG_FS_DOEPx_wrapper_t;
_Static_assert(sizeof(OTG_FS_DOEPx_wrapper_t) == 0x20U, "OTG_FS_DOEPx_wrapper_t is wrong size");
// this array is one-based and therefore the first element is at 0xB20
#define OTG_FS_DOEP ((volatile OTG_FS_DOEPx_wrapper_t *) (OTG_FS_BASE + 0xB00))

typedef struct {
	unsigned STPPCLK : 1;
	unsigned GATEHCLK : 1;
	unsigned : 2;
	unsigned PHYSUSP : 1;
	unsigned : 27;
} OTG_FS_PCGCCTL_t;
_Static_assert(sizeof(OTG_FS_PCGCCTL_t) == 4U, "OTG_FS_PCGCCTL_t is wrong size");
#define OTG_FS_PCGCCTL (*(volatile OTG_FS_PCGCCTL_t *) (OTG_FS_BASE + 0xE00))

typedef uint32_t OTG_FS_FIFO_t[0x1000 / 4];
typedef OTG_FS_FIFO_t OTG_FS_FIFOS_t[4];
#define OTG_FS_FIFO (*(volatile OTG_FS_FIFOS_t *) (OTG_FS_BASE + 0x1000))

#endif

/**
 * @}
 */

