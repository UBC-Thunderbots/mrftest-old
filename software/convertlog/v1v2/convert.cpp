#include "convertlog/v1v2/convert.h"
#include "convertlog/v1v2/tags.h"
#include "log/shared/enums.h"
#include "log/shared/magic.h"
#include "proto/log_record.pb.h"
#include "util/codec.h"
#include "util/crc16.h"
#include "util/dprint.h"
#include "util/exception.h"
#include "util/fd.h"
#include "util/mapped_file.h"
#include "util/noncopyable.h"
#include "util/string.h"
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>
#include <unistd.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace {
	class ScopedFileDeleter : public NonCopyable {
		public:
			ScopedFileDeleter(const std::string &filename) : filename(filename) {
			}

			~ScopedFileDeleter() {
				remove(filename.c_str());
			}

		private:
			std::string filename;
	};

	class NoAITicksException : public std::runtime_error {
		public:
			NoAITicksException() : std::runtime_error("No AI Ticks") {
			}
	};

	bool looks_like_v1(const MappedFile &f) {
		if (f.size() < 5) {
			return false;
		}
		const uint8_t *d = static_cast<const uint8_t *>(f.data());
		return d[0] == 0x00 && d[1] == 0x00 && d[2] == 0x00 && d[3] == 0x39 && d[4] == 0x33;
	}

	bool check_packets(const MappedFile &f) {
		const uint8_t *ptr = static_cast<const uint8_t *>(f.data());
		std::size_t left = f.size();
		while (left) {
			if (left < 5) {
				std::cout << "Log file truncated (EOF in middle of packet).\n";
				return false;
			}
			if (ptr[0] > ConvertLogV1V2::T_FRIENDLY_COLOUR) {
				std::cout << "Log file contains unrecognized tag.\n";
				return false;
			}
			uint16_t payload_length = decode_u16(ptr + 1);
			if (1U + payload_length + 2U > left) {
				std::cout << "Log file truncated (EOF in middle of packet).\n";
				return false;
			}
			uint16_t crc_stored = decode_u16(ptr + 1 + 2 + payload_length);
			uint16_t crc_computed = CRC16::calculate(ptr, 1 + 2 + payload_length);
			if (crc_stored != crc_computed) {
				std::cout << "Log file contains packet with bad CRC (stored " << tohex(crc_stored, 4) << ", computed " << tohex(crc_computed, 4) << ").\n";
				return false;
			}
			if (ptr[0] == ConvertLogV1V2::T_END) {
				return true;
			}
			ptr += 1 + 2 + payload_length + 2;
			left -= 1 + 2 + payload_length + 2;
		}
		return true;
	}

	void write_record(const Log::Record &record, google::protobuf::io::ZeroCopyOutputStream &dest) {
		assert(record.IsInitialized());
		google::protobuf::io::CodedOutputStream cos(&dest);
		cos.WriteVarint32(static_cast<uint32_t>(record.ByteSize()));
		record.SerializeWithCachedSizes(&cos);
		if (cos.HadError()) {
			throw std::runtime_error("Failed to serialize log record.");
		}
	}

	void convert(const MappedFile &src, google::protobuf::io::ZeroCopyOutputStream &dest) {
		// Write the magic string header.
		{
			google::protobuf::io::CodedOutputStream cos(&dest);
			cos.WriteString(Log::MAGIC);
		}

		// Accumulate at all times the most recent monotonic timestamp.
		timespec most_recent_monotonic;

		// Before we can start our normal iteration, we need to find the first AI tick and grab its UNIX timestamp.
		{
			const uint8_t *sptr = static_cast<const uint8_t *>(src.data());
			std::size_t sleft = src.size();
			bool found = false;
			while (!found && sleft) {
				if (sptr[0] == ConvertLogV1V2::T_AI_TICK) {
					int64_t seconds = static_cast<int64_t>(decode_u64(&sptr[3]));
					int32_t nanoseconds = static_cast<int32_t>(decode_u32(&sptr[11]));
					Log::Record record;
					Log::UNIXTimeSpec *ts = record.mutable_startup_time();
					ts->set_seconds(seconds);
					ts->set_nanoseconds(nanoseconds);
					write_record(record, dest);
					seconds = static_cast<int64_t>(decode_u64(&sptr[15]));
					nanoseconds = static_cast<int32_t>(decode_u32(&sptr[23]));
					most_recent_monotonic.tv_sec = seconds;
					most_recent_monotonic.tv_nsec = nanoseconds;
					found = true;
				}
				std::size_t packet_length = 1 + 2 + decode_u16(&sptr[1]) + 2;
				sptr += packet_length;
				sleft -= packet_length;
			}
			if (!found) {
				throw NoAITicksException();
			}
		}

		// Begin going through the source data.
		const uint8_t *sptr = static_cast<const uint8_t *>(src.data());
		std::size_t sleft = src.size();

		// Old logs keep play types on change, whereas the new log keeps it at each tick.
		// We need to remember the current play type as we walk.
		AI::Common::PlayType play_type = AI::Common::PlayType::HALT;

		// Configuration records will be built up incrementally and accumulate data.
		// Keep the accumulated configuration.
		Log::Record config_record;
		Log::Config &config = *config_record.mutable_config();
		config.set_nominal_ticks_per_second(15);

		// We see a Start of Log packet first (this has already been checked).
		// This will be followed by a somewhat arbitrary interleaving of:
		// - Parameter change records.
		// - Configuration-related record (component selection and colour choice).
		// - Play type change records.
		// - Score records.
		// As long as only the above appear, we collect them all into a single Config record and a single Parameter list record.
		// When a packet not in the above list appears, this marks the start of the log proper so we start outputting record in the usual sequential way.
		{
			Log::Record boot_parameters_record;
			while (sleft) {
				ConvertLogV1V2::Tag tag = static_cast<ConvertLogV1V2::Tag>(sptr[0]);
				std::size_t payload_length = decode_u16(&sptr[1]);
				const uint8_t *payload = sptr + 1 + 2;
				bool end_of_boot = false;
				switch (tag) {
					case ConvertLogV1V2::T_START:
						// This packet has no payload and doesn't need to do anything.
						break;

					case ConvertLogV1V2::T_BOOL_PARAM:
					{
						// Add this parameter value to the parameters record.
						Log::Parameter &param = *boot_parameters_record.add_parameters();
						param.set_name(std::string(payload + 1, payload + payload_length));
						param.set_bool_value(!!payload[0]);
						break;
					}

					case ConvertLogV1V2::T_INT_PARAM:
					{
						// Add this parameter value to the parameters record.
						Log::Parameter &param = *boot_parameters_record.add_parameters();
						param.set_name(std::string(payload + 8, payload + payload_length));
						param.set_int_value(static_cast<int64_t>(decode_u64(payload)));
						break;
					}

					case ConvertLogV1V2::T_DOUBLE_PARAM_OLD:
					{
						// Add this parameter value to the parameters record.
						Log::Parameter &param = *boot_parameters_record.add_parameters();
						param.set_name(std::string(payload + 20, payload + payload_length));
						std::istringstream iss(std::string(payload, payload + 20));
						iss.imbue(std::locale("C"));
						double v;
						iss >> v;
						param.set_double_value(v);
						break;
					}

					case ConvertLogV1V2::T_BALL_FILTER:
						// Add this information to the configuration record.
						config.set_ball_filter(std::string(payload, payload + payload_length));
						break;

					case ConvertLogV1V2::T_COACH:
						// Add this information to the configuration record.
						config.set_coach(std::string(payload, payload + payload_length));
						break;

					case ConvertLogV1V2::T_STRATEGY:
						// Add this information to the configuration record.
						config.set_strategy(std::string(payload, payload + payload_length));
						break;

					case ConvertLogV1V2::T_ROBOT_CONTROLLER:
						// Add this information to the configuration record.
						config.set_robot_controller(std::string(payload, payload + payload_length));
						break;

					case ConvertLogV1V2::T_PLAYTYPE:
						// Record the new play type.
						play_type = AI::Common::PlayTypeInfo::of_int(payload[0]);
						break;

					case ConvertLogV1V2::T_SCORES:
						// The old XBee backend had a bug which generated bad T_SCORES packets.
						// Ignore them and reconstruct from refbox packets if present.
						break;

					case ConvertLogV1V2::T_BACKEND:
						// Add this information to the configuration record.
						config.set_backend(std::string(payload, payload + payload_length));
						break;

					case ConvertLogV1V2::T_DOUBLE_PARAM:
					{
						// Add this parameter value to the parameters record.
						Log::Parameter &param = *boot_parameters_record.add_parameters();
						param.set_name(std::string(payload + 8, payload + payload_length));
						param.set_double_value(decode_double(payload));
						break;
					}

					case ConvertLogV1V2::T_HIGH_LEVEL:
						// Add this information to the configuration record.
						config.set_high_level(std::string(payload, payload + payload_length));
						break;

					case ConvertLogV1V2::T_FRIENDLY_COLOUR:
						// Add this information to the configuration record.
						config.set_friendly_colour(payload[0] == 0 ? Log::COLOUR_YELLOW : Log::COLOUR_BLUE);
						break;

					case ConvertLogV1V2::T_END:
					case ConvertLogV1V2::T_DEBUG:
					case ConvertLogV1V2::T_ANNUNCIATOR:
					case ConvertLogV1V2::T_VISION:
					case ConvertLogV1V2::T_REFBOX:
					case ConvertLogV1V2::T_FIELD:
					case ConvertLogV1V2::T_FRIENDLY_ROBOT:
					case ConvertLogV1V2::T_PATH_ELEMENT:
					case ConvertLogV1V2::T_ENEMY_ROBOT:
					case ConvertLogV1V2::T_BALL:
					case ConvertLogV1V2::T_AI_TICK:
						end_of_boot = true;
						break;
				}
				if (end_of_boot) {
					break;
				} else {
					sptr += 1 + 2 + payload_length + 2;
					sleft -= 1 + 2 + payload_length + 2;
				}
			}
			write_record(config_record, dest);
			write_record(boot_parameters_record, dest);
			Log::Record boot_scores_record;
			boot_scores_record.mutable_scores()->set_friendly(0);
			boot_scores_record.mutable_scores()->set_enemy(0);
			write_record(boot_scores_record, dest);
		}

		// A workaround is needed for a bug in the simulator backend where it sometimes never wrote out a field dimensions packet.
		{
			bool found = false;
			const uint8_t *sptr2 = sptr;
			std::size_t sleft2 = sleft;
			while (!found && sleft2) {
				if (sptr2[0] == ConvertLogV1V2::T_FIELD) {
					found = true;
				}
				std::size_t packet_length = 1 + 2 + decode_u16(&sptr2[1]) + 2;
				sptr2 += packet_length;
				sleft2 -= packet_length;
			}
			if (!found) {
				Log::Record record;
				Log::Field &field = *record.mutable_field();
				field.set_length(6050000);
				field.set_total_length(7400000);
				field.set_width(4050000);
				field.set_total_width(5400000);
				field.set_goal_width(700000);
				field.set_centre_circle_radius(500000);
				field.set_defense_area_radius(500000);
				field.set_defense_area_stretch(350000);
				write_record(record, dest);
			}
		}

		// Handle the rest of the records.
		Log::Record tick_record;
		bool ended = false;
		while (!ended && sleft) {
			ConvertLogV1V2::Tag tag = static_cast<ConvertLogV1V2::Tag>(sptr[0]);
			std::size_t payload_length = decode_u16(&sptr[1]);
			const uint8_t *payload = sptr + 1 + 2;
			switch (tag) {
				case ConvertLogV1V2::T_START:
					// A second one of these should never appear.
					throw std::runtime_error("Unexpected duplicate T_START.");

				case ConvertLogV1V2::T_END:
				{
					// Decode the reason for termination and turn it into a proper record.
					Log::Record record;
					Log::Shutdown &shutdown = *record.mutable_shutdown();
					ConvertLogV1V2::EndMajorReason reason = static_cast<ConvertLogV1V2::EndMajorReason>(payload[0]);
					bool found_reason = false;
					switch (reason) {
						case ConvertLogV1V2::ER_NORMAL:
							shutdown.mutable_normal();
							found_reason = true;
							break;

						case ConvertLogV1V2::ER_SIGNAL:
							shutdown.mutable_signal()->set_signal(payload[1]);
							found_reason = true;
							break;

						case ConvertLogV1V2::ER_EXCEPTION:
							shutdown.mutable_exception()->set_message(reinterpret_cast<const char *>(&payload[1]), payload_length - 1);
							found_reason = true;
							break;
					}
					if (!found_reason) {
						throw std::runtime_error("T_END with unknown major reason code.");
					}
					write_record(record, dest);
					ended = true;
					break;
				}

				case ConvertLogV1V2::T_DEBUG:
				{
					// Copy over the packet.
					Log::Record record;
					switch (payload[0]) {
						case 0:
							record.mutable_debug_message()->set_level(Log::DEBUG_MESSAGE_LEVEL_DEBUG);
							break;

						case 1:
							record.mutable_debug_message()->set_level(Log::DEBUG_MESSAGE_LEVEL_INFO);
							break;

						case 2:
							record.mutable_debug_message()->set_level(Log::DEBUG_MESSAGE_LEVEL_WARN);
							break;

						case 3:
							record.mutable_debug_message()->set_level(Log::DEBUG_MESSAGE_LEVEL_ERROR);
							break;

						default:
							throw std::runtime_error("Unrecognized log level.");
					}
					record.mutable_debug_message()->set_text(reinterpret_cast<const char *>(&payload[1]), payload_length - 1);
					write_record(record, dest);
					break;
				}

				case ConvertLogV1V2::T_ANNUNCIATOR:
				{
					// Copy over the packet.
					Log::Record record;
					record.mutable_annunciator_message()->set_action(payload[0] ? Log::ANNUNCIATOR_ACTION_ASSERT : Log::ANNUNCIATOR_ACTION_DEASSERT);
					record.mutable_annunciator_message()->set_text(reinterpret_cast<const char *>(&payload[1]), payload_length - 1);
					write_record(record, dest);
					break;
				}

				case ConvertLogV1V2::T_BOOL_PARAM:
				{
					// Copy over the packet.
					Log::Record record;
					Log::Parameter &param = *record.add_parameters();
					param.set_name(reinterpret_cast<const char *>(&payload[1]), payload_length - 1);
					param.set_bool_value(!!payload[0]);
					write_record(record, dest);
					break;
				}

				case ConvertLogV1V2::T_INT_PARAM:
				{
					// Copy over the packet.
					Log::Record record;
					Log::Parameter &param = *record.add_parameters();
					param.set_name(reinterpret_cast<const char *>(&payload[8]), payload_length - 8);
					param.set_int_value(static_cast<int64_t>(decode_u64(&payload[0])));
					write_record(record, dest);
					break;
				}

				case ConvertLogV1V2::T_DOUBLE_PARAM_OLD:
				{
					// Copy over the packet.
					Log::Record record;
					Log::Parameter &param = *record.add_parameters();
					param.set_name(std::string(payload + 8, payload + payload_length));
					std::istringstream iss(std::string(payload, payload + 20));
					iss.imbue(std::locale("C"));
					double v;
					iss >> v;
					param.set_double_value(v);
					write_record(record, dest);
					break;
				}

				case ConvertLogV1V2::T_VISION:
				{
					// Decode and then copy the packet.
					SSL_WrapperPacket wrapper;
					wrapper.ParseFromArray(payload, static_cast<int>(payload_length));
					Log::Record record;
					record.mutable_vision()->mutable_timestamp()->set_seconds(most_recent_monotonic.tv_sec);
					record.mutable_vision()->mutable_timestamp()->set_nanoseconds(static_cast<int32_t>(most_recent_monotonic.tv_nsec));
					record.mutable_vision()->mutable_data()->CopyFrom(wrapper);
					write_record(record, dest);
					break;
				}

				case ConvertLogV1V2::T_REFBOX:
				{
					// Copy the packet.
					Log::Record record;
					record.mutable_refbox()->mutable_timestamp()->set_seconds(most_recent_monotonic.tv_sec);
					record.mutable_refbox()->mutable_timestamp()->set_nanoseconds(static_cast<int32_t>(most_recent_monotonic.tv_nsec));
					record.mutable_refbox()->set_data(payload, payload_length);
					write_record(record, dest);

					// The old XBee backend had a bug which generated bad T_SCORES packets.
					// Generate a proper scores packet from here instead.
					record.Clear();
					switch (config.friendly_colour()) {
						case Log::COLOUR_YELLOW:
							record.mutable_scores()->set_friendly(payload[3]);
							record.mutable_scores()->set_enemy(payload[2]);
							break;

						case Log::COLOUR_BLUE:
							record.mutable_scores()->set_friendly(payload[2]);
							record.mutable_scores()->set_enemy(payload[3]);
							break;
					}
					write_record(record, dest);
					break;
				}

				case ConvertLogV1V2::T_FIELD:
				{
					// Copy the packet.
					Log::Record record;
					Log::Field &field = *record.mutable_field();
					field.set_length(decode_u32(&payload[0]));
					field.set_total_length(decode_u32(&payload[4]));
					field.set_width(decode_u32(&payload[8]));
					field.set_total_width(decode_u32(&payload[12]));
					field.set_goal_width(decode_u32(&payload[16]));
					field.set_centre_circle_radius(decode_u32(&payload[20]));
					field.set_defense_area_radius(decode_u32(&payload[24]));
					field.set_defense_area_stretch(decode_u32(&payload[28]));
					write_record(record, dest);
					break;
				}

				case ConvertLogV1V2::T_BALL_FILTER:
					// Modify and rewrite the configuration record.
					config.set_ball_filter(reinterpret_cast<const char *>(payload), payload_length);
					write_record(config_record, dest);
					break;

				case ConvertLogV1V2::T_COACH:
					// Modify and rewrite the configuration record.
					config.set_coach(reinterpret_cast<const char *>(payload), payload_length);
					write_record(config_record, dest);
					break;

				case ConvertLogV1V2::T_STRATEGY:
					// Modify and rewrite the configuration record.
					config.set_strategy(reinterpret_cast<const char *>(payload), payload_length);
					write_record(config_record, dest);
					break;

				case ConvertLogV1V2::T_ROBOT_CONTROLLER:
					// Modify and rewrite the configuration record.
					config.set_robot_controller(reinterpret_cast<const char *>(payload), payload_length);
					write_record(config_record, dest);
					break;

				case ConvertLogV1V2::T_PLAYTYPE:
					// Record the new play type.
					play_type = AI::Common::PlayTypeInfo::of_int(payload[0]);
					break;

				case ConvertLogV1V2::T_SCORES:
					// The old XBee backend had a bug which generated bad T_SCORES packets.
					// Ignore them and reconstruct from refbox packets if present.
					break;

				case ConvertLogV1V2::T_FRIENDLY_ROBOT:
				{
					// Add to the current tick record.
					Log::Tick::FriendlyRobot &bot = *tick_record.mutable_tick()->add_friendly_robots();
					bot.set_pattern(payload[0]);
					bot.mutable_position()->set_x(static_cast<int32_t>(decode_u32(&payload[1])));
					bot.mutable_position()->set_y(static_cast<int32_t>(decode_u32(&payload[5])));
					bot.mutable_position()->set_t(static_cast<int32_t>(decode_u32(&payload[9])));
					bot.mutable_velocity()->set_x(static_cast<int32_t>(decode_u32(&payload[13])));
					bot.mutable_velocity()->set_y(static_cast<int32_t>(decode_u32(&payload[17])));
					bot.mutable_velocity()->set_t(static_cast<int32_t>(decode_u32(&payload[21])));
					bot.mutable_target()->set_x(static_cast<int32_t>(decode_u32(&payload[37])));
					bot.mutable_target()->set_y(static_cast<int32_t>(decode_u32(&payload[41])));
					bot.mutable_target()->set_t(static_cast<int32_t>(decode_u32(&payload[45])));
					uint64_t movement_flags = decode_u64(&payload[49]);
					// Workaround bug where two flags were present both meaning FLAG_AVOID_ENEMY_DEFENSE.
					if (movement_flags & 0x100) {
						movement_flags |= 0x10;
						movement_flags &= static_cast<uint64_t>(~0x100);
					}
					bot.set_movement_flags(movement_flags);
					bot.set_movement_type(Log::Util::MoveType::to_protobuf(static_cast<AI::Flags::MoveType>(payload[57])));
					bot.set_movement_priority(Log::Util::MovePrio::to_protobuf(static_cast<AI::Flags::MovePrio>(payload[58])));
					for (unsigned int i = 0; i < 4; ++i) {
						bot.add_wheel_setpoints(static_cast<int16_t>(decode_u16(&payload[59 + i * 2])));
					}
					break;
				}

				case ConvertLogV1V2::T_PATH_ELEMENT:
				{
					// Add the path element to the FriendlyRobot message.
					Log::Tick::FriendlyRobot *bot = 0;
					for (int i = 0; !bot && i < tick_record.mutable_tick()->friendly_robots_size(); ++i) {
						if (tick_record.tick().friendly_robots(i).pattern() == payload[0]) {
							bot = tick_record.mutable_tick()->mutable_friendly_robots(i);
						}
					}
					if (!bot) {
						throw std::runtime_error("T_PATH_ELEMENT without corresponding T_FRIENDLY_ROBOT.");
					}
					Log::Tick::FriendlyRobot::PathElement &elt = *bot->add_path();
					elt.mutable_point()->set_x(static_cast<int32_t>(decode_u32(&payload[1])));
					elt.mutable_point()->set_y(static_cast<int32_t>(decode_u32(&payload[5])));
					elt.mutable_point()->set_t(static_cast<int32_t>(decode_u32(&payload[9])));
					elt.mutable_timestamp()->set_seconds(static_cast<int64_t>(decode_u64(&payload[13])));
					elt.mutable_timestamp()->set_nanoseconds(static_cast<int32_t>(decode_u32(&payload[21])));
					break;
				}

				case ConvertLogV1V2::T_ENEMY_ROBOT:
				{
					// Add to the current tick record.
					Log::Tick::EnemyRobot &bot = *tick_record.mutable_tick()->add_enemy_robots();
					bot.set_pattern(payload[0]);
					bot.mutable_position()->set_x(static_cast<int32_t>(decode_u32(&payload[1])));
					bot.mutable_position()->set_y(static_cast<int32_t>(decode_u32(&payload[5])));
					bot.mutable_position()->set_t(static_cast<int32_t>(decode_u32(&payload[9])));
					bot.mutable_velocity()->set_x(static_cast<int32_t>(decode_u32(&payload[13])));
					bot.mutable_velocity()->set_y(static_cast<int32_t>(decode_u32(&payload[17])));
					bot.mutable_velocity()->set_t(static_cast<int32_t>(decode_u32(&payload[21])));
					break;
				}

				case ConvertLogV1V2::T_BALL:
				{
					// Add to the current tick record.
					Log::Tick::Ball &ball = *tick_record.mutable_tick()->mutable_ball();
					ball.mutable_position()->set_x(static_cast<int32_t>(decode_u32(&payload[0])));
					ball.mutable_position()->set_y(static_cast<int32_t>(decode_u32(&payload[4])));
					ball.mutable_velocity()->set_x(static_cast<int32_t>(decode_u32(&payload[8])));
					ball.mutable_velocity()->set_y(static_cast<int32_t>(decode_u32(&payload[12])));
					break;
				}

				case ConvertLogV1V2::T_AI_TICK:
				{
					// Finish and write out the tick record, then clear it in preparation for the next tick.
					Log::Tick &tick = *tick_record.mutable_tick();
					tick.set_play_type(Log::Util::PlayType::to_protobuf(play_type));
					most_recent_monotonic.tv_sec = static_cast<int64_t>(decode_u64(&payload[12]));
					most_recent_monotonic.tv_nsec = static_cast<int32_t>(decode_u32(&payload[20]));
					tick.mutable_start_time()->set_seconds(most_recent_monotonic.tv_sec);
					tick.mutable_start_time()->set_nanoseconds(static_cast<int32_t>(most_recent_monotonic.tv_nsec));
					tick.set_compute_time(0);
					write_record(tick_record, dest);
					tick_record.Clear();
					break;
				}

				case ConvertLogV1V2::T_BACKEND:
					// This should never appear a second time.
					throw std::runtime_error("Unexpected T_BACKEND.");

				case ConvertLogV1V2::T_DOUBLE_PARAM:
				{
					// Copy over the packet.
					Log::Record record;
					Log::Parameter &param = *record.add_parameters();
					param.set_name(reinterpret_cast<const char *>(&payload[8]), payload_length - 8);
					param.set_double_value(decode_double(&payload[0]));
					write_record(record, dest);
					break;
				}

				case ConvertLogV1V2::T_HIGH_LEVEL:
					// Modify and rewrite the configuration record.
					config.set_high_level(reinterpret_cast<const char *>(payload), payload_length);
					write_record(config_record, dest);
					break;

				case ConvertLogV1V2::T_FRIENDLY_COLOUR:
					// Modify and rewrite the configuration record.
					config.set_friendly_colour(payload[0] == 0 ? Log::COLOUR_YELLOW : Log::COLOUR_BLUE);
					write_record(config_record, dest);
					break;
			}
			sptr += 1 + 2 + payload_length + 2;
			sleft -= 1 + 2 + payload_length + 2;
		}
	}
}

