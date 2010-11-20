#include "log/analyzer.h"
#include "ai/flags.h"
#include "ai/common/playtype.h"
#include "log/shared/tags.h"
#include "proto/messages_robocup_ssl_wrapper.pb.h"
#include "uicomponents/abstract_list_model.h"
#include "util/algorithm.h"
#include "util/codec.h"
#include "util/crc16.h"
#include "util/dprint.h"
#include "util/exception.h"
#include "util/time.h"
#include <cstring>
#include <cwchar>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>

namespace {
	/**
	 * A column record containing two columns, a key and a value, suitable for displaying decoded packet structures.
	 */
	class PacketDecodedTreeColumns : public Gtk::TreeModelColumnRecord {
		public:
			/**
			 * The key column.
			 */
			Gtk::TreeModelColumn<Glib::ustring> key;

			/**
			 * The value column.
			 */
			Gtk::TreeModelColumn<Glib::ustring> value;

			/**
			 * Constructs a new PacketDecodedTreeColumns.
			 */
			PacketDecodedTreeColumns();

			/**
			 * Destroys a PacketTreeDecodedColumns.
			 */
			~PacketDecodedTreeColumns();

			/**
			 * Adds a key-value pair to a tree.
			 *
			 * \param[in] store the tree to add the row to.
			 *
			 * \param[in] k the text of the key to add.
			 *
			 * \param[in] v the text of the value to add.
			 *
			 * \return the new row.
			 */
			Gtk::TreeRow append_kv(Glib::RefPtr<Gtk::TreeStore> store, const Glib::ustring &k, const Glib::ustring &v = "") const;

			/**
			 * Adds a key-value pair to a tree.
			 *
			 * \param[in] store the tree to add the row to.
			 *
			 * \param[in] parent the row underneath which to add the new row.
			 *
			 * \param[in] k the text of the key to add.
			 *
			 * \param[in] v the text of the value to add.
			 *
			 * \return the new row.
			 */
			Gtk::TreeRow append_kv(Glib::RefPtr<Gtk::TreeStore> store, const Gtk::TreeRow &parent, const Glib::ustring &k, const Glib::ustring &v = "") const;
	};

	PacketDecodedTreeColumns::PacketDecodedTreeColumns() {
		add(key);
		add(value);
	}

	PacketDecodedTreeColumns::~PacketDecodedTreeColumns() {
	}

	Gtk::TreeRow PacketDecodedTreeColumns::append_kv(Glib::RefPtr<Gtk::TreeStore> store, const Glib::ustring &k, const Glib::ustring &v) const {
		Gtk::TreeRow new_row = *store->append();
		new_row[key] = k;
		new_row[value] = v;
		return new_row;
	}

	Gtk::TreeRow PacketDecodedTreeColumns::append_kv(Glib::RefPtr<Gtk::TreeStore> store, const Gtk::TreeRow &parent, const Glib::ustring &k, const Glib::ustring &v) const {
		Gtk::TreeRow new_row = *store->append(parent.children());
		new_row[key] = k;
		new_row[value] = v;
		return new_row;
	}

	std::string make_filename(const std::string &filename) {
		const std::string &parent_dir = Glib::get_user_data_dir();
		const std::string &tbots_dir = Glib::build_filename(parent_dir, "thunderbots");
		const std::string &logs_dir = Glib::build_filename(tbots_dir, "logs");
		return Glib::build_filename(logs_dir, filename);
	}

	enum {
		/**
		 * The packet has an unknown tag (it may have been written by a newer version).
		 */
		PF_UNKNOWN_TAG = 0x1,

		/**
		 * The packet is shorter than expected (it may have been written by an older version).
		 */
		PF_SHORT = 0x2,

		/**
		 * The packet is longer than expected (it may have been written by a newer version).
		 */
		PF_LONG = 0x4,

		/**
		 * The packet extends beyond the end of the file.
		 */
		PF_TRUNCATED = 0x8,

		/**
		 * The packet has a bad CRC16.
		 */
		PF_BAD_CRC = 0x10,

		/**
		 * The packet contains a UTF-8 string which is encoded illegally.
		 */
		PF_BAD_UTF8 = 0x20,

		/**
		 * The packet contains a log message but the log level is unknown (it may have been written by a newer version).
		 */
		PF_UNKNOWN_LOG_LEVEL = 0x40,

		/**
		 * The packet contains a double but the value is not a legal encoding of a floating-point number.
		 */
		PF_BAD_DOUBLE = 0x80,

		/**
		 * The packet contains a raw network packet but the packet data does not comply with the expected protocol.
		 */
		PF_BAD_NETWORK_PACKET = 0x100,

		/**
		 * The packet contains a play type but the play type is illegal.
		 */
		PF_BAD_PLAYTYPE = 0x200,

		/**
		 * The packet contains a robot movement flag set but one of the flags is unknown (it may have been written by a newer version).
		 */
		PF_UNKNOWN_MOVE_FLAG = 0x400,

		/**
		 * The packet contains a robot movement type but the type is unknown (it may have been written by a newer version).
		 */
		PF_UNKNOWN_MOVE_TYPE = 0x800,

		/**
		 * The packet contains a robot movement priority but the priority is unknown (it may have been written by a newer version).
		 */
		PF_UNKNOWN_MOVE_PRIO = 0x1000,

		/**
		 * The packet marks the shutdown of the application but the reason for termination is unknown (it may have been written by a newer version).
		 */
		PF_UNKNOWN_END_REASON = 0x2000,

		/**
		 * The packet contains a robot pattern index but the pattern index is invalid.
		 */
		PF_BAD_PATTERN = 0x4000,

		/**
		 * The packet contains a timespec whose nanoseconds field is greater than or equal to one second.
		 */
		PF_BAD_TIMESPEC = 0x8000,

		/**
		 * The packet uses the old ASCII encoding of floating-point values.
		 */
		PF_OLD_DOUBLE = 0x10000,
	};

	/**
	 * The set of packet flags that constitute errors.
	 */
	const unsigned int ERROR_FLAGS = PF_TRUNCATED | PF_BAD_CRC | PF_BAD_UTF8 | PF_BAD_DOUBLE | PF_BAD_NETWORK_PACKET | PF_BAD_PLAYTYPE | PF_BAD_PATTERN | PF_BAD_TIMESPEC;

	/**
	 * Converts a timespec to its string representation in either the local time zone or UTC.
	 *
	 * \param[in] ts the time to convert.
	 *
	 * \param[in] local \c true to convert to local time, or \c false to convert to UTC.
	 *
	 * \return the string representation of the time.
	 */
	Glib::ustring timespec_to_time_string_part(const timespec &ts, bool local) {
		static const wchar_t TIME_PATTERN[] = L"%Y-%m-%d %H:%M:%S";
		static const wchar_t TZ_PATTERN[] = L"%Z";
		std::wostringstream timebuf, tzbuf;
		struct tm tm;

		if (!(local ? localtime_r : gmtime_r)(&ts.tv_sec, &tm)) {
			throw SystemError(local ? "localtime_r" : "gmtime_r", errno);
		}
		
		std::use_facet<std::time_put<wchar_t> >(std::locale()).put(timebuf, timebuf, L' ', &tm, TIME_PATTERN, TIME_PATTERN + std::wcslen(TIME_PATTERN));
		std::use_facet<std::time_put<wchar_t> >(std::locale()).put(tzbuf, tzbuf, L' ', &tm, TZ_PATTERN, TZ_PATTERN + std::wcslen(TZ_PATTERN));
		return Glib::ustring::compose("%1.%2 %3", timebuf.str(), todec(ts.tv_nsec, 9), tzbuf.str());
	}

	/**
	 * Converts a timespec into a string representation showing both the local time zone and UTC.
	 *
	 * \param[in] ts the time.
	 *
	 * \return the string representation.
	 */
	Glib::ustring timespec_to_time_string(const timespec &ts) {
		return Glib::ustring::compose("%1.%2 (%3 / %4)", ts.tv_sec, todec(ts.tv_nsec, 9), timespec_to_time_string_part(ts, true), timespec_to_time_string_part(ts, false));
	}

	/**
	 * Converts a double count of seconds since the epoch into a string representation showing both the local time zone and UTC.
	 *
	 * \param[in] seconds the time.
	 *
	 * \return the string representation.
	 */
	Glib::ustring double_to_time_string(double seconds) {
		return timespec_to_time_string(double_to_timespec(seconds));
	}

	/**
	 * Validates a UTF-8 string.
	 *
	 * \param[in] data the string to validate.
	 *
	 * \param[in] length the number of bytes in the string.
	 *
	 * \return \c true if the string is valid, or \c false if not.
	 */
	bool check_utf8(const void *data, std::size_t length) {
		std::string utf8(static_cast<const char *>(data), length);
		Glib::ustring ustr(utf8);
		return ustr.validate();
	}

	/**
	 * Validates an ASCII-encoded floating point number.
	 *
	 * \param[in] data the encoded string.
	 *
	 * \param[in] length the number of bytes in the string.
	 *
	 * \return \c true if the string matches all the rules for an ASCII-encoded floating point number, or \c false if not.
	 */
	bool check_old_double(const void *data, std::size_t length) {
		const char *p = static_cast<const char *>(data);
		bool seen_sign = false;
		bool seen_ones = false;
		bool seen_dot = false;
		bool seen_e = false;
		bool seen_esign = false;
		bool seen_edigit = false;

		while (length) {
			char ch = *p++;
			--length;
			if (!seen_sign) {
				switch (ch) {
					case ' ':
						break;

					case '+':
					case '-':
						seen_sign = true;
						break;

					default:
						return false;
				}
			} else if (!seen_ones) {
				switch (ch) {
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						seen_ones = true;
						break;

					default:
						return false;
				}
			} else if (!seen_dot) {
				switch (ch) {
					case '.':
						seen_dot = true;
						break;

					default:
						return false;
				}
			} else if (!seen_e) {
				switch (ch) {
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						break;

					case 'e':
					case 'E':
						seen_e = true;
						break;

					default:
						return false;
				}
			} else if (!seen_esign) {
				switch (ch) {
					case '+':
					case '-':
						seen_esign = true;
						break;

					default:
						return false;
				}
			} else {
				switch (ch) {
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						seen_edigit = true;
						break;

					default:
						return false;
				}
			}
		}

		return seen_edigit;
	}

	/**
	 * Computes flags for a trailing UTF-8 string.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \return the packet flags.
	 */
	unsigned int packet_generic_utf8_compute_flags(const uint8_t *data, std::size_t length, std::size_t) {
		return check_utf8(data, length) ? 0 : PF_BAD_UTF8;
	}

	/**
	 * Parses a trailing UTF-8 string and produces a human-readable tree of its contents.
	 *
	 * \param[in] columns the column record containing the columns of the store.
	 *
	 * \param[in] root the tree row under which to build the element's row.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 */
	void packet_generic_utf8_build_tree(Glib::RefPtr<Gtk::TreeStore>, const PacketDecodedTreeColumns &columns, const Gtk::TreeRow &root, const uint8_t *data, std::size_t length, std::size_t declared_length) {
		std::string utf8(reinterpret_cast<const char *>(data), length);
		Glib::ustring ustr(utf8);
		if (ustr.validate()) {
			if (length < declared_length) {
				ustr += " <PACKET TRUNCATED>";
			}
			root[columns.value] = ustr;
		} else {
			root[columns.value] = "<ILLEGAL UTF-8>";
		}
	}

