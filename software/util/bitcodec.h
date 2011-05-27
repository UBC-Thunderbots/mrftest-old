/**
 * \file
 *
 * \brief Permits defining complicated data structure layouts to bit-level precision.
 *
 * To define a bitfielded structure, one does the following:
 * \li Write a \c .def file defining the layout of the structure (names, types, positions, and sizes of elements).
 * \li In your \c .h file, define the symbol \c BITCODEC_DEF_FILE as the path to the \c .def file.
 * \li In your \c .h file, define the symbol \c BITCODEC_STRUCT_NAME as the name of the struct you wish to define.
 * \li In your \c .h file, if you want the structure to be defined inside a namespace, define the symbol \c BITCODEC_NAMESPACE to be the name of the namespace.
 * \li In your \c .h file, if you want the structure to be available only within a single translation unit, define the symbol \c BITCODEC_ANON_NAMESPACE.
 * \li In your \c .h file, include this file (\c util/bitcodec.h).
 * \li Include the \c .h file anywhere the structure is needed.
 *
 * In the \c .def file, the following macros may be invoked:
 * <dl>
 * <dt>BITCODEC_DATA_U(type, name, offset, length, def)</dt>
 * <dd>Defines a field of type \p type, which must be an unsigned integral type, with name \p name.
 * When encoding or decoding, the field will be stored at offset \p offset, measured in bits, and will consume \p length bits.
 * When a new instance of the structure is created without decoding a buffer, the field will be given the default value \p def.</dd>
 *
 * <dt>BITCODEC_DATA_S(type, utype, name, offset, length, def)</dt>
 * <dd>Defines a field of type \p type, which must be a signed integral type, with name \p name.
 * When encoding or decoding, the field will be transcoded through type \p utype, which must be the unsigned equivalent of \p type.
 * The field will be stored at offset \p offset, measured in bits, and will consume \p length bits.
 * When a new instance of the structure is created without decoding a buffer, the field will be given the default value \p def.</dd>
 *
 * <dt>BITCODEC_DATA_BOOL(name, offset, def)</dt>
 * <dd>Defines a field of type \c bool with name \p name.
 * When encoding or decoding, the field will be stored at offset \p offset, measured in bits, and will consume 1 bit.
 * When a new instance of the structure is created without decoding a buffer, the field will be given the default value \p def.</dd>
 * </dl>
 *
 * You will end up with a structure of the requested name containing the following:
 * \li a member variable of the requested type and name for each declared field.
 * \li a nullary (default) constructor which initializes all member variables to the default values specified in their definitions.
 * \li a copy constructor and an assignment operator with the usual semantics.
 * \li equality and inequality (but not general relational) operators with elementwise semantics.
 * \li a constructor <code>BITCODEC_STRUCT_NAME(const void *buffer)</code> which initializes the member variables by decoding the packed bytes at \p buffer.
 * \li a member function <code>void BITCODEC_STRUCT_NAME::encode(void *buffer) const</code> which packs the member variables' values into \p buffer.
 * \li a member <code>static const std::size_t BUFFER_SIZE</code> which is a compile-time constant and indicates the proper length for the buffers passed to the constructor and \c encode function.
 *
 * It is legal for some bits to remain unused in the structure.
 * Unused bits are ignored by the constructor when decoding a packed structure.
 * Unused bits are zeroed by \c encode when encoding a packed structure.
 * These rules also apply to any bits appearing after the last field in the structure and before the next byte boundary.
 * Any bytes beyond that byte boundary are neither read nor written by either the constructor or \c encode.
 */

/** \cond */

#if !defined UTIL_BITCODEC_PRIMITIVES_H
#error Must include util/bitcodec_primitives.h before including bitcodec.h!
#endif
#if !defined BITCODEC_DEF_FILE
#error Must define BITCODEC_DEF_FILE before including bitcodec.h!
#endif
#if !defined BITCODEC_STRUCT_NAME
#error Must define BITCODEC_STRUCT_NAME before including bitcodec.h!
#endif

#if defined BITCODEC_NAMESPACE
namespace BITCODEC_NAMESPACE {
#elif defined BITCODEC_ANON_NAMESPACE
namespace {
#endif

struct BITCODEC_STRUCT_NAME {
	static const std::size_t BUFFER_SIZE = BitcodecPrimitives::LengthCalculator<0, 0
#define BITCODEC_DATA_U(type, name, offset, length, def) \
		, offset, length
#define BITCODEC_DATA_S(type, utype, name, offset, length, def) \
		, offset, length
#define BITCODEC_DATA_BOOL(name, offset, def) \
		, offset, 1
#include BITCODEC_DEF_FILE
#undef BITCODEC_DATA_U
#undef BITCODEC_DATA_S
#undef BITCODEC_DATA_BOOL
		>::BYTES;

