#include "buzzer.h"
#include "constants.h"
#include "enabled.h"
#include "estop.h"
#include "led.h"
#include "mrf.h"
#include "normal.h"
#include "pins.h"
#include <FreeRTOS.h>
#include <build_id.h>
#include <core_progmem.h>
#include <crc32.h>
#include <exception.h>
#include <format.h>
#include <gpio.h>
#include <init.h>
#include <rcc.h>
#include <sleep.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <unused.h>
#include <usb.h>
#include <registers/id.h>
#include <registers/otg_fs.h>
#include <registers/systick.h>

static void stm32_main(void) __attribute__((noreturn));

static unsigned long mstack[1024U] __attribute__((section(".mstack")));

typedef void (*fptr)(void);
static const fptr exception_vectors[16U] __attribute__((used, section(".exception_vectors"))) = {
	[0U] = (fptr) (mstack + sizeof(mstack) / sizeof(*mstack)),
	[1U] = &stm32_main,
	[3U] = &exception_hard_fault_isr,
	[4U] = &exception_memory_manage_fault_isr,
	[5U] = &exception_bus_fault_isr,
	[6U] = &exception_usage_fault_isr,
	[11U] = &vPortSVCHandler,
	[12U] = &exception_debug_fault_isr,
	[14U] = &vPortPendSVHandler,
	[15U] = &vPortSysTickHandler,
};

static const fptr interrupt_vectors[82U] __attribute__((used, section(".interrupt_vectors"))) = {
	[18U] = &adc_isr,
	[40U] = &exti10_15_isr,
	[50U] = &timer5_isr,
	[54U] = &timer6_isr,
	[56U] = &dma2_stream0_isr,
	[67U] = &udev_isr,
};

static void app_exception_early(void) {
	// Power down the USB engine to disconnect from the host.
	OTG_FS.GCCFG.PWRDWN = 0;

	// Turn the three LEDs on.
	led_end_lamp_test_early();
	led_on(LED_POWER);
	led_on(LED_TX);
	led_on(LED_RX);
}

static void app_exception_late(bool core_written) {
	// Disable SYSTICK while changing frequency.
	{
		SYST_CSR_t tmp = { 0 };
		SYSTICK.CSR = tmp;
	}
	// Set SYSTICK to divide by 144 so it overflows every microsecond.
	SYSTICK.RVR = 144U - 1U;
	// Reset the counter.
	SYSTICK.CVR = 0U;
	// Set SYSTICK to run with the core AHB clock.
	{
		SYST_CSR_t tmp = {
			.CLKSOURCE = 1, // Use core clock
			.ENABLE = 1, // Counter is running
		};
		SYSTICK.CSR = tmp;
	}

	// Show flashing lights.
	for (;;) {
		led_off(LED_POWER);
		led_off(LED_TX);
		led_off(LED_RX);
		sleep_ms(500U);
		led_on(LED_POWER);
		if (core_written) {
			led_on(LED_TX);
			led_on(LED_RX);
		}
		sleep_ms(500U);
	}
}

static const init_specs_t INIT_SPECS = {
	.flags = {
		.hse_crystal = true,
		.freertos = true,
		.io_compensation_cell = false,
	},
	.hse_frequency = 8,
	.pll_frequency = 288,
	.sys_frequency = 144,
	.cpu_frequency = 144,
	.apb1_frequency = 36,
	.apb2_frequency = 72,
	.exception_core_writer = &core_progmem_writer,
	.exception_app_cbs = {
		.early = &app_exception_early,
		.late = &app_exception_late,
	},
};

static const usb_string_descriptor_t STRING_EN_CA_MANUFACTURER = USB_STRING_DESCRIPTOR_INITIALIZER(u"UBC Thunderbots Football Club");
static const usb_string_descriptor_t STRING_EN_CA_PRODUCT = USB_STRING_DESCRIPTOR_INITIALIZER(u"Radio Base Station");
static const usb_string_descriptor_t STRING_EN_CA_RADIO_OFF = USB_STRING_DESCRIPTOR_INITIALIZER(u"Radio Off");
static const usb_string_descriptor_t STRING_EN_CA_NORMAL = USB_STRING_DESCRIPTOR_INITIALIZER(u"Normal Mode");
static const usb_string_descriptor_t STRING_EN_CA_PROMISCUOUS = USB_STRING_DESCRIPTOR_INITIALIZER(u"Promiscuous Mode");
static usb_string_descriptor_t STRING_SERIAL = USB_STRING_DESCRIPTOR_INITIALIZER(u"                        ");
static const usb_string_descriptor_t * const STRINGS_EN_CA[] = {
	[STRING_INDEX_MANUFACTURER - 1U] = &STRING_EN_CA_MANUFACTURER,
	[STRING_INDEX_PRODUCT - 1U] = &STRING_EN_CA_PRODUCT,
	[STRING_INDEX_RADIO_OFF - 1U] = &STRING_EN_CA_RADIO_OFF,
	[STRING_INDEX_NORMAL - 1U] = &STRING_EN_CA_NORMAL,
	[STRING_INDEX_PROMISCUOUS - 1U] = &STRING_EN_CA_PROMISCUOUS,
	[STRING_INDEX_SERIAL - 1U] = &STRING_SERIAL,
};
static const udev_language_info_t LANGUAGE_TABLE[] = {
	{ .id = 0x1009U /* en_CA */, .strings = STRINGS_EN_CA },
	{ .id = 0, .strings = 0 },
};
static const usb_string_zero_descriptor_t STRING_ZERO = {
	.bLength = sizeof(usb_string_zero_descriptor_t) + sizeof(uint16_t),
	.bDescriptorType = USB_DTYPE_STRING,
	.wLANGID = { 0x1009U /* en_CA */, },
};