	/**
	 * Computes flags for a one-byte boolean.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 *
	 * \return the packet flags.
	 */
	unsigned int packet_generic_boolean_compute_flags(const uint8_t *, std::size_t, std::size_t declared_length) {
		unsigned int flags = 0;

		if (declared_length < 1) {
			flags |= PF_SHORT;
		} else if (declared_length > 1) {
			flags |= PF_LONG;
		}

		return flags;
	}

	/**
	 * Parses a one-byte boolean and produces a human-readable tree of its contents.
	 *
	 * \param[in] columns the column record containing the columns of the store.
	 *
	 * \param[in] root the tree row under which to build the element's row.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 */
	void packet_generic_boolean_build_tree(Glib::RefPtr<Gtk::TreeStore>, const PacketDecodedTreeColumns &columns, const Gtk::TreeRow &root, const uint8_t *data, std::size_t length, std::size_t declared_length) {
		if (declared_length < 1) {
			root[columns.value] = "<OMITTED>";
		} else {
			if (length >= 1) {
				uint8_t value = decode_u8(&data[0]);
				root[columns.value] = value ? "True" : "False";
			} else {
				root[columns.value] = "<PACKET TRUNCATED>";
			}
		}
	}

	/**
	 * Computes flags for an unsigned 8-bit integer.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 *
	 * \return the packet flags.
	 */
	unsigned int packet_generic_uint8_compute_flags(const uint8_t *, std::size_t, std::size_t declared_length) {
		unsigned int flags = 0;

		if (declared_length < 1) {
			flags |= PF_SHORT;
		} else if (declared_length > 1) {
			flags |= PF_LONG;
		}

		return flags;
	}

	/**
	 * Parses an unsigned 8-bit integer and produces a human-readable tree of its contents.
	 *
	 * \param[in] columns the column record containing the columns of the store.
	 *
	 * \param[in] root the tree row under which to build the element's row.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 */
	void packet_generic_uint8_build_tree(Glib::RefPtr<Gtk::TreeStore>, const PacketDecodedTreeColumns &columns, const Gtk::TreeRow &root, const uint8_t *data, std::size_t length, std::size_t declared_length) {
		if (declared_length < 1) {
			root[columns.value] = "<OMITTED>";
		} else {
			if (length >= 1) {
				uint8_t value = decode_u8(data);
				root[columns.value] = Glib::ustring::format(static_cast<unsigned int>(value));
			} else {
				root[columns.value] = "<PACKET TRUNCATED>";
			}
		}
	}

	/**
	 * Computes flags for an unsigned 16-bit integer.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 *
	 * \return the packet flags.
	 */
	unsigned int packet_generic_uint16_compute_flags(const uint8_t *, std::size_t, std::size_t declared_length) {
		unsigned int flags = 0;

		if (declared_length < 2) {
			flags |= PF_SHORT;
		} else if (declared_length > 2) {
			flags |= PF_LONG;
		}

		return flags;
	}

	/**
	 * Parses an unsigned 16-bit integer and produces a human-readable tree of its contents.
	 *
	 * \param[in] columns the column record containing the columns of the store.
	 *
	 * \param[in] root the tree row under which to build the element's row.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 */
	void packet_generic_uint16_build_tree(Glib::RefPtr<Gtk::TreeStore>, const PacketDecodedTreeColumns &columns, const Gtk::TreeRow &root, const uint8_t *data, std::size_t length, std::size_t declared_length) {
		if (declared_length < 2) {
			root[columns.value] = "<OMITTED>";
		} else {
			if (length >= 2) {
				uint16_t value = decode_u16(data);
				root[columns.value] = Glib::ustring::format(value);
			} else {
				root[columns.value] = "<PACKET TRUNCATED>";
			}
		}
	}

	/**
	 * Computes flags for a signed 16-bit integer.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 *
	 * \return the packet flags.
	 */
	unsigned int packet_generic_int16_compute_flags(const uint8_t *, std::size_t, std::size_t declared_length) {
		unsigned int flags = 0;

		if (declared_length < 2) {
			flags |= PF_SHORT;
		} else if (declared_length > 2) {
			flags |= PF_LONG;
		}

		return flags;
	}

	/**
	 * Parses a signed 16-bit integer and produces a human-readable tree of its contents.
	 *
	 * \param[in] columns the column record containing the columns of the store.
	 *
	 * \param[in] root the tree row under which to build the element's row.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 */
	void packet_generic_int16_build_tree(Glib::RefPtr<Gtk::TreeStore>, const PacketDecodedTreeColumns &columns, const Gtk::TreeRow &root, const uint8_t *data, std::size_t length, std::size_t declared_length) {
		if (declared_length < 2) {
			root[columns.value] = "<OMITTED>";
		} else {
			if (length >= 2) {
				int16_t value = decode_u16(data);
				root[columns.value] = Glib::ustring::format(value);
			} else {
				root[columns.value] = "<PACKET TRUNCATED>";
			}
		}
	}

	/**
	 * Computes flags for a signed 64-bit integer.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 *
	 * \return the packet flags.
	 */
	unsigned int packet_generic_int64_compute_flags(const uint8_t *, std::size_t, std::size_t declared_length) {
		unsigned int flags = 0;

		if (declared_length < 8) {
			flags |= PF_SHORT;
		} else if (declared_length > 8) {
			flags |= PF_LONG;
		}

		return flags;
	}

	/**
	 * Parses a signed 64-bit integer and produces a human-readable tree of its contents.
	 *
	 * \param[in] columns the column record containing the columns of the store.
	 *
	 * \param[in] root the tree row under which to build the element's row.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 */
	void packet_generic_int64_build_tree(Glib::RefPtr<Gtk::TreeStore>, const PacketDecodedTreeColumns &columns, const Gtk::TreeRow &root, const uint8_t *data, std::size_t length, std::size_t declared_length) {
		if (declared_length < 8) {
			root[columns.value] = "<OMITTED>";
		} else {
			if (length >= 8) {
				int64_t value = decode_u64(data);
				root[columns.value] = Glib::ustring::format(value);
			} else {
				root[columns.value] = "<PACKET TRUNCATED>";
			}
		}
	}

	/**
	 * Computes flags for an unsigned 8-bit integer representing a robot pattern.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 *
	 * \return the packet flags.
	 */
	unsigned int packet_generic_pattern_compute_flags(const uint8_t *data, std::size_t length, std::size_t declared_length) {
		unsigned int flags = 0;

		if (declared_length < 1) {
			flags |= PF_SHORT;
		} else if (declared_length > 1) {
			flags |= PF_LONG;
		}

		if (length >= 1) {
			uint8_t pattern = decode_u8(data);
			if (pattern >= 16) {
				flags |= PF_BAD_PATTERN;
			}
		}

		return flags;
	}

	/**
	 * Parses an unsigned 8-bit integer representing a robot pattern and produces a human-readable tree of its contents.
	 *
	 * \param[in] columns the column record containing the columns of the store.
	 *
	 * \param[in] root the tree row under which to build the element's row.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 */
	void packet_generic_pattern_build_tree(Glib::RefPtr<Gtk::TreeStore>, const PacketDecodedTreeColumns &columns, const Gtk::TreeRow &root, const uint8_t *data, std::size_t length, std::size_t declared_length) {
		if (declared_length < 1) {
			root[columns.value] = "<OMITTED>";
		} else {
			if (length >= 1) {
				uint8_t pattern = decode_u8(data);
				if (pattern < 16) {
					root[columns.value] = Glib::ustring::format(static_cast<unsigned int>(pattern));
				} else {
					root[columns.value] = Glib::ustring::format("<INVALID PATTERN NUMBER %1>", static_cast<unsigned int>(pattern));
				}
			} else {
				root[columns.value] = "<PACKET TRUNCATED>";
			}
		}
	}

	/**
	 * Computes flags for a signed 32-bit integer representing a fixed-point number in millionths.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 *
	 * \return the packet flags.
	 */
	unsigned int packet_generic_int32u_compute_flags(const uint8_t *, std::size_t, std::size_t declared_length) {
		unsigned int flags = 0;

		if (declared_length < 4) {
			flags |= PF_SHORT;
		} else if (declared_length > 4) {
			flags |= PF_LONG;
		}

		return flags;
	}

	/**
	 * Parses a signed 32-bit integer representing a fixed-point number in millionths and produces a human-readable tree of its contents.
	 *
	 * \param[in] columns the column record containing the columns of the store.
	 *
	 * \param[in] root the tree row under which to build the element's row.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 */
	void packet_generic_int32u_build_tree(Glib::RefPtr<Gtk::TreeStore>, const PacketDecodedTreeColumns &columns, const Gtk::TreeRow &root, const uint8_t *data, std::size_t length, std::size_t declared_length) {
		if (declared_length < 4) {
			root[columns.value] = "<OMITTED>";
		} else {
			if (length >= 4) {
				int32_t raw_value = decode_u32(data);
				double value = raw_value / 1000000.0;
				root[columns.value] = Glib::ustring::format(value);
			} else {
				root[columns.value] = "<PACKET TRUNCATED>";
			}
		}
	}

	/**
	 * Computes flags for an ASCII-encoded floating point number.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 *
	 * \return the packet flags.
	 */
	unsigned int packet_generic_old_double_compute_flags(const uint8_t *data, std::size_t length, std::size_t declared_length) {
		unsigned int flags = PF_OLD_DOUBLE;

		if (declared_length < 20) {
			flags |= PF_SHORT;
		} else if (declared_length > 20) {
			flags |= PF_LONG;
		}

		if (length >= 20) {
			if (!check_old_double(data, length)) {
				flags |= PF_BAD_DOUBLE;
			}
		}

		return flags;
	}

	/**
	 * Parses an ASCII-encoded floating point number and produces a human-readable tree of its contents.
	 *
	 * \param[in] columns the column record containing the columns of the store.
	 *
	 * \param[in] root the tree row under which to build the element's row.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 */
	void packet_generic_old_double_build_tree(Glib::RefPtr<Gtk::TreeStore>, const PacketDecodedTreeColumns &columns, const Gtk::TreeRow &root, const uint8_t *data, std::size_t length, std::size_t declared_length) {
		if (declared_length < 20) {
			root[columns.value] = "<OMITTED>";
		} else {
			if (length >= 20) {
				if (check_old_double(data, 20)) {
					std::istringstream iss(std::string(reinterpret_cast<const char *>(data), 20));
					iss.imbue(std::locale("C"));
					double value;
					iss >> value;
					root[columns.value] = Glib::ustring::format(value);
				} else {
					root[columns.value] = "<INVALID FLOATING POINT ENCODING>";
				}
			} else {
				root[columns.value] = "<PACKET TRUNCATED>";
			}
		}
	}

	/**
	 * Computes flags for an IEEE754 double-precision-encoded floating point number.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 *
	 * \return the packet flags.
	 */
	unsigned int packet_generic_double_compute_flags(const uint8_t *, std::size_t, std::size_t declared_length) {
		unsigned int flags = 0;

		if (declared_length < 8) {
			flags |= PF_SHORT;
		} else if (declared_length > 8) {
			flags |= PF_LONG;
		}

		return flags;
	}

