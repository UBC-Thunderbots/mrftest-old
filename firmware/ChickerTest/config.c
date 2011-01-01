#include <pic18fregs.h>

static const char __code __at(__CONFIG1L) c1l = _USBPLL_CLOCK_SRC_FROM_96MHZ_PLL_2_1L & _CPUDIV__OSC1_OSC2_SRC___1__96MHZ_PLL_SRC___2__1L & _PLLDIV_NO_DIVIDE__4MHZ_INPUT__1L;//SET SYSTEM SYSTEM CLOCK TO RUN AT 96MHz/2 WITH NO PRESCALER
static const char __code __at(__CONFIG1H) c1h = _OSC_XT__XT_PLL__USB_XT_1H & _FCMEN_OFF_1H & _IESO_OFF_1H;//XTPLL with no fail safe monitor
//static const char __code __at(__CONFIG1H) c1h = _OSC_XT__XT_PLL__USB_XT_1H & _FCMEN_ON_1H & _IESO_OFF_1H;//XTPLL enabled (0011) with fail safe monitor enable bit
static const char __code __at(__CONFIG2L) c2l = _VREGEN_OFF_2L & _PUT_ON_2L & _BODEN_OFF_2L;
static const char __code __at(__CONFIG2H) c2h = _WDT_DISABLED_CONTROLLED_2H;
static const char __code __at(__CONFIG3H) c3h = _CCP2MUX_RC1_3H & _PBADEN_PORTB_4_0__CONFIGURED_AS_DIGITAL_I_O_ON_RESET_3H & _MCLRE_MCLR_ON_RE3_OFF_3H;//MCLR pin on,CCP2 output mux set to RC1
static const char __code __at(__CONFIG4L) c4l = _STVR_ON_4L & _LVP_OFF_4L & _ENICPORT_OFF_4L & _ENHCPU_OFF_4L & _BACKBUG_OFF_4L;
static const char __code __at(__CONFIG5L) c5l = _CP_0_OFF_5L & _CP_1_OFF_5L & _CP_2_OFF_5L & _CP_3_OFF_5L;
static const char __code __at(__CONFIG5H) c5h = _CPD_OFF_5H & _CPB_OFF_5H;
static const char __code __at(__CONFIG6L) c6l = _WRT_0_OFF_6L & _WRT_1_OFF_6L & _WRT_2_OFF_6L & _WRT_3_OFF_6L;
static const char __code __at(__CONFIG6H) c6h = _WRTD_OFF_6H & _WRTB_OFF_6H & _WRTC_OFF_6H;
static const char __code __at(__CONFIG7L) c7l = _EBTR_0_OFF_7L & _EBTR_1_OFF_7L & _EBTR_2_OFF_7L & _EBTR_3_OFF_7L;
static const char __code __at(__CONFIG7H) c7h = _EBTRB_OFF_7H;

