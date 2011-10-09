#include "log/analyzer.h"
#include "log/loader.h"
#include "ai/flags.h"
#include "ai/common/playtype.h"
#include "ai/common/team.h"
#include "proto/log_record.pb.h"
#include "uicomponents/abstract_list_model.h"
#include "util/codec.h"
#include "util/noncopyable.h"
#include "util/string.h"
#include "util/time.h"
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <locale>
#include <string>
#include <vector>
#include <glibmm/object.h>
#include <glibmm/refptr.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/filefilter.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/radiobuttongroup.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/stock.h>
#include <gtkmm/treemodelcolumn.h>
#include <gtkmm/treestore.h>

using namespace std::placeholders;

namespace {
	/**
	 * \brief A mapping from transmitted byte to descriptive text of referee box command.
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
	 * \brief A mapping from movement flag to descriptive text.
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
	 * \brief Converts a Log::MonotonicTimeSpec to an actual timespec structure.
	 *
	 * \param[in] ts the time to convert.
	 *
	 * \return the converted time.
	 */
	timespec timespec_of_log(const Log::MonotonicTimeSpec &ts) {
		timespec res;
		res.tv_sec = ts.seconds();
		res.tv_nsec = ts.nanoseconds();
		return res;
	}

	void tsv_writer_ball(std::ostream &os, const std::vector<Log::Record> &records, Gtk::Window &) {
		for (auto i = records.begin(), iend = records.end(); i != iend; ++i) {
			const Log::Record &record = *i;
			if (record.has_tick()) {
				const Log::Tick &tick = record.tick();
				const Log::Tick::Ball &ball = tick.ball();
				os << "Tick";
				os << '\t' << timespec_to_string_machine(timespec_of_log(tick.start_time()));
				os << '\t' << (ball.position().x() / 1000000.0);
				os << '\t' << (ball.position().y() / 1000000.0);
				os << '\t' << (ball.velocity().x() / 1000000.0);
				os << '\t' << (ball.velocity().y() / 1000000.0);
				os << '\n';
			} else if (record.has_vision()) {
				const Log::Vision &vision = record.vision();
				const SSL_WrapperPacket &wrapper = vision.data();
				if (wrapper.has_detection()) {
					const SSL_DetectionFrame &frame = wrapper.detection();
					for (int j = 0; j < frame.balls_size(); ++j) {
						const SSL_DetectionBall &ball = frame.balls(j);
						os << "Vision";
						os << '\t' << timespec_to_string_machine(timespec_of_log(vision.timestamp()));
						os << '\t' << (ball.x() / 1000.0);
						os << '\t' << (ball.y() / 1000.0);
						os << "\tX";
						os << "\tX";
						os << '\n';
					}
				}
			}
		}
	}