	/**
	 * Parses an IEEE754 double-precision-encoded floating point number and produces a human-readable tree of its contents.
	 *
	 * \param[in] columns the column record containing the columns of the store.
	 *
	 * \param[in] root the tree row under which to build the element's row.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 */
	void packet_generic_double_build_tree(Glib::RefPtr<Gtk::TreeStore>, const PacketDecodedTreeColumns &columns, const Gtk::TreeRow &root, const uint8_t *data, std::size_t length, std::size_t declared_length) {
		if (declared_length < 8) {
			root[columns.value] = "<OMITTED>";
		} else {
			if (length >= 8) {
				root[columns.value] = Glib::ustring::format(decode_double(data));
			} else {
				root[columns.value] = "<PACKET TRUNCATED>";
			}
		}
	}

	/**
	 * Computes flags for a realtime timespec.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 *
	 * \return the packet flags.
	 */
	unsigned int packet_generic_timespec_rt_compute_flags(const uint8_t *data, std::size_t length, std::size_t declared_length) {
		unsigned int flags = 0;

		if (declared_length < 12) {
			flags |= PF_SHORT;
		} else if (declared_length > 12) {
			flags |= PF_LONG;
		}

		if (length >= 12) {
			uint32_t nanos = decode_u32(&data[8]);
			if (nanos > UINT32_C(999999999)) {
				flags |= PF_BAD_TIMESPEC;
			}
		}

		return flags;
	}

	/**
	 * Parses a realtime timespec and produces a human-readable tree of its contents.
	 *
	 * \param[in] columns the column record containing the columns of the store.
	 *
	 * \param[in] root the tree row under which to build the element's row.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 */
	void packet_generic_timespec_rt_build_tree(Glib::RefPtr<Gtk::TreeStore>, const PacketDecodedTreeColumns &columns, const Gtk::TreeRow &root, const uint8_t *data, std::size_t length, std::size_t declared_length) {
		if (declared_length < 12) {
			root[columns.value] = "<OMITTED>";
		} else {
			if (length >= 12) {
				uint64_t seconds = decode_u64(&data[0]);
				uint32_t nanos = decode_u32(&data[8]);
				timespec ts;
				ts.tv_sec = static_cast<std::time_t>(seconds);
				ts.tv_nsec = static_cast<long>(nanos);
				root[columns.value] = timespec_to_time_string(ts);
			} else {
				root[columns.value] = "<PACKET TRUNCATED>";
			}
		}
	}

	/**
	 * Computes flags for a monotonic timespec.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 *
	 * \return the packet flags.
	 */
	unsigned int packet_generic_timespec_monotonic_compute_flags(const uint8_t *data, std::size_t length, std::size_t declared_length) {
		unsigned int flags = 0;

		if (declared_length < 12) {
			flags |= PF_SHORT;
		} else if (declared_length > 12) {
			flags |= PF_LONG;
		}

		if (length >= 12) {
			uint32_t nanos = decode_u32(&data[8]);
			if (nanos > UINT32_C(999999999)) {
				flags |= PF_BAD_TIMESPEC;
			}
		}

		return flags;
	}

	/**
	 * Parses a monotonic timespec and produces a human-readable tree of its contents.
	 *
	 * \param[in] columns the column record containing the columns of the store.
	 *
	 * \param[in] root the tree row under which to build the element's row.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 */
	void packet_generic_timespec_monotonic_build_tree(Glib::RefPtr<Gtk::TreeStore>, const PacketDecodedTreeColumns &columns, const Gtk::TreeRow &root, const uint8_t *data, std::size_t length, std::size_t declared_length) {
		if (declared_length < 12) {
			root[columns.value] = "<OMITTED>";
		} else {
			if (length >= 12) {
				uint64_t seconds = decode_u64(&data[0]);
				uint32_t nanos = decode_u32(&data[8]);
				root[columns.value] = Glib::ustring::compose("%1.%2 (monotonic)", seconds, todec(nanos, 9));
			} else {
				root[columns.value] = "<PACKET TRUNCATED>";
			}
		}
	}

	/**
	 * Computes flags for a debug message log level.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 *
	 * \return the packet flags.
	 */
	unsigned int packet_debug_log_level_compute_flags(const uint8_t *data, std::size_t length, std::size_t declared_length) {
		unsigned int flags = 0;

		if (declared_length < 1) {
			flags |= PF_SHORT | PF_UNKNOWN_LOG_LEVEL;
		} else if (declared_length > 1) {
			flags |= PF_LONG;
		}

		if (length >= 1) {
			uint8_t level = decode_u8(&data[0]);
			switch (level) {
				case LOG_LEVEL_DEBUG:
				case LOG_LEVEL_INFO:
				case LOG_LEVEL_WARN:
				case LOG_LEVEL_ERROR:
					break;

				default:
					flags |= PF_UNKNOWN_LOG_LEVEL;
					break;
			}
		}

		return flags;
	}

	/**
	 * Parses a debug log message log level and produces a human-readable tree of its contents.
	 *
	 * \param[in] columns the column record containing the columns of the store.
	 *
	 * \param[in] root the tree row under which to build the element's row.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 */
	void packet_debug_log_level_build_tree(Glib::RefPtr<Gtk::TreeStore>, const PacketDecodedTreeColumns &columns, const Gtk::TreeRow &root, const uint8_t *data, std::size_t length, std::size_t declared_length) {
		if (declared_length < 1) {
			root[columns.value] = "<OMITTED>";
		} else {
			if (length >= 1) {
				uint8_t level = decode_u8(data);
				switch (level) {
					case LOG_LEVEL_DEBUG:
						root[columns.value] = "Debug";
						break;

					case LOG_LEVEL_INFO:
						root[columns.value] = "Informational";
						break;

					case LOG_LEVEL_WARN:
						root[columns.value] = "Warning";
						break;

					case LOG_LEVEL_ERROR:
						root[columns.value] = "Error";
						break;

					default:
						root[columns.value] = "<UNKNOWN LEVEL>";
						break;
				}
			} else {
				root[columns.value] = "<PACKET TRUNCATED>";
			}
		}
	}

	/**
	 * A mapping from transmitted byte to descriptive text of referee box command.
	 */
	const struct {
		char command;
		const char *description;
	} REFBOX_MAPPING[] = {
		{ 'H', "Halt" },
		{ 'S', "Stop" },
		{ ' ', "Ready" },
		{ 's', "Force start" },
		{ '1', "Begin first half" },
		{ 'h', "Begin half time" },
		{ '2', "Begin second half" },
		{ 'o', "Begin overtime half 1" },
		{ 'O', "Begin overtime half 2" },
		{ 'a', "Begin penalty shootout" },
		{ 'k', "Kick off yellow" },
		{ 'K', "Kick off blue" },
		{ 'p', "Penalty kick yellow" },
		{ 'P', "Penalty kick blue" },
		{ 'f', "Direct free kick yellow" },
		{ 'F', "Direct free kick blue" },
		{ 'i', "Indirect free kick yellow" },
		{ 'I', "Indirect free kick blue" },
		{ 't', "Timeout yellow" },
		{ 'T', "Timeout blue" },
		{ 'z', "Timeout end" },
		{ 'g', "Goal yellow" },
		{ 'G', "Goal blue" },
		{ 'd', "Ungoal yellow" },
		{ 'D', "Ungoal blue" },
		{ 'y', "Yellow card yellow" },
		{ 'Y', "Yellow card blue" },
		{ 'r', "Red card yellow" },
		{ 'R', "Red card blue" },
		{ 'c', "Cancel" },
	};

	/**
	 * Computes flags for a referee box command character.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 *
	 * \return the packet flags.
	 */
	unsigned int packet_refbox_command_compute_flags(const uint8_t *data, std::size_t length, std::size_t declared_length) {
		unsigned int flags = 0;

		if (declared_length < 1) {
			flags |= PF_SHORT | PF_BAD_NETWORK_PACKET;
		} else if (declared_length > 1) {
			flags |= PF_LONG;
		}

		if (length >= 1) {
			char code = decode_u8(data);
			flags |= PF_BAD_NETWORK_PACKET;
			for (std::size_t i = 0; i < G_N_ELEMENTS(REFBOX_MAPPING); ++i) {
				if (REFBOX_MAPPING[i].command == code) {
					flags &= ~PF_BAD_NETWORK_PACKET;
				}
			}
		}

		return flags;
	}

	/**
	 * Parses a referee box command character and produces a human-readable tree of its contents.
	 *
	 * \param[in] columns the column record containing the columns of the store.
	 *
	 * \param[in] root the tree row under which to build the element's row.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 */
	void packet_refbox_command_build_tree(Glib::RefPtr<Gtk::TreeStore>, const PacketDecodedTreeColumns &columns, const Gtk::TreeRow &root, const uint8_t *data, std::size_t length, std::size_t declared_length) {
		if (declared_length < 1) {
			root[columns.value] = "<OMITTED>";
		} else {
			if (length >= 1) {
				char code = decode_u8(data);
				bool found = false;
				for (std::size_t i = 0; i < G_N_ELEMENTS(REFBOX_MAPPING); ++i) {
					if (REFBOX_MAPPING[i].command == code) {
						root[columns.value] = REFBOX_MAPPING[i].description;
						found = true;
					}
				}
				if (!found) {
					root[columns.value] = Glib::ustring::compose("<UNKNOWN CODE 0x%1>", tohex(code, 2));
				}
			} else {
				root[columns.value] = "<PACKET TRUNCATED>";
			}
		}
	}

	/**
	 * Computes flags for a play type code.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 *
	 * \return the packet flags.
	 */
	unsigned int packet_playtype_compute_flags(const uint8_t *data, std::size_t length, std::size_t declared_length) {
		unsigned int flags = 0;

		if (declared_length < 1) {
			flags |= PF_SHORT | PF_BAD_PLAYTYPE;
		} else if (declared_length > 1) {
			flags |= PF_LONG;
		}

		if (length >= 1) {
			uint8_t code = decode_u8(data);
			if (code >= AI::Common::PlayType::COUNT) {
				flags |= PF_BAD_PLAYTYPE;
			}
		}

		return flags;
	}

	/**
	 * Parses a play type code and produces a human-readable tree of its contents.
	 *
	 * \param[in] columns the column record containing the columns of the store.
	 *
	 * \param[in] root the tree row under which to build the element's row.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 */
	void packet_playtype_build_tree(Glib::RefPtr<Gtk::TreeStore>, const PacketDecodedTreeColumns &columns, const Gtk::TreeRow &root, const uint8_t *data, std::size_t length, std::size_t declared_length) {
		if (declared_length < 1) {
			root[columns.value] = "<OMITTED>";
		} else {
			if (length >= 1) {
				uint8_t code = decode_u8(data);
				if (code < AI::Common::PlayType::COUNT) {
					root[columns.value] = AI::Common::PlayType::DESCRIPTIONS_GENERIC[code];
				} else {
					root[columns.value] = Glib::ustring::compose("<UNKNOWN CODE 0x%1>", tohex(code, 2));
				}
			} else {
				root[columns.value] = "<PACKET TRUNCATED>";
			}
		}
	}