static bool usb_control_handler(const usb_setup_packet_t *pkt) {
	if (pkt->bmRequestType.recipient == USB_RECIPIENT_DEVICE && pkt->bmRequestType.type == USB_CTYPE_VENDOR && pkt->bRequest == CONTROL_REQUEST_READ_CORE && pkt->wValue < 256U && !pkt->wIndex && pkt->wLength == 1024U) {
		uep0_data_write(&core_progmem_dump[pkt->wValue * 1024U / 4U], 1024U);
		return true;
	} else if (pkt->bmRequestType.recipient == USB_RECIPIENT_DEVICE && pkt->bmRequestType.type == USB_CTYPE_VENDOR && pkt->bRequest == CONTROL_REQUEST_READ_BUILD_ID && !pkt->wValue && !pkt->wIndex && pkt->wLength == 4U) {
		uint32_t id = build_id_get();
		uep0_data_write(&id, sizeof(id));
		return true;
	}
	return false;
}

static const udev_info_t USB_INFO = {
	.flags = {
		.vbus_sensing = 0,
		.minimize_interrupts = 0,
		.self_powered = 0,
	},
	.internal_task_priority = 4U,
	.internal_task_stack_size = 1024U,
	.isr_priority = EXCEPTION_MKPRIO(5U, 0U),
	.receive_fifo_words = 10U /* SETUP packets */ + 1U /* Global OUT NAK status */ + ((64U / 4U) + 1U) * 2U /* Packets */ + 4U /* Transfer complete status */,
	.device_descriptor = {
		.bLength = sizeof(usb_device_descriptor_t),
		.bDescriptorType = USB_DTYPE_DEVICE,
		.bcdUSB = 0x0200U,
		.bDeviceClass = 0x00U,
		.bDeviceSubClass = 0x00U,
		.bDeviceProtocol = 0x00U,
		.bMaxPacketSize0 = 64U,
		.idVendor = VENDOR_ID,
		.idProduct = PRODUCT_ID,
		.bcdDevice = 0x0101U,
		.iManufacturer = STRING_INDEX_MANUFACTURER,
		.iProduct = STRING_INDEX_PRODUCT,
		.iSerialNumber = STRING_INDEX_SERIAL,
		.bNumConfigurations = 1U,
	},
	.string_count = 6U,
	.string_zero_descriptor = &STRING_ZERO,
	.language_table = LANGUAGE_TABLE,
	.control_handler = &usb_control_handler,
	.configurations = {
		&ENABLED_CONFIGURATION,
	},
};

void vApplicationIdleHook(void) {
	asm volatile("wfi");
}

static void main_task(void *param) __attribute__((noreturn));

static void stm32_main(void) {
	// Initialize the basic chip hardware.
	init_chip(&INIT_SPECS);

	// Initialize the GPIO pins.
	gpio_init(PINS_INIT, sizeof(PINS_INIT) / sizeof(*PINS_INIT));

	// Get into FreeRTOS.
	BaseType_t ok = xTaskCreate(&main_task, "main", 512U, 0, 1U, 0);
	assert(ok == pdPASS);
	vTaskStartScheduler();
	__builtin_unreachable();
}

static void main_task(void *UNUSED(param)) {
	// Initialize subsystems.
	led_init();
	crc32_init();
	build_id_init();
	buzzer_init();
	estop_init(EXCEPTION_MKPRIO(5U, 0U));

	// Fill in the device serial number.
	{
		char temp[24U];
		formathex32(&temp[0U], U_ID.H);
		formathex32(&temp[8U], U_ID.M);
		formathex32(&temp[16U], U_ID.L);
		for (size_t i = 0U; i < 24U; ++i) {
			STRING_SERIAL.bString[i] = temp[i];
		}
	}

	// Initialize USB.
	udev_init(&USB_INFO);
	udev_attach();

	// Done setting up.
	vTaskDelete(0);
	__builtin_unreachable();
}