void ConvertLogV1V2::run() {
	const std::string &parent_dir = Glib::get_user_data_dir();
	const std::string &tbots_dir = Glib::build_filename(parent_dir, "thunderbots");
	const std::string &logs_dir = Glib::build_filename(tbots_dir, "logs");
	const std::string &temp_file = Glib::build_filename(logs_dir, ".convertlog.v1v2.tmp");
	FileDescriptor lock_fd = FileDescriptor::create_open(Glib::build_filename(logs_dir, ".lock").c_str(), O_RDWR | O_CREAT, 0666);
	if (lockf(lock_fd.fd(), F_TLOCK, 0) < 0) {
		if (errno == EAGAIN || errno == EACCES) {
			std::cout << "Directory locked; try again later.\n";
			return;
		} else {
			throw SystemError("lockf", errno);
		}
	}
	std::cout << "Scanning for version 1 logs and converting to version 2.\n";
	std::remove(temp_file.c_str());
	Glib::Dir dir(logs_dir);
	std::vector<std::string> logs(dir.begin(), dir.end());
	bool any_converted = false;
	for (auto i = logs.begin(), iend = logs.end(); i != iend; ++i) {
		const std::string &real_file = Glib::build_filename(logs_dir, *i);
		MappedFile real_file_mapped(real_file);
		if (looks_like_v1(real_file_mapped)) {
			std::cout << *i << ": ";
			std::cout.flush();
			if (check_packets(real_file_mapped)) {
				try {
					ScopedFileDeleter sfd(temp_file);
					{
						FileDescriptor fd = FileDescriptor::create_open(temp_file.c_str(), O_WRONLY | O_CREAT, 0666);
						{
							google::protobuf::io::FileOutputStream fos(fd.fd());
							convert(real_file_mapped, fos);
						}
						if (fdatasync(fd.fd()) < 0) {
							throw SystemError("fdatasync", errno);
						}
					}
					if (std::rename(temp_file.c_str(), real_file.c_str()) < 0) {
						throw SystemError("rename", errno);
					}
					std::cout << "OK\n";
					any_converted = true;
				} catch (const NoAITicksException &) {
					std::cout << "No data\n";
					std::remove(real_file.c_str());
				}
			}
		}
	}

	if (any_converted) {
		std::cout << "\nLog files are uncompressed initially (both after conversion and when written out by the AI). You may want to run bin/log to compress them and save some disk space (do that now and also every now and then after some AI runs).\n";
	}
}