	/**
	 * A mapping from movement flag to descriptive text.
	 */
	const struct {
		uint64_t mask;
		const char *description;
	} MOVE_FLAG_MAPPING[] = {
		{ AI::Flags::FLAG_CLIP_PLAY_AREA, "Clip play area" },
		{ AI::Flags::FLAG_AVOID_BALL_STOP, "Avoid ball (stop)" },
		{ AI::Flags::FLAG_AVOID_BALL_TINY, "Avoid ball (tiny)" },
		{ AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE, "Avoid friendly defense area" },
		{ AI::Flags::FLAG_AVOID_ENEMY_DEFENSE, "Avoid enemy defense area" },
		{ AI::Flags::FLAG_STAY_OWN_HALF, "Stay on own half" },
		{ AI::Flags::FLAG_PENALTY_KICK_FRIENDLY, "Non-kicker in friendly penalty kick" },
		{ AI::Flags::FLAG_PENALTY_KICK_ENEMY, "Non-kicker in enemy penalty kick" },
	};

	/**
	 * Computes flags for a movement flag mask.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 *
	 * \return the packet flags.
	 */
	unsigned int packet_friendly_robot_move_flags_compute_flags(const uint8_t *data, std::size_t length, std::size_t declared_length) {
		unsigned int flags = 0;

		if (declared_length < 8) {
			flags |= PF_SHORT;
		} else if (declared_length > 8) {
			flags |= PF_LONG;
		}

		if (length >= 8) {
			uint64_t mask = decode_u64(data);
			for (std::size_t i = 0; i < G_N_ELEMENTS(MOVE_FLAG_MAPPING); ++i) {
				mask &= ~MOVE_FLAG_MAPPING[i].mask;
			}
			if (mask) {
				flags |= PF_UNKNOWN_MOVE_FLAG;
			}
		}

		return flags;
	}

	/**
	 * Parses a movement flag mask and produces a human-readable tree of its contents.
	 *
	 * \param[in] store the tree store into which to store the tree.
	 *
	 * \param[in] columns the column record containing the columns of the store.
	 *
	 * \param[in] root the tree row under which to build the element's row.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 */
	void packet_friendly_robot_move_flags_build_tree(Glib::RefPtr<Gtk::TreeStore> store, const PacketDecodedTreeColumns &columns, const Gtk::TreeRow &root, const uint8_t *data, std::size_t length, std::size_t declared_length) {
		if (declared_length < 8) {
			root[columns.value] = "<OMITTED>";
		} else {
			if (length >= 8) {
				uint64_t mask = decode_u64(data);
				root[columns.value] = Glib::ustring::compose("0x%1", tohex(mask, 0));
				for (std::size_t i = 0; i < G_N_ELEMENTS(MOVE_FLAG_MAPPING); ++i) {
					if (mask & MOVE_FLAG_MAPPING[i].mask) {
						mask &= ~MOVE_FLAG_MAPPING[i].mask;
						columns.append_kv(store, root, MOVE_FLAG_MAPPING[i].description);
					}
				}
				if (mask) {
					columns.append_kv(store, root, "Unknown flags", tohex(mask, 0));
				}
			} else {
				root[columns.value] = "<PACKET TRUNCATED>";
			}
		}
	}

	/**
	 * A mapping from movement type to descriptive text.
	 */
	const struct {
		AI::Flags::MOVE_TYPE type;
		const char *description;
	} MOVE_TYPE_MAPPING[] = {
		{ AI::Flags::MOVE_NORMAL, "Normal" },
		{ AI::Flags::MOVE_DRIBBLE, "Dribble" },
		{ AI::Flags::MOVE_CATCH, "Catch ball" },
		{ AI::Flags::MOVE_RAM_BALL, "Ram ball" },
		{ AI::Flags::MOVE_HALT, "Halt" },
	};

	/**
	 * Computes flags for a movement type.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 *
	 * \return the packet flags.
	 */
	unsigned int packet_friendly_robot_move_type_compute_flags(const uint8_t *data, std::size_t length, std::size_t declared_length) {
		unsigned int flags = 0;

		if (declared_length < 1) {
			flags |= PF_SHORT;
		} else if (declared_length > 1) {
			flags |= PF_LONG;
		}

		if (length >= 1) {
			uint8_t type = decode_u8(data);
			flags |= PF_UNKNOWN_MOVE_TYPE;
			for (std::size_t i = 0; i < G_N_ELEMENTS(MOVE_TYPE_MAPPING); ++i) {
				if (type == MOVE_TYPE_MAPPING[i].type) {
					flags &= ~PF_UNKNOWN_MOVE_TYPE;
				}
			}
		}

		return flags;
	}

	/**
	 * Parses a movement type and produces a human-readable tree of its contents.
	 *
	 * \param[in] columns the column record containing the columns of the store.
	 *
	 * \param[in] root the tree row under which to build the element's row.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 */
	void packet_friendly_robot_move_type_build_tree(Glib::RefPtr<Gtk::TreeStore>, const PacketDecodedTreeColumns &columns, const Gtk::TreeRow &root, const uint8_t *data, std::size_t length, std::size_t declared_length) {
		if (declared_length < 1) {
			root[columns.value] = "<OMITTED>";
		} else {
			if (length >= 1) {
				uint8_t type = decode_u8(data);
				bool found = false;
				for (std::size_t i = 0; i < G_N_ELEMENTS(MOVE_TYPE_MAPPING); ++i) {
					if (type == MOVE_TYPE_MAPPING[i].type) {
						found = true;
						root[columns.value] = MOVE_TYPE_MAPPING[i].description;
					}
				}
				if (!found) {
					root[columns.value] = Glib::ustring::compose("<UNKNOWN TYPE 0x%1>", tohex(type, 2));
				}
			} else {
				root[columns.value] = "<PACKET TRUNCATED>";
			}
		}
	}

	/**
	 * A mapping from movement priority to descriptive text.
	 */
	const struct {
		AI::Flags::MOVE_PRIO prio;
		const char *description;
	} MOVE_PRIO_MAPPING[] = {
		{ AI::Flags::PRIO_HIGH, "High" },
		{ AI::Flags::PRIO_MEDIUM, "Medium" },
		{ AI::Flags::PRIO_LOW, "Low" },
	};

	/**
	 * Computes flags for a movement priority.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 *
	 * \return the packet flags.
	 */
	unsigned int packet_friendly_robot_move_prio_compute_flags(const uint8_t *data, std::size_t length, std::size_t declared_length) {
		unsigned int flags = 0;

		if (declared_length < 1) {
			flags |= PF_SHORT;
		} else if (declared_length > 1) {
			flags |= PF_LONG;
		}

		if (length >= 1) {
			uint8_t prio = decode_u8(data);
			flags |= PF_UNKNOWN_MOVE_PRIO;
			for (std::size_t i = 0; i < G_N_ELEMENTS(MOVE_PRIO_MAPPING); ++i) {
				if (prio == MOVE_PRIO_MAPPING[i].prio) {
					flags &= ~PF_UNKNOWN_MOVE_PRIO;
				}
			}
		}

		return flags;
	}

	/**
	 * Parses a movement priority and produces a human-readable tree of its contents.
	 *
	 * \param[in] columns the column record containing the columns of the store.
	 *
	 * \param[in] root the tree row under which to build the element's row.
	 *
	 * \param[in] data this element's part of the packet payload.
	 *
	 * \param[in] length the length of the payload data.
	 *
	 * \param[in] declared_length the part of the length declared in the header that would belong to this element
	 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
	 */
	void packet_friendly_robot_move_prio_build_tree(Glib::RefPtr<Gtk::TreeStore>, const PacketDecodedTreeColumns &columns, const Gtk::TreeRow &root, const uint8_t *data, std::size_t length, std::size_t declared_length) {
		if (declared_length < 1) {
			root[columns.value] = "<OMITTED>";
		} else {
			if (length >= 1) {
				uint8_t prio = decode_u8(data);
				bool found = false;
				for (std::size_t i = 0; i < G_N_ELEMENTS(MOVE_PRIO_MAPPING); ++i) {
					if (prio == MOVE_PRIO_MAPPING[i].prio) {
						found = true;
						root[columns.value] = MOVE_PRIO_MAPPING[i].description;
					}
				}
				if (!found) {
					root[columns.value] = Glib::ustring::compose("<UNKNOWN PRIORITY 0x%1>", tohex(prio, 2));
				}
			} else {
				root[columns.value] = "<PACKET TRUNCATED>";
			}
		}
	}

	/**
	 * An abstract packet parser.
	 */
	class PacketParser : public NonCopyable {
		public:
			/**
			 * The tag of packets understood by this parser.
			 */
			const Log::Tag tag;

			/**
			 * The human-readable name of packets understood by this parser.
			 */
			const Glib::ustring name;

			/**
			 * Constructs a new PacketParser.
			 *
			 * \param[in] tag the tag of packets understood by this parser.
			 *
			 * \param[in] name the human-readable name of packets understood by this parser.
			 */
			PacketParser(Log::Tag tag, const Glib::ustring &name);

			/**
			 * Destroys a PacketParser.
			 */
			~PacketParser();

			/**
			 * Parses a packet and computes its corresponding flags.
			 *
			 * \param[in] data the payload of the packet.
			 *
			 * \param[in] length the length of the payload.
			 *
			 * \param[in] declared_length the length of the payload as declared in the header
			 * (which will be greater than \p length if the packet is truncated).
			 *
			 * \return the packet flags.
			 */
			virtual unsigned int compute_flags(const uint8_t *data, std::size_t length, std::size_t declared_length) const = 0;

			/**
			 * Parses a packet and produces a human-readable tree of its contents.
			 *
			 * \param[in] store the tree store into which to store the tree.
			 *
			 * \param[in] columns the column record containing the columns of the store.
			 *
			 * \param[in] root the tree row under which to build the data tree.
			 *
			 * \param[in] data the payload of the packet.
			 *
			 * \param[in] length the length of the payload.
			 *
			 * \param[in] declared_length the length of the payload as declared in the header
			 * (which will be greater than \p length if the packet is truncated).
			 */
			virtual void build_tree(Glib::RefPtr<Gtk::TreeStore> store, const PacketDecodedTreeColumns &columns, const Gtk::TreeRow &root, const uint8_t *data, std::size_t length, std::size_t declared_length) const = 0;
	};

	PacketParser::PacketParser(Log::Tag tag, const Glib::ustring &name) : tag(tag), name(name) {
	}

	PacketParser::~PacketParser() {
	}

	/**
	 * A packet parser for a packet containing a flat sequence of basic elements,
	 * the last of which may optionally be of unbounded length.
	 */
	class SequencePacketParser : public PacketParser {
		public:
			/**
			 * An element of the sequence.
			 */
			struct Element {
				/**
				 * The length of the element, in bytes, or zero if this element consumes any amount of data from the end of the packet.
				 */
				const std::size_t length;

				/**
				 * A text string naming the element.
				 */
				const Glib::ustring name;

				/**
				 * A function that computes the packet flags contributed by the element.
				 *
				 * \param[in] data this element's part of the packet payload.
				 *
				 * \param[in] length the length of the payload data.
				 *
				 * \param[in] declared_length the part of the length declared in the header that would belong to this element
				 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
				 *
				 * \return the packet flags.
				 */
				unsigned int(*const compute_flags) (const uint8_t *, std::size_t, std::size_t);