	BITCODEC_STRUCT_NAME();
	BITCODEC_STRUCT_NAME(const void *buffer);
	void encode(void *buffer) const;

#define BITCODEC_DATA_U(type, name, offset, length, def) \
	static_assert(sizeof(type) >= (length + 7) / 8, "Element type must be large enough to contain bit count."); \
	type name;
#define BITCODEC_DATA_S(type, utype, name, offset, length, def) \
	static_assert(sizeof(type) >= (length + 7) / 8, "Element type must be large enough to contain bit count."); \
	static_assert(sizeof(type) == sizeof(utype), "Element target type and unsigned type must be equal sizes."); \
	type name;
#define BITCODEC_DATA_BOOL(name, offset, def) \
	bool name;
#include BITCODEC_DEF_FILE
#undef BITCODEC_DATA_U
#undef BITCODEC_DATA_S
#undef BITCODEC_DATA_BOOL
};

inline BITCODEC_STRUCT_NAME::BITCODEC_STRUCT_NAME() {
#define BITCODEC_DATA_U(type, name, offset, length, def) \
	this->name = def;
#define BITCODEC_DATA_S(type, utype, name, offset, length, def) \
	this->name = def;
#define BITCODEC_DATA_BOOL(name, offset, def) \
	this->name = def;
#include BITCODEC_DEF_FILE
#undef BITCODEC_DATA_U
#undef BITCODEC_DATA_S
#undef BITCODEC_DATA_BOOL
}

inline BITCODEC_STRUCT_NAME::BITCODEC_STRUCT_NAME(const void *buffer) {
#define BITCODEC_DATA_U(type, name, offset, length, def) \
	this->name = BitcodecPrimitives::Decoder<type, offset, length>()(buffer);
#define BITCODEC_DATA_S(type, utype, name, offset, length, def) \
	this->name = BitcodecPrimitives::SignExtender<type, utype, length>()(BitcodecPrimitives::Decoder<utype, offset, length>()(buffer));
#define BITCODEC_DATA_BOOL(name, offset, def) \
	this->name = !!BitcodecPrimitives::Decoder<uint8_t, offset, 1>()(buffer);
#include BITCODEC_DEF_FILE
#undef BITCODEC_DATA_U
#undef BITCODEC_DATA_S
#undef BITCODEC_DATA_BOOL
}

inline void BITCODEC_STRUCT_NAME::encode(void *buffer) const {
	std::fill(static_cast<uint8_t *>(buffer), static_cast<uint8_t *>(buffer) + BUFFER_SIZE, 0);
#define BITCODEC_DATA_U(type, name, offset, length, def) \
	BitcodecPrimitives::Encoder<type, offset, length>()(buffer, this->name);
#define BITCODEC_DATA_S(type, utype, name, offset, length, def) \
	BitcodecPrimitives::Encoder<utype, offset, length>()(buffer, static_cast<utype>(this->name));
#define BITCODEC_DATA_BOOL(name, offset, def) \
	BitcodecPrimitives::Encoder<uint8_t, offset, 1>()(buffer, this->name ? 1 : 0);
#include BITCODEC_DEF_FILE
#undef BITCODEC_DATA_U
#undef BITCODEC_DATA_S
#undef BITCODEC_DATA_BOOL
}

inline bool operator==(const BITCODEC_STRUCT_NAME &x, const BITCODEC_STRUCT_NAME &y) {
	return true
#define BITCODEC_DATA_U(type, name, offset, length, def) && x.name == y.name
#define BITCODEC_DATA_S(type, utype, name, offset, length, def) && x.name == y.name
#define BITCODEC_DATA_BOOL(name, offset, def) && x.name == y.name
#include BITCODEC_DEF_FILE
#undef BITCODEC_DATA_U
#undef BITCODEC_DATA_S
#undef BITCODEC_DATA_BOOL
		;
}

inline bool operator!=(const BITCODEC_STRUCT_NAME &x, const BITCODEC_STRUCT_NAME &y) {
	return !(x == y);
}

#if defined BITCODEC_NAMESPACE
}
#elif defined BITCODEC_ANON_NAMESPACE
}
#endif

namespace {
	static_assert(BitcodecPrimitives::OverlapChecker<0, 0
#define BITCODEC_DATA_U(type, name, offset, length, def) \
	, offset, length
#define BITCODEC_DATA_S(type, utype, name, offset, length, def) \
	, offset, length
#define BITCODEC_DATA_BOOL(name, offset, def) \
	, offset, 1
#include BITCODEC_DEF_FILE
#undef BITCODEC_DATA_U
#undef BITCODEC_DATA_S
#undef BITCODEC_DATA_BOOL
		>::OK, "Packet fields overlap!");
}

/** \endcond */