	void tsv_writer_friendly_player(std::ostream &os, const std::vector<Log::Record> &records, Gtk::Window &parent) {
		unsigned int pattern;
		{
			Gtk::Dialog dlg("Save to TSV", parent, true);
			dlg.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
			dlg.set_default_response(Gtk::RESPONSE_OK);
			Gtk::HBox hb;
			Gtk::Label label("Player index:");
			hb.pack_start(label, Gtk::PACK_SHRINK);
			Gtk::SpinButton spin;
			spin.get_adjustment()->configure(0, 0, 15, 1, 5, 0);
			spin.set_digits(0);
			spin.set_numeric();
			hb.pack_start(spin, Gtk::PACK_EXPAND_WIDGET);
			hb.show_all();
			dlg.get_vbox()->pack_start(hb, Gtk::PACK_SHRINK);
			dlg.run();
			pattern = spin.get_value_as_int();
		}
		if (records.size() < 2 || !records[1].has_config()) {
			throw std::runtime_error("No config record where it ought to be.");
		}
		Log::Colour friendly_colour = records[1].config().friendly_colour();
		for (auto i = records.begin(), iend = records.end(); i != iend; ++i) {
			const Log::Record &record = *i;
			if (record.has_config()) {
				friendly_colour = record.config().friendly_colour();
			} else if (record.has_tick()) {
				const Log::Tick &tick = record.tick();
				for (int j = 0; j < tick.friendly_robots_size(); ++j) {
					const Log::Tick::FriendlyRobot &bot = tick.friendly_robots(j);
					if (bot.pattern() == pattern) {
						os << "Tick";
						os << '\t' << timespec_to_string_machine(timespec_of_log(tick.start_time()));
						os << '\t' << (bot.position().x() / 1000000.0);
						os << '\t' << (bot.position().y() / 1000000.0);
						os << '\t' << (bot.position().t() / 1000000.0);
						os << '\t' << (bot.velocity().x() / 1000000.0);
						os << '\t' << (bot.velocity().y() / 1000000.0);
						os << '\t' << (bot.velocity().t() / 1000000.0);
						os << '\t' << (bot.target().x() / 1000000.0);
						os << '\t' << (bot.target().y() / 1000000.0);
						os << '\t' << (bot.target().t() / 1000000.0);
						os << '\n';
					}
				}
			} else if (record.has_vision()) {
				const Log::Vision &vision = record.vision();
				const SSL_WrapperPacket &wrapper = vision.data();
				if (wrapper.has_detection()) {
					const SSL_DetectionFrame &frame = wrapper.detection();
					int count;
					const SSL_DetectionRobot & (SSL_DetectionFrame::*fp)(int) const;
					if (friendly_colour == Log::COLOUR_YELLOW) {
						count = frame.robots_yellow_size();
						fp = &SSL_DetectionFrame::robots_yellow;
					} else {
						count = frame.robots_blue_size();
						fp = &SSL_DetectionFrame::robots_blue;
					}
					for (int j = 0; j < count; ++j) {
						const SSL_DetectionRobot &bot = (frame.*fp)(j);
						if (bot.has_robot_id() && bot.has_orientation() && bot.robot_id() == pattern) {
							os << "Vision";
							os << '\t' << timespec_to_string_machine(timespec_of_log(vision.timestamp()));
							os << '\t' << (bot.x() / 1000.0);
							os << '\t' << (bot.y() / 1000.0);
							os << '\t' << (bot.orientation() / 1000.0);
							os << "\tX";
							os << "\tX";
							os << "\tX";
							os << "\tX";
							os << "\tX";
							os << "\tX";
							os << '\n';
						}
					}
				}
			}
		}
	}

	void tsv_writer_stamps(std::ostream &os, const std::vector<Log::Record> &records, Gtk::Window &) {
		for (auto i = records.begin(), iend = records.end(); i != iend; ++i) {
			const Log::Record &record = *i;
			if (record.has_tick()) {
				os << "Tick\t" << timespec_to_string_machine(timespec_of_log(record.tick().start_time())) << '\n';
			} else if (record.has_vision()) {
				os << "Vision\t" << timespec_to_string_machine(timespec_of_log(record.vision().timestamp())) << '\n';
			} else if (record.has_refbox()) {
				os << "Refbox\t" << timespec_to_string_machine(timespec_of_log(record.refbox().timestamp())) << '\n';
			}
		}
	}

	void tsv_writer_compute_times(std::ostream &os, const std::vector<Log::Record> &records, Gtk::Window &) {
		for (auto i = records.begin(), iend = records.end(); i != iend; ++i) {
			const Log::Record &record = *i;
			if (record.has_tick()) {
				os << record.tick().compute_time() << '\n';
			}
		}
	}

	/**
	 * \brief The different types of TSV writers available.
	 */
	const struct {
		std::function<void(std::ostream &, const std::vector<Log::Record> &, Gtk::Window &)> fn;
		const char *description;
	} TSV_WRITERS[] = {
		{ &tsv_writer_ball, "Ball position/velocity (predicted and vision)" },
		{ &tsv_writer_friendly_player, "Friendly player position/velocity (predicted and vision)" },
		{ &tsv_writer_stamps, "Tick/vision/refbox timestamps" },
		{ &tsv_writer_compute_times, "Tick computation times" },
	};

	/**
	 * \brief Returns a string describing the type of record.
	 *
	 * \param[in] record the record to analyze.
	 *
	 * \return the descriptive string.
	 */
	Glib::ustring get_record_type_string(const Log::Record &record) {
		const google::protobuf::Descriptor &desc = *record.GetDescriptor();
		const google::protobuf::Reflection &refl = *record.GetReflection();

		for (int i = 0; i < desc.field_count(); ++i) {
			const google::protobuf::FieldDescriptor &fd = *desc.field(i);
			if (fd.is_optional() && refl.HasField(record, &fd)) {
				return fd.name();
			} else if (fd.is_repeated() && refl.FieldSize(record, &fd)) {
				return fd.name();
			}
		}

		throw std::runtime_error("Record has no data.");
	}