				/**
				 * A function that parses an element and produces a human-readable tree of its contents.
				 *
				 * \param[in] store the tree store into which to store the tree.
				 *
				 * \param[in] columns the column record containing the columns of the store.
				 *
				 * \param[in] root the tree row under which to build the element's row.
				 *
				 * \param[in] data this element's part of the packet payload.
				 *
				 * \param[in] length the length of the payload data.
				 *
				 * \param[in] declared_length the part of the length declared in the header that would belong to this element
				 * (which will be greater than \p length for some suffix of the elements of a truncated packet).
				 */
				void(*const build_tree) (Glib::RefPtr<Gtk::TreeStore>, const PacketDecodedTreeColumns &, const Gtk::TreeRow &, const uint8_t *, std::size_t, std::size_t);
			};

			/**
			 * A pointer to an array of Element objects describing the sequence.
			 */
			const Element *const elements;

			/**
			 * The number of Element objects in the elements array.
			 */
			const std::size_t num_elements;

			/**
			 * Constructs a new SequencePacketParser.
			 *
			 * \param[in] tag the tag of packets understood by this parser.
			 *
			 * \param[in] name the human-readable name of packets understood by this parser.
			 *
			 * \param[in] elements a pointer to an array of Element objects describing the sequence.
			 *
			 * \param[in] num_elements the number of Element objects in the \p elements array.
			 */
			SequencePacketParser(Log::Tag tag, const Glib::ustring &name, const Element *elements, std::size_t num_elements);

			/**
			 * Destroys a SequencePacketParser.
			 */
			~SequencePacketParser();

			unsigned int compute_flags(const uint8_t *data, std::size_t length, std::size_t declared_length) const;

			void build_tree(Glib::RefPtr<Gtk::TreeStore> store, const PacketDecodedTreeColumns &columns, const Gtk::TreeRow &root, const uint8_t *data, std::size_t length, std::size_t declared_length) const;
	};

	SequencePacketParser::SequencePacketParser(Log::Tag tag, const Glib::ustring &name, const Element *elements, std::size_t num_elements) : PacketParser(tag, name), elements(elements), num_elements(num_elements) {
	}

	SequencePacketParser::~SequencePacketParser() {
	}

	unsigned int SequencePacketParser::compute_flags(const uint8_t *data, std::size_t length, std::size_t declared_length) const {
		unsigned int flags = 0;

		for (std::size_t i = 0; i < num_elements; ++i) {
			std::size_t element_length, element_declared_length;
			if (elements[i].length > 0) {
				element_length = std::min(length, elements[i].length);
				element_declared_length = std::min(declared_length, elements[i].length);
			} else {
				element_length = length;
				element_declared_length = declared_length;
			}
			flags |= elements[i].compute_flags(data, element_length, element_declared_length);
			length -= element_length;
			declared_length -= element_declared_length;
			if (length) {
				data += element_length;
			} else {
				data = 0;
			}
		}

		if (declared_length > 0) {
			flags |= PF_LONG;
		}

		return flags;
	}

	void SequencePacketParser::build_tree(Glib::RefPtr<Gtk::TreeStore> store, const PacketDecodedTreeColumns &columns, const Gtk::TreeRow &root, const uint8_t *data, std::size_t length, std::size_t declared_length) const {
		root[columns.value] = name;

		for (std::size_t i = 0; i < num_elements; ++i) {
			std::size_t element_length, element_declared_length;
			if (elements[i].length > 0) {
				element_length = std::min(length, elements[i].length);
				element_declared_length = std::min(declared_length, elements[i].length);
			} else {
				element_length = length;
				element_declared_length = declared_length;
			}
			Gtk::TreeRow row = columns.append_kv(store, root, elements[i].name);
			elements[i].build_tree(store, columns, row, data, element_length, element_declared_length);
			length -= element_length;
			declared_length -= element_declared_length;
			if (length) {
				data += element_length;
			} else {
				data = 0;
			}
		}
	}

	/**
	 * A packet parser for the end of log packet.
	 */
	class EndOfLogPacketParser : public PacketParser {
		public:
			/**
			 * Constructs a new EndOfLogPacketParser.
			 */
			EndOfLogPacketParser();

			/**
			 * Destroys an EndOfLogPacketParser.
			 */
			~EndOfLogPacketParser();

			unsigned int compute_flags(const uint8_t *data, std::size_t length, std::size_t declared_length) const;

			void build_tree(Glib::RefPtr<Gtk::TreeStore> store, const PacketDecodedTreeColumns &columns, const Gtk::TreeRow &root, const uint8_t *data, std::size_t length, std::size_t declared_length) const;
	};

	EndOfLogPacketParser::EndOfLogPacketParser() : PacketParser(Log::T_END, "End of Log") {
	}

	EndOfLogPacketParser::~EndOfLogPacketParser() {
	}

	unsigned int EndOfLogPacketParser::compute_flags(const uint8_t *data, std::size_t length, std::size_t declared_length) const {
		unsigned int flags = 0;

		if (declared_length < 1) {
			flags |= PF_SHORT;
		} else if (length >= 1) {
			uint8_t reason = decode_u8(&data[0]);
			switch (reason) {
				case Log::ER_NORMAL:
					if (declared_length > 1) {
						flags |= PF_LONG;
					}
					break;

				case Log::ER_SIGNAL:
					if (declared_length < 2) {
						flags |= PF_SHORT;
					} else if (declared_length > 2) {
						flags |= PF_LONG;
					}
					break;

				case Log::ER_EXCEPTION:
					flags |= packet_generic_utf8_compute_flags(&data[1], length - 1, declared_length - 1);
					break;

				default:
					flags |= PF_UNKNOWN_END_REASON;
					break;
			}
		}

		return flags;
	}

	void EndOfLogPacketParser::build_tree(Glib::RefPtr<Gtk::TreeStore> store, const PacketDecodedTreeColumns &columns, const Gtk::TreeRow &root, const uint8_t *data, std::size_t length, std::size_t declared_length) const {
		root[columns.value] = name;

		Gtk::TreeRow reason_row = columns.append_kv(store, root, "Reason");

		if (declared_length < 1) {
			reason_row[columns.value] = "<OMITTED>";
		} else if (length >= 1) {
			uint8_t reason = decode_u8(&data[0]);
			switch (reason) {
				case Log::ER_NORMAL:
					reason_row[columns.value] = "Normal termination";
					break;

				case Log::ER_SIGNAL:
				{
					reason_row[columns.value] = "Fatal signal";
					Gtk::TreeRow signal_row = columns.append_kv(store, reason_row, "Signal");
					if (declared_length < 2) {
						signal_row[columns.value] = "<OMITTED>";
					} else if (length < 2) {
						signal_row[columns.value] = "<PACKET TRUNCATED>";
					} else {
						int sig = decode_u8(&data[1]);
						signal_row[columns.value] = Glib::ustring::compose("%1 (%2)", sig, strsignal(sig));
					}
					break;
				}

				case Log::ER_EXCEPTION:
				{
					reason_row[columns.value] = "Uncaught exception";
					Gtk::TreeRow message_row = columns.append_kv(store, reason_row, "Message");
					packet_generic_utf8_build_tree(store, columns, message_row, &data[1], length - 1, declared_length - 1);
					break;
				}

				default:
					reason_row[columns.value] = Glib::ustring::compose("<UNKNOWN REASON 0x%1>", tohex(reason, 2));
					break;
			}
		}
	}

	/**
	 * A packet parser for the SSL-Vision packet.
	 */
	class SSLVisionPacketParser : public PacketParser {
		public:
			/**
			 * Constructs a new SSLVisionPacketParser.
			 */
			SSLVisionPacketParser();

			/**
			 * Destroys an SSLVisionPacketParser.
			 */
			~SSLVisionPacketParser();

			unsigned int compute_flags(const uint8_t *data, std::size_t length, std::size_t declared_length) const;

			void build_tree(Glib::RefPtr<Gtk::TreeStore> store, const PacketDecodedTreeColumns &columns, const Gtk::TreeRow &root, const uint8_t *data, std::size_t length, std::size_t declared_length) const;
	};

	SSLVisionPacketParser::SSLVisionPacketParser() : PacketParser(Log::T_VISION, "SSL-Vision Packet") {
	}

	SSLVisionPacketParser::~SSLVisionPacketParser() {
	}

	unsigned int SSLVisionPacketParser::compute_flags(const uint8_t *data, std::size_t length, std::size_t) const {
		SSL_WrapperPacket pkt;
		if (pkt.ParseFromArray(data, static_cast<int>(length))) {
			return 0;
		} else {
			return PF_BAD_NETWORK_PACKET;
		}
	}

