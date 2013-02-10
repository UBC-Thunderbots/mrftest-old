#include "string.h"
#include "usb_ep0_sources.h"

static size_t memory_source_generate(void *opaque, void *buffer, size_t length) {
	usb_ep0_memory_source_t *source = opaque;
	if (length > source->len) {
		length = source->len;
	}
	memcpy(buffer, source->ptr, length);
	source->ptr += length;
	source->len -= length;
	return length;
}

usb_ep0_source_t *usb_ep0_memory_source_init(usb_ep0_memory_source_t *source, const void *data, size_t length) {
	source->ptr = data;
	source->len = length;
	source->src.opaque = source;
	source->src.generate = &memory_source_generate;
	return &source->src;
}



#define UTF8_IS_CONTINUATION(x) (UINT8_C(0x80) <= (x) && (x) <= UINT8_C(0xBF))
static uint32_t decode_utf8(const unsigned char **psrc) {
	const unsigned char *src = *psrc;
	uint32_t code_point;

	if (src[0] <= UINT8_C(0x7F)) {
		// Leading bytes in this range signify 1-byte encodings of code points from U+000000 to U+00007F.
		code_point = *src++;
	} else if (src[0] <= UINT8_C(0xC1)) {
		// Leading bytes in this range are illegal; bytes from 0x80 to 0xBF encode continuation bytes and 0xC0 and 0xC1 are overlong encodings of what should be 1-byte-encoded code points.
		// Just give up.
		return UINT32_C(0xFFFFFFFF);
	} else if (src[0] <= UINT8_C(0xDF)) {
		// Leading bytes in this range signify 2-byte encodings of code points which must lie from U+000080 to U+0007FF.
		if (UTF8_IS_CONTINUATION(src[1])) {
			code_point = *src++;
			code_point = (code_point << 6) | ((*src++) & UINT8_C(0x3F));
		} else {
			return UINT32_C(0xFFFFFFFF);
		}
		if (!(0x80 <= code_point && code_point <= 0x7FF)) {
			return UINT32_C(0xFFFFFFFF);
		}
	} else if (src[0] <= UINT8_C(0xEF)) {
		// Leading bytes in this range signify 3-byte encodings of code points which must lie from U+0x000800 to U+00FFFF.
		if (UTF8_IS_CONTINUATION(src[1]) && UTF8_IS_CONTINUATION(src[2])) {
			code_point = *src++;
			code_point = (code_point << 6) | ((*src++) & UINT8_C(0x3F));
			code_point = (code_point << 6) | ((*src++) & UINT8_C(0x3F));
		} else {
			return UINT32_C(0xFFFFFFFF);
		}
		if (!(0x800 <= code_point && code_point <= 0xFFFF)) {
			return UINT32_C(0xFFFFFFFF);
		}
	} else if (src[0] <= UINT8_C(0xF4)) {
		// Leading bytes in this range signify 4-byte encodings of code points which must lie from U+0x010000 to U+10FFFF.
		if (UTF8_IS_CONTINUATION(src[1]) && UTF8_IS_CONTINUATION(src[2]) && UTF8_IS_CONTINUATION(src[3])) {
			code_point = *src++;
			code_point = (code_point << 6) | ((*src++) & UINT8_C(0x3F));
			code_point = (code_point << 6) | ((*src++) & UINT8_C(0x3F));
			code_point = (code_point << 6) | ((*src++) & UINT8_C(0x3F));
		} else {
			return UINT32_C(0xFFFFFFFF);
		}
		if (!(0x10000 <= code_point && code_point <= 0x10FFFF)) {
			return UINT32_C(0xFFFFFFFF);
		}
	} else {
		// Leading bytes from 0xF5 to 0xFF signify nominal encodings of code points which are illegal due to lying outside the range U+000000 to U+10FFFF.
		return UINT32_C(0xFFFFFFFF);
	}

	// Check that the decoded code unit doesn’t lie in the UTF-16 surrogate range (these numbers may never appear as code points, only UTF-16 code units).
	if (0xD800 <= code_point && code_point <= 0xDFFF) {
		return UINT32_C(0xFFFFFFFF);
	}

	// Code point is OK.
	*psrc = src;
	return code_point;
}
#undef UTF8_IS_CONTINUATION

#define FLAG_HEADER_PENDING 0x01
static size_t string_descriptor_source_generate(void *opaque, void *buffer, size_t length) {
	// Set up.
	usb_ep0_string_descriptor_source_t *source = opaque;
	uint16_t *pdest = buffer;
	size_t generated = 0;

	// Generate as much as we can.
	while (generated < length) {
		if (source->flags & FLAG_HEADER_PENDING) {
			// We need to send the header before anything else.
			*pdest++ = source->descriptor_length | (USB_DTYPE_STRING << 8);
			generated += 2;
			source->flags &= ~FLAG_HEADER_PENDING;
		} else if (source->pending_trail_surrogate) {
			// A trail surrogate was generated last time around and not yet delivered.
			// Deliver it now.
			*pdest++ = source->pending_trail_surrogate;
			generated += 2;
			source->pending_trail_surrogate = 0;
		} else {
			// Try to decode a Unicode code point from one or more UTF-8 code units.
			uint32_t code_point = decode_utf8(&source->ptr);

			// If this marks the end of the string or an error, stop now.
			if (!code_point || code_point == UINT32_C(0xFFFFFFFF)) {
				return generated;
			}

			// Encode the code point into either one UTF-16 code unit or a pair of surrogate UTF-16 code units.
			if (code_point <= 0xFFFF) {
				*pdest++ = (uint16_t) code_point;
				generated += 2;
			} else {
				code_point -= 0x10000;
				*pdest++ = (uint16_t) ((code_point >> 10) + 0xD800);
				generated += 2;
				source->pending_trail_surrogate = (uint16_t) ((code_point & 0x3FF) + 0xDC00);
			}
		}
	}

	return generated;
}

usb_ep0_source_t *usb_ep0_string_descriptor_source_init(usb_ep0_string_descriptor_source_t *source, const char *string) {
	// Compute the length of the descriptor by figuring out how long the transcoded string will be.
	// Add 2 bytes for the header (bLength plus bDescriptorType).
	source->ptr = (const unsigned char *) string;
	source->descriptor_length = 2;
	{
		for (;;) {
			uint32_t code_point = decode_utf8(&source->ptr);
			if (!code_point || code_point == UINT32_C(0xFFFFFFFF)) {
				break;
			}
			source->descriptor_length += 2;
			if (code_point > 0xFFFF) {
				source->descriptor_length += 2;
			}
		}
	}

	// Fill in the rest of the structure.
	source->flags = FLAG_HEADER_PENDING;
	source->ptr = (const unsigned char *) string;
	source->pending_trail_surrogate = 0;
	source->src.opaque = source;
	source->src.generate = &string_descriptor_source_generate;
	return &source->src;
}
#undef FLAG_HEADER_PENDING