	/**
	 * \brief A column record containing two columns, a key and a value, suitable for displaying decoded packet structures.
	 */
	class PacketDecodedTreeColumns : public Gtk::TreeModelColumnRecord {
		public:
			/**
			 * \brief The key column.
			 */
			Gtk::TreeModelColumn<Glib::ustring> key;

			/**
			 * \brief The value column.
			 */
			Gtk::TreeModelColumn<Glib::ustring> value;

			/**
			 * \brief Constructs a new PacketDecodedTreeColumns.
			 */
			PacketDecodedTreeColumns();

			/**
			 * \brief Adds a key-value pair to a tree.
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
			 * \brief Adds a key-value pair to a tree.
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

	class RecordsALM : public Glib::Object, public AbstractListModel {
		public:
			typedef Glib::RefPtr<RecordsALM> Ptr;
			Gtk::TreeModelColumn<std::size_t> index_column;
			Gtk::TreeModelColumn<Glib::ustring> type_column;

			static Ptr create(const std::vector<Log::Record> &packets) {
				Ptr p(new RecordsALM(packets));
				return p;
			}

		private:
			const std::vector<Log::Record> &records;

			RecordsALM(const std::vector<Log::Record> &records) : Glib::ObjectBase(typeid(RecordsALM)), Glib::Object(), AbstractListModel(), records(records) {
				alm_column_record.add(index_column);
				alm_column_record.add(type_column);
			}

			std::size_t alm_rows() const {
				return records.size();
			}

			void alm_get_value(std::size_t row, unsigned int col, Glib::ValueBase &value) const {
				if (col == static_cast<unsigned int>(index_column.index())) {
					Glib::Value<std::size_t> v;
					v.init(index_column.type());
					v.set(row);
					value.init(index_column.type());
					value = v;
				} else if (col == static_cast<unsigned int>(type_column.index())) {
					Glib::Value<Glib::ustring> v;
					v.init(type_column.type());
					v.set(get_record_type_string(records[row]));
					value.init(type_column.type());
					value = v;
				}
			}

			void alm_set_value(std::size_t, unsigned int, const Glib::ValueBase &) {
			}

			friend class Glib::RefPtr<RecordsALM>;
	};

	Glib::ustring build_decoded_tree_format_string(const std::string &data, google::protobuf::FieldDescriptor::Type type) {
		switch (type) {
			case google::protobuf::FieldDescriptor::TYPE_STRING:
				return data;

			case google::protobuf::FieldDescriptor::TYPE_BYTES:
			{
				Glib::ustring ustr;
				for (auto i = data.begin(), iend = data.end(); i != iend; ++i) {
					if (!ustr.empty()) {
						ustr.append(" ");
					}
					ustr.append(ustr.format(std::hex, std::setw(2), std::setfill(L'0'), std::uppercase, static_cast<unsigned int>(static_cast<uint8_t>(*i))));
				}
				return ustr;
			}

			case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
			case google::protobuf::FieldDescriptor::TYPE_FLOAT:
			case google::protobuf::FieldDescriptor::TYPE_INT64:
			case google::protobuf::FieldDescriptor::TYPE_UINT64:
			case google::protobuf::FieldDescriptor::TYPE_INT32:
			case google::protobuf::FieldDescriptor::TYPE_FIXED64:
			case google::protobuf::FieldDescriptor::TYPE_FIXED32:
			case google::protobuf::FieldDescriptor::TYPE_BOOL:
			case google::protobuf::FieldDescriptor::TYPE_GROUP:
			case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
			case google::protobuf::FieldDescriptor::TYPE_UINT32:
			case google::protobuf::FieldDescriptor::TYPE_ENUM:
			case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
			case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
			case google::protobuf::FieldDescriptor::TYPE_SINT32:
			case google::protobuf::FieldDescriptor::TYPE_SINT64:
				std::abort();
		}
		std::abort();
	}

	void build_decoded_tree_nontoplevel(const google::protobuf::Message &message, Glib::RefPtr<Gtk::TreeStore> ts, PacketDecodedTreeColumns &cols, Gtk::TreeRow parent);

	void build_decoded_tree_field_singular(const google::protobuf::Message &message, const google::protobuf::FieldDescriptor &fd, Glib::RefPtr<Gtk::TreeStore> ts, PacketDecodedTreeColumns &cols, Gtk::TreeRow &row) {
		const google::protobuf::Reflection &refl = *message.GetReflection();

		if (dynamic_cast<const Log::UNIXTimeSpec *>(&message) && fd.name() == "seconds") {
			assert(fd.type() == google::protobuf::FieldDescriptor::TYPE_INT64);
			row[cols.key] = "date/time";
			char buffer[4096];
			std::time_t t = static_cast<std::time_t>(refl.GetInt64(message, &fd));
			std::strftime(buffer, sizeof(buffer), "%c", std::localtime(&t));
			row[cols.value] = buffer;
			return;
		} else if (dynamic_cast<const Log::Tick::FriendlyRobot *>(&message) && fd.name() == "movement_flags") {
			assert(fd.type() == google::protobuf::FieldDescriptor::TYPE_FIXED64);
			uint64_t flags = refl.GetUInt64(message, &fd);
			row[cols.value] = tohex(flags, 16);
			for (std::size_t i = 0; i < G_N_ELEMENTS(MOVE_FLAG_MAPPING); ++i) {
				if (flags & MOVE_FLAG_MAPPING[i].mask) {
					flags &= ~MOVE_FLAG_MAPPING[i].mask;
					cols.append_kv(ts, row, MOVE_FLAG_MAPPING[i].description, tohex(MOVE_FLAG_MAPPING[i].mask, 16));
				}
			}
			if (flags) {
				cols.append_kv(ts, row, "Unknown Flags", tohex(flags, 16));
			}
			return;
		} else if (dynamic_cast<const Log::Refbox *>(&message) && fd.name() == "data") {
			assert(fd.type() == google::protobuf::FieldDescriptor::TYPE_BYTES);
			const std::string &data = refl.GetString(message, &fd);
			if (data.size() != 6) {
				throw std::runtime_error("Bad refbox packet size.");
			}
			bool found = false;
			for (std::size_t i = 0; i < G_N_ELEMENTS(REFBOX_MAPPING); ++i) {
				if (REFBOX_MAPPING[i].command == data[0]) {
					cols.append_kv(ts, row, "Command", Glib::ustring::compose("%1 (%2)", REFBOX_MAPPING[i].description, data[0]));
					found = true;
				}
			}
			if (!found) {
				cols.append_kv(ts, row, "Command", Glib::ustring::compose("Unknown (%1)", data[0]));
			}
			cols.append_kv(ts, row, "Counter", Glib::ustring::format(static_cast<unsigned int>(static_cast<uint8_t>(data[1]))));
			cols.append_kv(ts, row, "Goals Blue", Glib::ustring::format(static_cast<unsigned int>(static_cast<uint8_t>(data[2]))));
			cols.append_kv(ts, row, "Goals Yellow", Glib::ustring::format(static_cast<unsigned int>(static_cast<uint8_t>(data[3]))));
			cols.append_kv(ts, row, "Seconds Left", Glib::ustring::format(decode_u16(&data.data()[4])));
		}

		switch (fd.cpp_type()) {
			case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
				row[cols.value] = Glib::ustring::format(refl.GetInt32(message, &fd));
				return;

			case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
				row[cols.value] = Glib::ustring::format(refl.GetInt64(message, &fd));
				return;

			case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
				row[cols.value] = Glib::ustring::format(refl.GetUInt32(message, &fd));
				return;

			case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
				row[cols.value] = Glib::ustring::format(refl.GetUInt64(message, &fd));
				return;

			case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
				row[cols.value] = Glib::ustring::format(refl.GetDouble(message, &fd));
				return;

			case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
				row[cols.value] = Glib::ustring::format(refl.GetFloat(message, &fd));
				return;

			case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
				row[cols.value] = refl.GetBool(message, &fd) ? "True" : "False";
				return;

			case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
				row[cols.value] = refl.GetEnum(message, &fd)->name();
				return;

			case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
				row[cols.value] = build_decoded_tree_format_string(refl.GetString(message, &fd), fd.type());
				return;

			case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
				build_decoded_tree_nontoplevel(refl.GetMessage(message, &fd), ts, cols, row);
				return;
		}
		std::abort();
	}

	void build_decoded_tree_field_repeated(const google::protobuf::Message &message, const google::protobuf::FieldDescriptor &fd, int index, Glib::RefPtr<Gtk::TreeStore> ts, PacketDecodedTreeColumns &cols, Gtk::TreeRow &row) {
		const google::protobuf::Reflection &refl = *message.GetReflection();
		switch (fd.cpp_type()) {
			case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
				row[cols.value] = Glib::ustring::format(refl.GetRepeatedInt32(message, &fd, index));
				return;

			case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
				row[cols.value] = Glib::ustring::format(refl.GetRepeatedInt64(message, &fd, index));
				return;

			case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
				row[cols.value] = Glib::ustring::format(refl.GetRepeatedUInt32(message, &fd, index));
				return;

			case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
				row[cols.value] = Glib::ustring::format(refl.GetRepeatedUInt64(message, &fd, index));
				return;

			case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
				row[cols.value] = Glib::ustring::format(refl.GetRepeatedDouble(message, &fd, index));
				return;

			case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
				row[cols.value] = Glib::ustring::format(refl.GetRepeatedFloat(message, &fd, index));
				return;

			case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
				row[cols.value] = refl.GetRepeatedBool(message, &fd, index) ? "true" : "false";
				return;

			case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
				row[cols.value] = refl.GetRepeatedEnum(message, &fd, index)->name();
				return;

			case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
				row[cols.value] = build_decoded_tree_format_string(refl.GetRepeatedString(message, &fd, index), fd.type());
				return;

			case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
				build_decoded_tree_nontoplevel(refl.GetRepeatedMessage(message, &fd, index), ts, cols, row);
				return;
		}
		std::abort();
	}

	void build_decoded_tree_message(const google::protobuf::Message &message, Glib::RefPtr<Gtk::TreeStore> ts, PacketDecodedTreeColumns &cols, std::function<Gtk::TreeRow(const Glib::ustring &, const Glib::ustring &)> append_kv) {
		const google::protobuf::Descriptor &desc = *message.GetDescriptor();
		const google::protobuf::Reflection &refl = *message.GetReflection();

		if (dynamic_cast<const Log::Parameter *>(&message)) {
			const Log::Parameter &param = dynamic_cast<const Log::Parameter &>(message);
			if (param.has_bool_value()) {
				append_kv(param.name(), param.bool_value() ? "true" : "false");
			} else if (param.has_int_value()) {
				append_kv(param.name(), Glib::ustring::format(param.int_value()));
			} else if (param.has_double_value()) {
				append_kv(param.name(), Glib::ustring::format(param.double_value()));
			} else if (param.has_radian_value()) {
				append_kv(param.name(), Glib::ustring::compose("%1r", param.radian_value()));
			} else if (param.has_degree_value()) {
				append_kv(param.name(), Glib::ustring::compose("%1°", param.degree_value()));
			} else {
				std::abort();
			}
			return;
		}

		for (int i = 0; i < desc.field_count(); ++i) {
			const google::protobuf::FieldDescriptor &fd = *desc.field(i);
			bool ok = false;
			Gtk::TreeRow row = append_kv(fd.name(), Glib::ustring());
			switch (fd.label()) {
				case google::protobuf::FieldDescriptor::LABEL_OPTIONAL:
					ok = true;
					if (refl.HasField(message, &fd)) {
						build_decoded_tree_field_singular(message, fd, ts, cols, row);
					} else {
						row[cols.value] = "<No Data>";
					}
					break;

				case google::protobuf::FieldDescriptor::LABEL_REQUIRED:
					ok = true;
					build_decoded_tree_field_singular(message, fd, ts, cols, row);
					break;

				case google::protobuf::FieldDescriptor::LABEL_REPEATED:
					ok = true;
					row[cols.value] = Glib::ustring::compose("%1 instances", refl.FieldSize(message, &fd));
					for (int i = 0; i < refl.FieldSize(message, &fd); ++i) {
						Gtk::TreeRow child = cols.append_kv(ts, row, Glib::ustring::compose("Element %1", i));
						build_decoded_tree_field_repeated(message, fd, i, ts, cols, child);
					}
					break;
			}
			assert(ok);
		}
	}

	void build_decoded_tree_toplevel(const google::protobuf::Message &message, Glib::RefPtr<Gtk::TreeStore> ts, PacketDecodedTreeColumns &cols) {
		Gtk::TreeRow (PacketDecodedTreeColumns::*fp)(Glib::RefPtr<Gtk::TreeStore>, const Glib::ustring &, const Glib::ustring &) const = &PacketDecodedTreeColumns::append_kv;
		build_decoded_tree_message(message, ts, cols, std::bind(std::mem_fn(fp), std::cref(cols), ts, _1, _2));
	}

	void build_decoded_tree_nontoplevel(const google::protobuf::Message &message, Glib::RefPtr<Gtk::TreeStore> ts, PacketDecodedTreeColumns &cols, Gtk::TreeRow parent) {
		if (dynamic_cast<const Log::MonotonicTimeSpec *>(&message)) {
			const Log::MonotonicTimeSpec &mts = dynamic_cast<const Log::MonotonicTimeSpec &>(message);
			timespec ts;
			ts.tv_sec = mts.seconds();
			ts.tv_nsec = mts.nanoseconds();
			parent[cols.value] = timespec_to_string(ts);
			return;
		} else if (dynamic_cast<const Log::Vector2 *>(&message)) {
			const Log::Vector2 &v = dynamic_cast<const Log::Vector2 &>(message);
			parent[cols.value] = Glib::ustring::compose("(%1, %2)", v.x() / 1000000.0, v.y() / 1000000.0);
			return;
		} else if (dynamic_cast<const Log::Vector3 *>(&message)) {
			const Log::Vector3 &v = dynamic_cast<const Log::Vector3 &>(message);
			parent[cols.value] = Glib::ustring::compose("(%1, %2, %3)", v.x() / 1000000.0, v.y() / 1000000.0, v.t() / 1000000.0);
			return;
		}

		Gtk::TreeRow (PacketDecodedTreeColumns::*fp)(Glib::RefPtr<Gtk::TreeStore>, const Gtk::TreeRow &, const Glib::ustring &, const Glib::ustring &) const = &PacketDecodedTreeColumns::append_kv;
		build_decoded_tree_message(message, ts, cols, std::bind(std::mem_fn(fp), std::cref(cols), ts, parent, _1, _2));
	}

	void build_decoded_tree(const Log::Record &record, Glib::RefPtr<Gtk::TreeStore> ts, PacketDecodedTreeColumns &cols) {
		// We don't want the Record message itself to be shown in the tree as it's only a wrapper and would obfuscate the output.
		// Instead, go through its fields and find the one that's present.
		const google::protobuf::Descriptor &desc = *record.GetDescriptor();
		const google::protobuf::Reflection &refl = *record.GetReflection();
		for (int i = 0; i < desc.field_count(); ++i) {
			const google::protobuf::FieldDescriptor &fd = *desc.field(i);
			if (fd.is_optional() && fd.type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE && refl.HasField(record, &fd)) {
				// This record contains an optional field.
				// The type of the optional field itself is displayed in the list of records.
				// Therefore, there is no need to show a tree node for said field.
				// Instead, the contents of the field's message should become the top-level nodes in the tree.
				build_decoded_tree_toplevel(refl.GetMessage(record, &fd), ts, cols);
				return;
			} else if (fd.is_repeated() && fd.type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE && refl.FieldSize(record, &fd)) {
				// This record contains a repeated field.
				// Show a tree node for each instance.
				for (int i = 0; i < refl.FieldSize(record, &fd); ++i) {
					Gtk::TreeRow row = cols.append_kv(ts, Glib::ustring::compose("Element %1", i));
					build_decoded_tree_nontoplevel(refl.GetRepeatedMessage(record, &fd, i), ts, cols, row);
				}
				return;
			}
		}

		// The record contained something we didn't fully understand.
		std::abort();
	}
}

class LogAnalyzer::Impl : public NonCopyable {
	public:
		std::vector<Log::Record> records;
		RecordsALM::Ptr alm;
		PacketDecodedTreeColumns packet_decoded_tree_columns;

		Impl(const std::string &pathname) : records(LogLoader::load(pathname)) {
			alm = RecordsALM::create(records);
		}
};

LogAnalyzer::LogAnalyzer(Gtk::Window &parent, const std::string &pathname) : impl(new Impl(pathname)), pane_fixed(false), packets_list_view(impl->alm), to_tsv_button("To TSV") {
	set_title(Glib::ustring::compose("Thunderbots Log Tools - Analyzer - %1", Glib::filename_display_basename(pathname)));
	set_transient_for(parent);
	set_modal(false);
	set_size_request(400, 400);

	packets_list_view.append_column("Index", impl->alm->index_column);
	packets_list_view.append_column("Packet Type", impl->alm->type_column);
	packets_list_view.get_selection()->set_mode(Gtk::SELECTION_SINGLE);
	packets_list_view.get_selection()->signal_changed().connect(sigc::mem_fun(this, &LogAnalyzer::on_packets_list_view_selection_changed));
	packets_list_scroller.add(packets_list_view);
	packets_list_frame.set_shadow_type(Gtk::SHADOW_IN);
	packets_list_frame.add(packets_list_scroller);
	hpaned.pack1(packets_list_frame, Gtk::EXPAND | Gtk::FILL);

	packet_decoded_tree.append_column("Field", impl->packet_decoded_tree_columns.key);
	packet_decoded_tree.get_column(0)->set_resizable();
	packet_decoded_tree.append_column("Value", impl->packet_decoded_tree_columns.value);
	packet_decoded_tree.get_column(1)->set_resizable();
	packet_decoded_scroller.add(packet_decoded_tree);
	packet_decoded_frame.set_shadow_type(Gtk::SHADOW_IN);
	packet_decoded_frame.add(packet_decoded_scroller);
	hpaned.pack2(packet_decoded_frame, Gtk::EXPAND | Gtk::FILL);

	to_tsv_button.signal_clicked().connect(sigc::mem_fun(this, &LogAnalyzer::on_to_tsv_clicked));

	vbox.pack_start(hpaned, Gtk::PACK_EXPAND_WIDGET);
	vbox.pack_start(to_tsv_button, Gtk::PACK_SHRINK);

	add(vbox);

	show_all();
}

void LogAnalyzer::on_size_allocate(Gtk::Allocation &alloc) {
	Gtk::Window::on_size_allocate(alloc);
	if (!pane_fixed) {
		hpaned.set_position(hpaned.get_width() / 2);
		pane_fixed = true;
	}
}

bool LogAnalyzer::on_delete_event(GdkEventAny *) {
	delete this;
	return true;
}

void LogAnalyzer::on_packets_list_view_selection_changed() {
	if (packets_list_view.get_selection()->count_selected_rows() > 0) {
		const Log::Record &record = impl->records[(*packets_list_view.get_selection()->get_selected_rows().begin())[0]];
		Glib::RefPtr<Gtk::TreeStore> ts = Gtk::TreeStore::create(impl->packet_decoded_tree_columns);
		build_decoded_tree(record, ts, impl->packet_decoded_tree_columns);
		packet_decoded_tree.set_model(ts);
	} else {
		packet_decoded_tree.set_model(Glib::RefPtr<Gtk::TreeModel>());
	}
}

void LogAnalyzer::on_to_tsv_clicked() {
	Gtk::Dialog dlg("Save to TSV", *this, true);
	dlg.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
	dlg.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dlg.set_default_response(Gtk::RESPONSE_OK);
	Gtk::RadioButtonGroup group;
	Gtk::RadioButton radio_buttons[G_N_ELEMENTS(TSV_WRITERS)];
	for (std::size_t i = 0; i < G_N_ELEMENTS(TSV_WRITERS); ++i) {
		radio_buttons[i].set_label(TSV_WRITERS[i].description);
		radio_buttons[i].set_group(group);
		dlg.get_vbox()->pack_start(radio_buttons[i]);
		radio_buttons[i].show();
	}
	if (dlg.run() == Gtk::RESPONSE_OK) {
		Gtk::FileChooserDialog fc(*this, "Save to TSV", Gtk::FILE_CHOOSER_ACTION_SAVE);
		fc.set_local_only();
		fc.set_select_multiple(false);
		fc.set_do_overwrite_confirmation();
		Gtk::FileFilter ff;
		ff.set_name("TSV Files");
		ff.add_pattern("*.tsv");
		fc.add_filter(ff);
		fc.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);
		fc.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
		if (fc.run() == Gtk::RESPONSE_OK) {
			const std::string &filename = fc.get_filename();
			std::ofstream ofs;
			ofs.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
			ofs.open(filename.c_str(), std::ios::out | std::ios::trunc);
			ofs.imbue(std::locale("C"));
			for (std::size_t i = 0; i < G_N_ELEMENTS(TSV_WRITERS); ++i) {
				if (radio_buttons[i].get_active()) {
					TSV_WRITERS[i].fn(ofs, impl->records, *this);
				}
			}
		}
	}
}