	void SSLVisionPacketParser::build_tree(Glib::RefPtr<Gtk::TreeStore> store, const PacketDecodedTreeColumns &columns, const Gtk::TreeRow &root, const uint8_t *data, std::size_t length, std::size_t declared_length) const {
		root[columns.value] = name;
		Gtk::TreeRow decode_row = columns.append_kv(store, root, "Wrapper", "<PACKET TRUNCATED>");
		if (length == declared_length) {
			SSL_WrapperPacket wrapper;
			if (wrapper.ParseFromArray(data, static_cast<int>(length))) {
				decode_row[columns.value] = "OK";
				if (wrapper.has_detection()) {
					const SSL_DetectionFrame &det = wrapper.detection();
					Gtk::TreeRow detection_row = columns.append_kv(store, decode_row, "Detection");
					columns.append_kv(store, detection_row, "Frame #", Glib::ustring::format(det.frame_number()));
					columns.append_kv(store, detection_row, "Capture time", double_to_time_string(det.t_capture()));
					columns.append_kv(store, detection_row, "Sent time", double_to_time_string(det.t_sent()));
					columns.append_kv(store, detection_row, "Camera", Glib::ustring::format(det.camera_id()));
					Gtk::TreeRow balls_row = columns.append_kv(store, detection_row, "Balls", Glib::ustring::format(det.balls_size()));
					for (int i = 0; i < det.balls_size(); ++i) {
						const SSL_DetectionBall &ball = det.balls(i);
						Gtk::TreeRow ball_row = columns.append_kv(store, balls_row, Glib::ustring::compose("Ball #%1", i));
						columns.append_kv(store, ball_row, "Confidence", Glib::ustring::format(ball.confidence()));
						columns.append_kv(store, ball_row, "Area", ball.has_area() ? Glib::ustring::format(ball.area()) : "<OMITTED>");
						columns.append_kv(store, ball_row, "Position", Glib::ustring::compose("(%1, %2)", ball.x(), ball.y()));
						columns.append_kv(store, ball_row, "Altitude", ball.has_z() ? Glib::ustring::format(ball.z()) : "<OMITTED>");
						columns.append_kv(store, ball_row, "Pixel position", Glib::ustring::compose("(%1, %2)", ball.pixel_x(), ball.pixel_y()));
					}
					static const char *const team_colours[2] = { "Yellow Robots", "Blue Robots" };
					static const SSL_DetectionRobot &(SSL_DetectionFrame::*const team_members[2])(int) const = { &SSL_DetectionFrame::robots_yellow, &SSL_DetectionFrame::robots_blue };
					static int(SSL_DetectionFrame::*const team_sizes[2]) () const = { &SSL_DetectionFrame::robots_yellow_size, &SSL_DetectionFrame::robots_blue_size };
					for (unsigned int i = 0; i < 2; ++i) {
						Gtk::TreeRow team_row = columns.append_kv(store, detection_row, team_colours[i]);
						for (int j = 0; j < (det.*team_sizes[i])(); ++j) {
							const SSL_DetectionRobot &bot = (det.*team_members[i])(j);
							Gtk::TreeRow robot_row = columns.append_kv(store, team_row, Glib::ustring::compose("Robot #%1", j));
							columns.append_kv(store, robot_row, "Confidence", Glib::ustring::format(bot.confidence()));
							columns.append_kv(store, robot_row, "ID", bot.has_robot_id() ? Glib::ustring::format(bot.robot_id()) : "<OMITTED>");
							columns.append_kv(store, robot_row, "Position", Glib::ustring::compose("(%1, %2)", bot.x(), bot.y()));
							columns.append_kv(store, robot_row, "Orientation", bot.has_orientation() ? Glib::ustring::format(bot.orientation()) : "<OMITTED>");
							columns.append_kv(store, robot_row, "Pixel position", Glib::ustring::compose("(%1, %2)", bot.pixel_x(), bot.pixel_y()));
							columns.append_kv(store, robot_row, "Height", bot.has_height() ? Glib::ustring::format(bot.height()) : "<OMITTED>");
						}
					}
				}
				if (wrapper.has_geometry()) {
					const SSL_GeometryData &geom = wrapper.geometry();
					Gtk::TreeRow geom_row = columns.append_kv(store, decode_row, "Geometry");
					const SSL_GeometryFieldSize &field = geom.field();
					Gtk::TreeRow field_row = columns.append_kv(store, geom_row, "Field");
					columns.append_kv(store, field_row, "Line width", Glib::ustring::format(field.line_width()));
					columns.append_kv(store, field_row, "Field length", Glib::ustring::format(field.field_length()));
					columns.append_kv(store, field_row, "Field width", Glib::ustring::format(field.field_width()));
					columns.append_kv(store, field_row, "Boundary width", Glib::ustring::format(field.boundary_width()));
					columns.append_kv(store, field_row, "Referee width", Glib::ustring::format(field.referee_width()));
					columns.append_kv(store, field_row, "Goal width", Glib::ustring::format(field.goal_width()));
					columns.append_kv(store, field_row, "Goal depth", Glib::ustring::format(field.goal_depth()));
					columns.append_kv(store, field_row, "Goal wall width", Glib::ustring::format(field.goal_wall_width()));
					columns.append_kv(store, field_row, "Centre circle radius", Glib::ustring::format(field.center_circle_radius()));
					columns.append_kv(store, field_row, "Defense radius", Glib::ustring::format(field.defense_radius()));
					columns.append_kv(store, field_row, "Defense stretch", Glib::ustring::format(field.defense_stretch()));
					columns.append_kv(store, field_row, "Free kick from defense distance", Glib::ustring::format(field.free_kick_from_defense_dist()));
					columns.append_kv(store, field_row, "Penalty spot from field line distance", Glib::ustring::format(field.penalty_spot_from_field_line_dist()));
					columns.append_kv(store, field_row, "Penalty line from spot distance", Glib::ustring::format(field.penalty_line_from_spot_dist()));
					for (int i = 0; i < geom.calib_size(); ++i) {
						const SSL_GeometryCameraCalibration &calib = geom.calib(i);
						Gtk::TreeRow calib_row = columns.append_kv(store, geom_row, "Camera", Glib::ustring::format(calib.camera_id()));
						columns.append_kv(store, calib_row, "Focal length", Glib::ustring::format(calib.focal_length()));
						columns.append_kv(store, calib_row, "Principal point", Glib::ustring::compose("(%1, %2)", calib.principal_point_x(), calib.principal_point_y()));
						columns.append_kv(store, calib_row, "Distortion", Glib::ustring::format(calib.distortion()));
						columns.append_kv(store, calib_row, "q0", Glib::ustring::format(calib.q0()));
						columns.append_kv(store, calib_row, "q1", Glib::ustring::format(calib.q1()));
						columns.append_kv(store, calib_row, "q2", Glib::ustring::format(calib.q2()));
						columns.append_kv(store, calib_row, "q3", Glib::ustring::format(calib.q3()));
						columns.append_kv(store, calib_row, "tx", Glib::ustring::format(calib.tx()));
						columns.append_kv(store, calib_row, "ty", Glib::ustring::format(calib.ty()));
						columns.append_kv(store, calib_row, "tz", Glib::ustring::format(calib.tz()));
						columns.append_kv(store, calib_row, "Derived camera world tx", calib.has_derived_camera_world_tx() ? Glib::ustring::format(calib.derived_camera_world_tx()) : "<OMITTED>");
						columns.append_kv(store, calib_row, "Derived camera world ty", calib.has_derived_camera_world_ty() ? Glib::ustring::format(calib.derived_camera_world_ty()) : "<OMITTED>");
						columns.append_kv(store, calib_row, "Derived camera world tz", calib.has_derived_camera_world_tz() ? Glib::ustring::format(calib.derived_camera_world_tz()) : "<OMITTED>");
					}
				}
			} else {
				decode_row[columns.value] = "<PARSE FAILED>";
			}
		}
	}

	/**
	 * The sequence of elements in a Log::T_START.
	 */
	const SequencePacketParser::Element packet_start_elements[] = {
	};

	/**
	 * A parser for a Log::T_START.
	 */
	const SequencePacketParser packet_start_parser(Log::T_START, "Start of Log", packet_start_elements, G_N_ELEMENTS(packet_start_elements));

	/**
	 * A parser for a Log::T_END.
	 */
	const EndOfLogPacketParser packet_end_parser;

	/**
	 * The sequence of elements in a Log::T_DEBUG.
	 */
	const SequencePacketParser::Element packet_debug_elements[] = {
		{ length: 1, name: "Log level", compute_flags: &packet_debug_log_level_compute_flags, build_tree: &packet_debug_log_level_build_tree, },
		{ length: 0, name: "Message", compute_flags: &packet_generic_utf8_compute_flags, build_tree: &packet_generic_utf8_build_tree },
	};

	/**
	 * A parser for a Log::T_DEBUG.
	 */
	const SequencePacketParser packet_debug_parser(Log::T_DEBUG, "Debug Message", packet_debug_elements, G_N_ELEMENTS(packet_debug_elements));

	/**
	 * The sequence of elements in a Log::T_ANNUNCIATOR.
	 */
	const SequencePacketParser::Element packet_annunciator_elements[] = {
		{ length: 1, name: "Activate", compute_flags: &packet_generic_boolean_compute_flags, build_tree: &packet_generic_boolean_build_tree },
		{ length: 0, name: "Message", compute_flags: &packet_generic_utf8_compute_flags, build_tree: &packet_generic_utf8_build_tree },
	};

	/**
	 * A parser for a Log::T_ANNUNCIATOR.
	 */
	const SequencePacketParser packet_annunciator_parser(Log::T_ANNUNCIATOR, "Annunciator Message", packet_annunciator_elements, G_N_ELEMENTS(packet_annunciator_elements));

	/**
	 * The sequence of elements in a Log::T_BOOL_PARAM.
	 */
	const SequencePacketParser::Element packet_bool_param_elements[] = {
		{ length: 1, name: "Value", compute_flags: &packet_generic_boolean_compute_flags, build_tree: &packet_generic_boolean_build_tree },
		{ length: 0, name: "Parameter", compute_flags: &packet_generic_utf8_compute_flags, build_tree: &packet_generic_utf8_build_tree },
	};

	/**
	 * A parser for a Log::T_BOOL_PARAM.
	 */
	const SequencePacketParser packet_bool_param_parser(Log::T_BOOL_PARAM, "Parameter Changed", packet_bool_param_elements, G_N_ELEMENTS(packet_bool_param_elements));

	/**
	 * The sequence of elements in a Log::T_INT_PARAM.
	 */
	const SequencePacketParser::Element packet_int_param_elements[] = {
		{ length: 8, name: "Value", compute_flags: &packet_generic_int64_compute_flags, build_tree: &packet_generic_int64_build_tree },
		{ length: 0, name: "Parameter", compute_flags: &packet_generic_utf8_compute_flags, build_tree: &packet_generic_utf8_build_tree },
	};

	/**
	 * A parser for a Log::T_INT_PARAM.
	 */
	const SequencePacketParser packet_int_param_parser(Log::T_INT_PARAM, "Parameter Changed", packet_int_param_elements, G_N_ELEMENTS(packet_int_param_elements));

	/**
	 * The sequence of elements in a Log::T_DOUBLE_PARAM_OLD.
	 */
	const SequencePacketParser::Element packet_double_param_old_elements[] = {
		{ length: 20, name: "Value", compute_flags: &packet_generic_old_double_compute_flags, build_tree: &packet_generic_old_double_build_tree },
		{ length: 0, name: "Parameter", compute_flags: &packet_generic_utf8_compute_flags, build_tree: &packet_generic_utf8_build_tree },
	};

	/**
	 * A parser for a Log::T_DOUBLE_PARAM_OLD.
	 */
	const SequencePacketParser packet_double_param_old_parser(Log::T_DOUBLE_PARAM_OLD, "Parameter Changed", packet_double_param_old_elements, G_N_ELEMENTS(packet_double_param_old_elements));

	/**
	 * A parser for a Log::T_VISION.
	 */
	const SSLVisionPacketParser packet_vision_parser;

	/**
	 * The sequence of elements in a Log::T_REFBOX.
	 */
	const SequencePacketParser::Element packet_refbox_elements[] = {
		{ length: 1, name: "Command", compute_flags: &packet_refbox_command_compute_flags, build_tree: &packet_refbox_command_build_tree },
		{ length: 1, name: "Counter", compute_flags: &packet_generic_uint8_compute_flags, build_tree: &packet_generic_uint8_build_tree },
		{ length: 1, name: "Goals Blue", compute_flags: &packet_generic_uint8_compute_flags, build_tree: &packet_generic_uint8_build_tree },
		{ length: 1, name: "Goals Yellow", compute_flags: &packet_generic_uint8_compute_flags, build_tree: &packet_generic_uint8_build_tree },
		{ length: 2, name: "Time Remaining", compute_flags: &packet_generic_uint16_compute_flags, build_tree: &packet_generic_uint16_build_tree },
	};

	/**
	 * A parser for a Log::T_REFBOX.
	 */
	const SequencePacketParser packet_refbox_parser(Log::T_REFBOX, "Referee Box Packet", packet_refbox_elements, G_N_ELEMENTS(packet_refbox_elements));

	/**
	 * The sequence of elements in a Log::T_FIELD.
	 */
	const SequencePacketParser::Element packet_field_elements[] = {
		{ length: 4, name: "Length", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Total length", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Width", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Total width", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Goal width", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Centre circle radius", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Defense arc radius", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Defense area stretch", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
	};

	/**
	 * A parser for a Log::T_FIELD.
	 */
	const SequencePacketParser packet_field_parser(Log::T_FIELD, "Field Geometry", packet_field_elements, G_N_ELEMENTS(packet_field_elements));

	/**
	 * The sequence of elements in a Log::T_BALL_FILTER, Log::T_COACH, Log::T_STRATEGY, or Log::T_ROBOT_CONTROLLER.
	 */
	const SequencePacketParser::Element packet_bfcsrc_elements[] = {
		{ length: 0, name: "Name", compute_flags: &packet_generic_utf8_compute_flags, build_tree: &packet_generic_utf8_build_tree },
	};

	/**
	 * A parser for a Log::T_BALL_FILTER.
	 */
	const SequencePacketParser packet_ball_filter_parser(Log::T_BALL_FILTER, "Ball Filter Changed", packet_bfcsrc_elements, G_N_ELEMENTS(packet_bfcsrc_elements));

	/**
	 * A parser for a Log::T_COACH.
	 */
	const SequencePacketParser packet_coach_parser(Log::T_COACH, "Coach Changed", packet_bfcsrc_elements, G_N_ELEMENTS(packet_bfcsrc_elements));

	/**
	 * A parser for a Log::T_STRATEGY.
	 */
	const SequencePacketParser packet_strategy_parser(Log::T_STRATEGY, "Strategy Changed", packet_bfcsrc_elements, G_N_ELEMENTS(packet_bfcsrc_elements));

	/**
	 * A parser for a Log::T_ROBOT_CONTROLLER.
	 */
	const SequencePacketParser packet_robot_controller_parser(Log::T_ROBOT_CONTROLLER, "Robot Controller Changed", packet_bfcsrc_elements, G_N_ELEMENTS(packet_bfcsrc_elements));

	/**
	 * The sequence of elements in a Log::T_PLAYTYPE.
	 */
	const SequencePacketParser::Element packet_playtype_elements[] = {
		{ length: 1, name: "Play type", compute_flags: &packet_playtype_compute_flags, build_tree: &packet_playtype_build_tree },
	};

	/**
	 * A parser for a Log::T_PLAYTYPE.
	 */
	const SequencePacketParser packet_playtype_parser(Log::T_PLAYTYPE, "Play Type Changed", packet_playtype_elements, G_N_ELEMENTS(packet_playtype_elements));

	/**
	 * The sequence of elements in a Log::T_SCORES.
	 */
	const SequencePacketParser::Element packet_scores_elements[] = {
		{ length: 1, name: "Friendly score", compute_flags: &packet_generic_uint8_compute_flags, build_tree: &packet_generic_uint8_build_tree },
		{ length: 1, name: "Enemy score", compute_flags: &packet_generic_uint8_compute_flags, build_tree: &packet_generic_uint8_build_tree },
	};

	/**
	 * A parser for a Log::T_SCORES.
	 */
	const SequencePacketParser packet_scores_parser(Log::T_SCORES, "Team Scores", packet_scores_elements, G_N_ELEMENTS(packet_scores_elements));

	/**
	 * The sequence of elements in a Log::T_FRIENDLY_ROBOT.
	 */
	const SequencePacketParser::Element packet_friendly_robot_elements[] = {
		{ length: 1, name: "Pattern", compute_flags: &packet_generic_pattern_compute_flags, build_tree: &packet_generic_pattern_build_tree },
		{ length: 4, name: "X position", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Y position", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Orientation", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "X velocity", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Y velocity", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Angular velocity", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "X acceleration", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Y acceleration", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Angular acceleration", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Target X position", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Target Y position", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Target orientation", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 8, name: "Movement flags", compute_flags: &packet_friendly_robot_move_flags_compute_flags, build_tree: &packet_friendly_robot_move_flags_build_tree },
		{ length: 1, name: "Movement type", compute_flags: &packet_friendly_robot_move_type_compute_flags, build_tree: &packet_friendly_robot_move_type_build_tree },
		{ length: 1, name: "Movement priority", compute_flags: &packet_friendly_robot_move_prio_compute_flags, build_tree: &packet_friendly_robot_move_prio_build_tree },
		{ length: 2, name: "Wheel 1 speed", compute_flags: &packet_generic_int16_compute_flags, build_tree: &packet_generic_int16_build_tree },
		{ length: 2, name: "Wheel 2 speed", compute_flags: &packet_generic_int16_compute_flags, build_tree: &packet_generic_int16_build_tree },
		{ length: 2, name: "Wheel 3 speed", compute_flags: &packet_generic_int16_compute_flags, build_tree: &packet_generic_int16_build_tree },
		{ length: 2, name: "Wheel 4 speed", compute_flags: &packet_generic_int16_compute_flags, build_tree: &packet_generic_int16_build_tree },
	};

	/**
	 * A parser for a Log::T_FRIENDLY_ROBOT.
	 */
	const SequencePacketParser packet_friendly_robot_parser(Log::T_FRIENDLY_ROBOT, "Friendly Robot Data", packet_friendly_robot_elements, G_N_ELEMENTS(packet_friendly_robot_elements));

	/**
	 * The sequence of elements in a Log::T_PATH_ELEMENT.
	 */
	const SequencePacketParser::Element packet_path_element_elements[] = {
		{ length: 1, name: "Pattern", compute_flags: &packet_generic_pattern_compute_flags, build_tree: &packet_generic_pattern_build_tree },
		{ length: 4, name: "Target X position", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Target Y position", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Target orientation", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 12, name: "Deadline", compute_flags: &packet_generic_timespec_monotonic_compute_flags, build_tree: &packet_generic_timespec_monotonic_build_tree },
	};

	/**
	 * A parser for a Log::T_PATH_ELEMENT.
	 */
	const SequencePacketParser packet_path_element_parser(Log::T_PATH_ELEMENT, "Navigator Path Element", packet_path_element_elements, G_N_ELEMENTS(packet_path_element_elements));

	/**
	 * The sequence of elements in a Log::T_ENEMY_ROBOT.
	 */
	const SequencePacketParser::Element packet_enemy_robot_elements[] = {
		{ length: 1, name: "Pattern", compute_flags: &packet_generic_pattern_compute_flags, build_tree: &packet_generic_pattern_build_tree },
		{ length: 4, name: "X position", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Y position", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Orientation", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "X velocity", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Y velocity", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Angular velocity", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "X acceleration", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Y acceleration", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Angular acceleration", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
	};

	/**
	 * A parser for a Log::T_ENEMY_ROBOT.
	 */
	const SequencePacketParser packet_enemy_robot_parser(Log::T_ENEMY_ROBOT, "Enemy Robot Data", packet_enemy_robot_elements, G_N_ELEMENTS(packet_enemy_robot_elements));

	/**
	 * The sequence of elements in a Log::T_BALL.
	 */
	const SequencePacketParser::Element packet_ball_elements[] = {
		{ length: 4, name: "X position", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Y position", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "X velocity", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Y velocity", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "X acceleration", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
		{ length: 4, name: "Y acceleration", compute_flags: &packet_generic_int32u_compute_flags, build_tree: &packet_generic_int32u_build_tree },
	};

	/**
	 * A parser for a Log::T_BALL.
	 */
	const SequencePacketParser packet_ball_parser(Log::T_BALL, "Ball Data", packet_ball_elements, G_N_ELEMENTS(packet_ball_elements));

	/**
	 * The sequence of elements in a Log::T_AI_TICK.
	 */
	const SequencePacketParser::Element packet_ai_tick_elements[] = {
		{ length: 12, name: "Time", compute_flags: &packet_generic_timespec_rt_compute_flags, build_tree: &packet_generic_timespec_rt_build_tree },
		{ length: 12, name: "Time", compute_flags: &packet_generic_timespec_monotonic_compute_flags, build_tree: &packet_generic_timespec_monotonic_build_tree },
	};

	/**
	 * A parser for a Log::T_AI_TICK.
	 */
	const SequencePacketParser packet_ai_tick_parser(Log::T_AI_TICK, "End of Tick", packet_ai_tick_elements, G_N_ELEMENTS(packet_ai_tick_elements));

	/**
	 * The sequence of elements in a Log::T_BACKEND.
	 */
	const SequencePacketParser::Element packet_backend_elements[] = {
		{ length: 0, name: "Name", compute_flags: &packet_generic_utf8_compute_flags, build_tree: &packet_generic_utf8_build_tree },
	};

	/**
	 * A parser for a Log::T_BACKEND.
	 */
	const SequencePacketParser packet_backend_parser(Log::T_BACKEND, "Backend", packet_backend_elements, G_N_ELEMENTS(packet_backend_elements));

	/**
	 * The sequence of elements in a Log::T_DOUBLE_PARAM.
	 */
	const SequencePacketParser::Element packet_double_param_elements[] = {
		{ length: 8, name: "Value", compute_flags: &packet_generic_double_compute_flags, build_tree: &packet_generic_double_build_tree },
		{ length: 0, name: "Parameter", compute_flags: &packet_generic_utf8_compute_flags, build_tree: &packet_generic_utf8_build_tree },
	};

	/**
	 * A parser for a Log::T_DOUBLE_PARAM.
	 */
	const SequencePacketParser packet_double_param_parser(Log::T_DOUBLE_PARAM, "Parameter Changed", packet_double_param_elements, G_N_ELEMENTS(packet_double_param_elements));

	/**
	 * The set of all packet parsers.
	 */
	const PacketParser *const packet_parsers[] = {
		&packet_start_parser,
		&packet_end_parser,
		&packet_debug_parser,
		&packet_annunciator_parser,
		&packet_bool_param_parser,
		&packet_int_param_parser,
		&packet_double_param_old_parser,
		&packet_vision_parser,
		&packet_refbox_parser,
		&packet_field_parser,
		&packet_ball_filter_parser,
		&packet_coach_parser,
		&packet_strategy_parser,
		&packet_robot_controller_parser,
		&packet_playtype_parser,
		&packet_scores_parser,
		&packet_friendly_robot_parser,
		&packet_path_element_parser,
		&packet_enemy_robot_parser,
		&packet_ball_parser,
		&packet_ai_tick_parser,
		&packet_backend_parser,
		&packet_double_param_parser,
	};

	class PacketInfo {
		public:
			const uint8_t *data;
			std::size_t length;
			unsigned int flags;

			PacketInfo(const uint8_t *data, std::size_t remaining) : data(data), length(0), flags(0) {
				if (remaining < 3) {
					flags |= PF_TRUNCATED;
					length = remaining;
				} else {
					uint8_t tag = decode_u8(&data[0]);
					uint16_t payload_length = decode_u16(&data[1]);
					uint16_t real_payload_length;
					const uint8_t *payload = &data[3];
					if (remaining >= 3U + payload_length + 2U) {
						const uint8_t *footer = payload + payload_length;
						real_payload_length = payload_length;
						length = 3U + payload_length + 2U;
						if (CRC16::calculate(data, 3 + payload_length) != decode_u16(footer)) {
							flags |= PF_BAD_CRC;
						}
					} else {
						flags |= PF_TRUNCATED;
						if (remaining >= 3 + 2) {
							real_payload_length = static_cast<uint16_t>(remaining - 3 - 2);
						} else {
							real_payload_length = 0;
						}
						length = remaining;
					}

					flags |= PF_UNKNOWN_TAG;
					for (std::size_t i = 0; i < G_N_ELEMENTS(packet_parsers); ++i) {
						const PacketParser *parser = packet_parsers[i];
						if (parser->tag == tag) {
							flags &= ~PF_UNKNOWN_TAG;
							flags |= parser->compute_flags(payload, real_payload_length, payload_length);
						}
					}
				}
			}

			bool has_error() const {
				return !!(flags & ERROR_FLAGS);
			}

			bool has_note() const {
				return !!(flags & ~ERROR_FLAGS);
			}

			Glib::ustring type() const {
				uint8_t tag = decode_u8(&data[0]);
				for (std::size_t i = 0; i < G_N_ELEMENTS(packet_parsers); ++i) {
					const PacketParser *parser = packet_parsers[i];
					if (parser->tag == tag) {
						return parser->name;
					}
				}
				return Glib::ustring::compose("Unknown Tag 0x%1", tohex(tag, 2));
			}

			Glib::ustring raw() const {
				std::wostringstream oss;
				for (std::size_t i = 0; i < length; ++i) {
					if (i && !(i % 16)) {
						oss << L'\n';
					} else if (i) {
						oss << L' ';
					}
					oss << tohex(data[i], 2);
				}
				return Glib::ustring::format(oss.str());
			}

			Glib::RefPtr<Gtk::TreeModel> decode(const PacketDecodedTreeColumns &columns) const {
				Glib::RefPtr<Gtk::TreeStore> store(Gtk::TreeStore::create(columns));

				Gtk::TreeRow errors_row = columns.append_kv(store, "Errors");
				if (has_error()) {
					if (flags & PF_TRUNCATED) {
						columns.append_kv(store, errors_row, "Log file truncated");
					}
					if (flags & PF_BAD_CRC) {
						columns.append_kv(store, errors_row, "Packet CRC failed");
					}
					if (flags & PF_BAD_UTF8) {
						columns.append_kv(store, errors_row, "Illegal UTF-8 string");
					}
					if (flags & PF_BAD_DOUBLE) {
						columns.append_kv(store, errors_row, "Illegal floating-point encoding");
					}
					if (flags & PF_BAD_NETWORK_PACKET) {
						columns.append_kv(store, errors_row, "Malformed network packet");
					}
					if (flags & PF_BAD_PLAYTYPE) {
						columns.append_kv(store, errors_row, "Illegal play type");
					}
					if (flags & PF_BAD_PATTERN) {
						columns.append_kv(store, errors_row, "Illegal lid pattern index");
					}
					if (flags & PF_BAD_TIMESPEC) {
						columns.append_kv(store, errors_row, "Illegal timestamp encoding");
					}
				}

				Gtk::TreeRow notes_row = columns.append_kv(store, "Notes");
				if (has_note()) {
					if (flags & PF_UNKNOWN_TAG) {
						columns.append_kv(store, notes_row, "Unknown packet tag");
					}
					if (flags & PF_SHORT) {
						columns.append_kv(store, notes_row, "Packet shorter than expected");
					}
					if (flags & PF_LONG) {
						columns.append_kv(store, notes_row, "Packet longer than expected");
					}
					if (flags & PF_UNKNOWN_LOG_LEVEL) {
						columns.append_kv(store, notes_row, "Log level unknown");
					}
					if (flags & PF_UNKNOWN_MOVE_FLAG) {
						columns.append_kv(store, notes_row, "Movement flag unknown");
					}
					if (flags & PF_UNKNOWN_MOVE_TYPE) {
						columns.append_kv(store, notes_row, "Movement type unknown");
					}
					if (flags & PF_UNKNOWN_MOVE_PRIO) {
						columns.append_kv(store, notes_row, "Movement priority unknown");
					}
					if (flags & PF_UNKNOWN_END_REASON) {
						columns.append_kv(store, notes_row, "Termination reason unknown");
					}
					if (flags & PF_OLD_DOUBLE) {
						columns.append_kv(store, notes_row, "Old floating-point encoding");
					}
				}

				uint8_t tag = decode_u8(&data[0]);

				Gtk::TreeRow envelope_row = columns.append_kv(store, "Envelope");
				Gtk::TreeRow tag_row = columns.append_kv(store, envelope_row, "Tag", Glib::ustring::compose("0x%1", tohex(tag, 2)));
				Gtk::TreeRow declared_length_row = columns.append_kv(store, envelope_row, "Payload length (declared)", "<PACKET TRUNCATED>");
				Gtk::TreeRow crcs_row = columns.append_kv(store, envelope_row, "CRC", "<PACKET TRUNCATED>");

				Gtk::TreeRow payload_row = columns.append_kv(store, "Payload", "<PACKET TRUNCATED>");

				if (length >= 3) {
					const uint8_t *payload = &data[3];

					std::size_t payload_declared_length = decode_u16(&data[1]);
					declared_length_row[columns.value] = Glib::ustring::format(payload_declared_length);

					std::size_t payload_length = payload_declared_length;
					if (3 + payload_length > length) {
						payload_length = length - 3;
					}

					payload_row[columns.value] = "<TAG UNKNOWN>";
					for (std::size_t i = 0; i < G_N_ELEMENTS(packet_parsers); ++i) {
						const PacketParser *parser = packet_parsers[i];
						if (parser->tag == tag) {
							parser->build_tree(store, columns, payload_row, payload, payload_length, payload_declared_length);
						}
					}

					if (length == 3 + payload_declared_length + 2) {
						uint16_t stored = decode_u16(&data[3 + payload_length]);
						uint16_t computed = CRC16::calculate(data, 3 + payload_length);
						columns.append_kv(store, crcs_row, "Stored", Glib::ustring::compose("0x%1", tohex(stored, 4)));
						columns.append_kv(store, crcs_row, "Computed", Glib::ustring::compose("0x%1", tohex(computed, 4)));
						crcs_row[columns.value] = stored == computed ? "OK" : "Fail";
					}
				}
				return store;
			}
	};

	class PacketsALM : public Glib::Object, public AbstractListModel {
		public:
			typedef Glib::RefPtr<PacketsALM> Ptr;
			Gtk::TreeModelColumn<bool> error_column;
			Gtk::TreeModelColumn<bool> note_column;
			Gtk::TreeModelColumn<Glib::ustring> type_column;

			static Ptr create(const std::vector<PacketInfo> &packets) {
				Ptr p(new PacketsALM(packets));
				return p;
			}

		private:
			const std::vector<PacketInfo> &packets;

			PacketsALM(const std::vector<PacketInfo> &packets) : Glib::ObjectBase(typeid(PacketsALM)), Glib::Object(), AbstractListModel(), packets(packets) {
				alm_column_record.add(error_column);
				alm_column_record.add(note_column);
				alm_column_record.add(type_column);
			}

			~PacketsALM() {
			}

			std::size_t alm_rows() const {
				return packets.size();
			}

			void alm_get_value(std::size_t row, unsigned int col, Glib::ValueBase &value) const {
				if (col == static_cast<unsigned int>(error_column.index())) {
					Glib::Value<bool> v;
					v.init(error_column.type());
					v.set(packets[row].has_error());
					value.init(error_column.type());
					value = v;
				} else if (col == static_cast<unsigned int>(note_column.index())) {
					Glib::Value<bool> v;
					v.init(note_column.type());
					v.set(packets[row].has_note());
					value.init(note_column.type());
					value = v;
				} else if (col == static_cast<unsigned int>(type_column.index())) {
					Glib::Value<Glib::ustring> v;
					v.init(type_column.type());
					v.set(packets[row].type());
					value.init(type_column.type());
					value = v;
				}
			}

			void alm_set_value(std::size_t, unsigned int, const Glib::ValueBase &) {
			}

			friend class Glib::RefPtr<PacketsALM>;
	};
}

class LogAnalyzer::Impl : public NonCopyable {
	public:
		MappedFile mapping;
		std::vector<PacketInfo> packets;
		PacketsALM::Ptr alm;
		PacketDecodedTreeColumns packet_decoded_tree_columns;

		Impl(const std::string &filename) : mapping(make_filename(filename)) {
			const uint8_t *data = static_cast<const uint8_t *>(mapping.data());
			std::size_t size = mapping.size();
			while (size) {
				packets.push_back(PacketInfo(data, size));
				data += packets.back().length;
				size -= packets.back().length;
			}

			alm = PacketsALM::create(packets);
		}

		~Impl() {
		}
};

LogAnalyzer::LogAnalyzer(Gtk::Window &parent, const std::string &filename) : impl(new Impl(filename)), panes_fixed(false), packets_list_view(impl->alm) {
	set_title(Glib::ustring::compose("Thunderbots Log Tools - Analyzer - %1", Glib::filename_to_utf8(filename)));
	set_transient_for(parent);
	set_modal(false);
	set_size_request(400, 400);

	packets_list_view.append_column("Error?", impl->alm->error_column);
	packets_list_view.append_column("Note?", impl->alm->note_column);
	packets_list_view.append_column("Packet Type", impl->alm->type_column);
	packets_list_view.get_selection()->set_mode(Gtk::SELECTION_SINGLE);
	packets_list_view.get_selection()->signal_changed().connect(sigc::mem_fun(this, &LogAnalyzer::on_packets_list_view_selection_changed));
	Gtk::ScrolledWindow *scroller = Gtk::manage(new Gtk::ScrolledWindow);
	scroller->add(packets_list_view);
	Gtk::Frame *frame = Gtk::manage(new Gtk::Frame);
	frame->set_shadow_type(Gtk::SHADOW_IN);
	frame->add(*scroller);
	hpaned.pack1(*frame, Gtk::EXPAND | Gtk::FILL);

	packet_raw_entry.set_editable(false);
	scroller = Gtk::manage(new Gtk::ScrolledWindow);
	scroller->add(packet_raw_entry);
	frame = Gtk::manage(new Gtk::Frame);
	frame->set_shadow_type(Gtk::SHADOW_IN);
	frame->add(*scroller);
	vpaned.pack1(*frame, Gtk::EXPAND | Gtk::FILL);

	packet_decoded_tree.append_column("Field", impl->packet_decoded_tree_columns.key);
	packet_decoded_tree.get_column(0)->set_resizable();
	packet_decoded_tree.append_column("Value", impl->packet_decoded_tree_columns.value);
	packet_decoded_tree.get_column(1)->set_resizable();
	scroller = Gtk::manage(new Gtk::ScrolledWindow);
	scroller->add(packet_decoded_tree);
	frame = Gtk::manage(new Gtk::Frame);
	frame->set_shadow_type(Gtk::SHADOW_IN);
	frame->add(*scroller);
	vpaned.pack2(*frame, Gtk::EXPAND | Gtk::FILL);
	frame = Gtk::manage(new Gtk::Frame);
	frame->set_shadow_type(Gtk::SHADOW_IN);
	frame->add(vpaned);
	hpaned.pack2(*frame, Gtk::EXPAND | Gtk::SHRINK | Gtk::FILL);

	add(hpaned);

	show_all();
}

LogAnalyzer::~LogAnalyzer() {
}

void LogAnalyzer::on_size_allocate(Gtk::Allocation &alloc) {
	Gtk::Window::on_size_allocate(alloc);
	if (!panes_fixed) {
		hpaned.set_position(hpaned.get_width() / 2);
		vpaned.set_position(vpaned.get_height() / 2);
		panes_fixed = true;
	}
}

bool LogAnalyzer::on_delete_event(GdkEventAny *) {
	delete this;
	return true;
}

void LogAnalyzer::on_packets_list_view_selection_changed() {
	if (packets_list_view.get_selection()->count_selected_rows() > 0) {
		const PacketInfo &pkt = impl->packets[(*packets_list_view.get_selection()->get_selected_rows().begin())[0]];
		packet_raw_entry.get_buffer()->set_text(pkt.raw());
		packet_decoded_tree.set_model(pkt.decode(impl->packet_decoded_tree_columns));
	} else {
		packet_raw_entry.get_buffer()->erase(packet_raw_entry.get_buffer()->begin(), packet_raw_entry.get_buffer()->end());
		packet_decoded_tree.set_model(Glib::RefPtr<Gtk::TreeModel>());
	}
}

