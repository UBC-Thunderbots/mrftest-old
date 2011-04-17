#include "ai/logger.h"
#include "log/shared/tags.h"
#include "util/algorithm.h"
#include "util/annunciator.h"
#include "util/codec.h"
#include "util/crc16.h"
#include "util/dprint.h"
#include "util/exception.h"
#include "util/param.h"
#include "util/time.h"
#include <cerrno>
#include <csignal>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <glibmm.h>
#include <limits>
#include <locale>
#include <sstream>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>

namespace {
	FileDescriptor::Ptr create_file() {
		const std::string &parent_dir = Glib::get_user_data_dir();
		const std::string &tbots_dir = Glib::build_filename(parent_dir, "thunderbots");
		mkdir(tbots_dir.c_str(), 0777);
		const std::string &logs_dir = Glib::build_filename(tbots_dir, "logs");
		mkdir(logs_dir.c_str(), 0777);
		time_t tim;
		if (!time(&tim)) {
			throw SystemError("time", errno);
		}
		struct tm tm;
		if (!localtime_r(&tim, &tm)) {
			throw SystemError("localtime_r", errno);
		}
		std::ostringstream buffer;
		static const char PATTERN[] = "%Y-%m-%d %H:%M:%S";
		std::use_facet<std::time_put<char> >(std::locale()).put(buffer, buffer, L' ', &tm, PATTERN, PATTERN + std::strlen(PATTERN));
		const std::string &filename = Glib::build_filename(logs_dir, buffer.str());
		return FileDescriptor::create_open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
	}

	void do_write(FileDescriptor::Ptr fd, const void *data, std::size_t length) {
		while (length) {
			ssize_t rc = write(fd->fd(), data, length);
			if (rc < 0) {
				throw SystemError("write", errno);
			}
			data = static_cast<const char *>(data) + rc;
			length -= rc;
		}
	}

	void writev_packet(FileDescriptor::Ptr fd, Log::Tag tag, iovec *payload, std::size_t payload_count) {
		std::size_t payload_size = 0;
		for (std::size_t i = 0; i < payload_count; ++i) {
			payload_size += payload[i].iov_len;
		}

		if (payload_size > 65535) {
			throw std::length_error("AI::Logger: payload_size");
		}

		uint8_t header[3];
		header[0] = static_cast<uint8_t>(tag);
		encode_u16(&header[1], static_cast<uint16_t>(payload_size));

		uint8_t footer[2];
		uint16_t crc = CRC16::calculate(header, sizeof(header));
		for (std::size_t i = 0; i < payload_count; ++i) {
			crc = CRC16::calculate(payload[i].iov_base, payload[i].iov_len, crc);
		}
		encode_u16(&footer[0], crc);

		iovec iov[payload_count + 2];
		iov[0].iov_base = header;
		iov[0].iov_len = sizeof(header);
		std::copy(payload, payload + payload_count, &iov[1]);
		iov[payload_count + 1].iov_base = footer;
		iov[payload_count + 1].iov_len = sizeof(footer);

		ssize_t rc = writev(fd->fd(), iov, static_cast<int>(G_N_ELEMENTS(iov)));
		if (rc < 0) {
			throw SystemError("writev", errno);
		} else if (rc != static_cast<ssize_t>(sizeof(header) + payload_size + sizeof(footer))) {
			throw SystemError("writev", EIO);
		}
	}

	void write_packet(FileDescriptor::Ptr fd, Log::Tag tag, const void *payload, std::size_t payload_size) {
		iovec iov;
		iov.iov_base = const_cast<void *>(payload);
		iov.iov_len = payload_size;
		writev_packet(fd, tag, &iov, 1);
	}

	uint32_t encode_micros(double in) {
		const double min = std::numeric_limits<int32_t>::min();
		const double max = std::numeric_limits<int32_t>::max();
		return static_cast<uint32_t>(static_cast<int32_t>(clamp(in * 1000000.0, min, max)));
	}

	AI::Logger *instance = 0;
}


void ai_logger_signal_handler_thunk(int sig) {
	instance->signal_handler(sig);
	raise(sig);
}

AI::Logger::Logger(const AI::AIPackage &ai) : ai(ai), fd(create_file()), ended(false), sigstack_registration(sigstack, sizeof(sigstack)), SIGHUP_registration(SIGHUP, &ai_logger_signal_handler_thunk, SA_RESETHAND), SIGINT_registration(SIGINT, &ai_logger_signal_handler_thunk, SA_RESETHAND), SIGQUIT_registration(SIGQUIT, &ai_logger_signal_handler_thunk, SA_RESETHAND), SIGILL_registration(SIGILL, &ai_logger_signal_handler_thunk, SA_RESETHAND), SIGTRAP_registration(SIGTRAP, &ai_logger_signal_handler_thunk, SA_RESETHAND), SIGABRT_registration(SIGABRT, &ai_logger_signal_handler_thunk, SA_RESETHAND), SIGBUS_registration(SIGBUS, &ai_logger_signal_handler_thunk, SA_RESETHAND), SIGFPE_registration(SIGFPE, &ai_logger_signal_handler_thunk, SA_RESETHAND), SIGSEGV_registration(SIGSEGV, &ai_logger_signal_handler_thunk, SA_RESETHAND), SIGPIPE_registration(SIGPIPE, &ai_logger_signal_handler_thunk, SA_RESETHAND), SIGTERM_registration(SIGTERM, &ai_logger_signal_handler_thunk, SA_RESETHAND), SIGSTKFLT_registration(SIGSTKFLT, &ai_logger_signal_handler_thunk, SA_RESETHAND) {
	write_packet(fd, Log::T_START, 0, 0);

	const std::string &backend_name = ai.backend.factory().name();
	write_packet(fd, Log::T_BACKEND, backend_name.data(), backend_name.size());

	signal_message_logged.connect(sigc::mem_fun(this, &AI::Logger::on_message_logged));

	Annunciator::signal_message_activated.connect(sigc::mem_fun(this, &AI::Logger::on_annunciator_message_activated));
	Annunciator::signal_message_deactivated.connect(sigc::mem_fun(this, &AI::Logger::on_annunciator_message_deactivated));
	Annunciator::signal_message_reactivated.connect(sigc::mem_fun(this, &AI::Logger::on_annunciator_message_reactivated));

	attach_param_change_handler(ParamTreeNode::root());

	ai.backend.signal_vision().connect(sigc::mem_fun(this, &AI::Logger::on_vision_packet));
	ai.backend.signal_refbox().connect(sigc::mem_fun(this, &AI::Logger::on_refbox_packet));
	ai.backend.field().signal_changed.connect(sigc::mem_fun(this, &AI::Logger::on_field_changed));
	ai.backend.friendly_colour().signal_changed().connect(sigc::mem_fun(this, &AI::Logger::on_friendly_colour_changed));
	on_friendly_colour_changed();
	ai.backend.ball_filter().signal_changed().connect(sigc::mem_fun(this, &AI::Logger::on_ball_filter_changed));
	on_ball_filter_changed();
	ai.robot_controller_factory.signal_changed().connect(sigc::mem_fun(this, &AI::Logger::on_robot_controller_factory_changed));
	on_robot_controller_factory_changed();
	ai.backend.playtype().signal_changed().connect(sigc::mem_fun(this, &AI::Logger::on_playtype_changed));
	on_playtype_changed();
	ai.backend.signal_score_changed().connect(sigc::mem_fun(this, &AI::Logger::on_score_changed));
	on_score_changed();
	ai.backend.signal_post_tick().connect(sigc::mem_fun(this, &AI::Logger::on_tick));
	ai.high_level.signal_changed().connect(sigc::mem_fun(this, &AI::Logger::on_high_level_changed));
	on_high_level_changed();

	instance = this;
}

AI::Logger::~Logger() {
	instance = 0;
	if (!ended) {
		try {
			uint8_t payload = Log::ER_NORMAL;
			write_packet(fd, Log::T_END, &payload, 1);
		} catch (...) {
			// Swallow; failing to write the T_END record is not the worst of our problems.
		}
	}
}

void AI::Logger::end_with_exception(const Glib::ustring &msg) {
	uint8_t reason = Log::ER_EXCEPTION;

	const std::string &utf8msg = msg;

	iovec iov[2];
	iov[0].iov_base = &reason;
	iov[0].iov_len = sizeof(reason);
	iov[1].iov_base = const_cast<char *>(utf8msg.data());
	iov[1].iov_len = utf8msg.size();

	writev_packet(fd, Log::T_END, iov, sizeof(iov) / sizeof(*iov));

	ended = true;
}

void AI::Logger::end_with_exception(const char *msg) {
	end_with_exception(Glib::locale_to_utf8(msg));
}

void AI::Logger::attach_param_change_handler(ParamTreeNode *node) {
	BoolParam *bp = dynamic_cast<BoolParam *>(node);
	IntParam *ip = dynamic_cast<IntParam *>(node);
	DoubleParam *dp = dynamic_cast<DoubleParam *>(node);

	if (bp) {
		bp->signal_changed().connect(sigc::bind(sigc::mem_fun(this, &AI::Logger::on_bool_param_changed), bp));
		on_bool_param_changed(bp);
	} else if (ip) {
		ip->signal_changed().connect(sigc::bind(sigc::mem_fun(this, &AI::Logger::on_int_param_changed), ip));
		on_int_param_changed(ip);
	} else if (dp) {
		dp->signal_changed().connect(sigc::bind(sigc::mem_fun(this, &AI::Logger::on_double_param_changed), dp));
		on_double_param_changed(dp);
	}

	for (std::size_t i = 0; i < node->num_children(); ++i) {
		attach_param_change_handler(node->child(i));
	}
}

void AI::Logger::signal_handler(int sig) {
	uint8_t payload[2];
	payload[0] = Log::ER_SIGNAL;
	payload[1] = static_cast<uint8_t>(sig);

	write_packet(fd, Log::T_END, payload, sizeof(payload));

	ended = true;
}

void AI::Logger::on_message_logged(unsigned int level, const Glib::ustring &msg) {
	uint8_t level8 = static_cast<uint8_t>(level);

	const std::string &utf8msg = msg;

	iovec iov[2];
	iov[0].iov_base = &level8;
	iov[0].iov_len = sizeof(level8);
	iov[1].iov_base = const_cast<char *>(utf8msg.data());
	iov[1].iov_len = utf8msg.size();

	writev_packet(fd, Log::T_DEBUG, iov, sizeof(iov) / sizeof(*iov));
}

void AI::Logger::log_annunciator(std::size_t i, bool activated) {
	uint8_t activated8 = activated ? UINT8_C(0xFF) : 0;

	const std::string &utf8msg = Annunciator::visible()[i]->text;

	iovec iov[2];
	iov[0].iov_base = &activated8;
	iov[0].iov_len = sizeof(activated8);
	iov[1].iov_base = const_cast<char *>(utf8msg.data());
	iov[1].iov_len = utf8msg.size();

	writev_packet(fd, Log::T_ANNUNCIATOR, iov, sizeof(iov) / sizeof(*iov));
}

void AI::Logger::on_annunciator_message_activated() {
	log_annunciator(Annunciator::visible().size() - 1, true);
}

void AI::Logger::on_annunciator_message_deactivated(std::size_t i) {
	log_annunciator(i, false);
}

void AI::Logger::on_annunciator_message_reactivated(std::size_t i) {
	log_annunciator(i, true);
}

void AI::Logger::on_bool_param_changed(BoolParam *p) {
	uint8_t value = (*p) ? UINT8_C(0xFF) : 0;

	const std::string &utf8name = p->path();

	iovec iov[2];
	iov[0].iov_base = &value;
	iov[0].iov_len = sizeof(value);
	iov[1].iov_base = const_cast<char *>(utf8name.data());
	iov[1].iov_len = utf8name.size();

	writev_packet(fd, Log::T_BOOL_PARAM, iov, sizeof(iov) / sizeof(*iov));
}

void AI::Logger::on_int_param_changed(IntParam *p) {
	uint8_t value[8];
	encode_u64(value, static_cast<uint64_t>(static_cast<int64_t>(*p)));

	const std::string &utf8name = p->path();

	iovec iov[2];
	iov[0].iov_base = &value;
	iov[0].iov_len = sizeof(value);
	iov[1].iov_base = const_cast<char *>(utf8name.data());
	iov[1].iov_len = utf8name.size();

	writev_packet(fd, Log::T_INT_PARAM, iov, sizeof(iov) / sizeof(*iov));
}

void AI::Logger::on_double_param_changed(DoubleParam *p) {
	uint8_t value[8];
	encode_double(value, *p);

	const std::string &utf8name = p->path();

	iovec iov[2];
	iov[0].iov_base = &value;
	iov[0].iov_len = sizeof(value);
	iov[1].iov_base = const_cast<char *>(utf8name.data());
	iov[1].iov_len = utf8name.size();

	writev_packet(fd, Log::T_DOUBLE_PARAM, iov, sizeof(iov) / sizeof(*iov));
}

void AI::Logger::on_vision_packet(const void *vision_packet, std::size_t vision_length) {
	write_packet(fd, Log::T_VISION, vision_packet, vision_length);
}

void AI::Logger::on_refbox_packet(const void *refbox_packet, std::size_t refbox_length) {
	write_packet(fd, Log::T_REFBOX, refbox_packet, refbox_length);
}

void AI::Logger::on_field_changed() {
	const AI::Common::Field &field = static_cast<const AI::HL::W::Field &>(ai.backend.field());
	uint8_t packet[32];
	encode_u32(&packet[0], encode_micros(field.length()));
	encode_u32(&packet[4], encode_micros(field.total_length()));
	encode_u32(&packet[8], encode_micros(field.width()));
	encode_u32(&packet[12], encode_micros(field.total_width()));
	encode_u32(&packet[16], encode_micros(field.goal_width()));
	encode_u32(&packet[20], encode_micros(field.centre_circle_radius()));
	encode_u32(&packet[24], encode_micros(field.defense_area_radius()));
	encode_u32(&packet[28], encode_micros(field.defense_area_stretch()));
	write_packet(fd, Log::T_FIELD, packet, sizeof(packet));
}

void AI::Logger::on_friendly_colour_changed() {
	AI::Common::Team::Colour clr = ai.backend.friendly_colour();
	uint8_t colour = static_cast<uint8_t>(clr);
	write_packet(fd, Log::T_FRIENDLY_COLOUR, &colour, sizeof(colour));
}

void AI::Logger::on_ball_filter_changed() {
	AI::BF::BallFilter *ball_filter = ai.backend.ball_filter();
	if (ball_filter) {
		const std::string &utf8name = ball_filter->name();
		write_packet(fd, Log::T_BALL_FILTER, utf8name.data(), utf8name.size());
	} else {
		write_packet(fd, Log::T_BALL_FILTER, 0, 0);
	}
}

void AI::Logger::on_robot_controller_factory_changed() {
	if (ai.robot_controller_factory) {
		const std::string &utf8name = ai.robot_controller_factory->name();
		write_packet(fd, Log::T_ROBOT_CONTROLLER, utf8name.data(), utf8name.size());
	} else {
		write_packet(fd, Log::T_ROBOT_CONTROLLER, 0, 0);
	}
}

void AI::Logger::on_playtype_changed() {
	AI::Common::PlayType pt = ai.backend.playtype();
	uint8_t pt8 = static_cast<uint8_t>(pt);
	write_packet(fd, Log::T_PLAYTYPE, &pt8, sizeof(pt8));
}

void AI::Logger::on_score_changed() {
	uint8_t buffer[2];
	buffer[0] = static_cast<uint8_t>(ai.backend.friendly_team().score());
	buffer[1] = static_cast<uint8_t>(ai.backend.enemy_team().score());
	write_packet(fd, Log::T_SCORES, buffer, sizeof(buffer));
}

void AI::Logger::on_tick() {
	for (std::size_t i = 0; i < ai.backend.friendly_team().size(); ++i) {
		AI::BE::Player::Ptr p = ai.backend.friendly_team().get(i);
		{
			uint8_t payload[67];
			encode_u8(&payload[0], static_cast<uint8_t>(p->pattern()));
			encode_u32(&payload[1], encode_micros(p->position(0.0).x));
			encode_u32(&payload[5], encode_micros(p->position(0.0).y));
			encode_u32(&payload[9], encode_micros(p->orientation(0.0)));
			encode_u32(&payload[13], encode_micros(p->velocity(0.0).x));
			encode_u32(&payload[17], encode_micros(p->velocity(0.0).y));
			encode_u32(&payload[21], encode_micros(p->avelocity(0.0)));
			encode_u32(&payload[25], encode_micros(p->acceleration(0.0).x));
			encode_u32(&payload[29], encode_micros(p->acceleration(0.0).y));
			encode_u32(&payload[33], encode_micros(p->aacceleration(0.0)));
			encode_u32(&payload[37], encode_micros(p->destination().first.x));
			encode_u32(&payload[41], encode_micros(p->destination().first.y));
			encode_u32(&payload[45], encode_micros(p->destination().second));
			encode_u64(&payload[49], p->flags());
			encode_u8(&payload[57], static_cast<uint8_t>(p->type()));
			encode_u8(&payload[58], static_cast<uint8_t>(p->prio()));
			const int(&wheel_speeds)[4] = p->wheel_speeds();
			encode_u16(&payload[59], static_cast<int16_t>(wheel_speeds[0]));
			encode_u16(&payload[61], static_cast<int16_t>(wheel_speeds[1]));
			encode_u16(&payload[63], static_cast<int16_t>(wheel_speeds[2]));
			encode_u16(&payload[65], static_cast<int16_t>(wheel_speeds[3]));
			write_packet(fd, Log::T_FRIENDLY_ROBOT, payload, sizeof(payload));
		}
		const std::vector<std::pair<std::pair<Point, double>, timespec> > &path = AI::RC::W::Player::Ptr::cast_static(p)->path();
		for (std::vector<std::pair<std::pair<Point, double>, timespec> >::const_iterator i = path.begin(), iend = path.end(); i != iend; ++i) {
			uint8_t payload[25];
			encode_u8(&payload[0], static_cast<uint8_t>(p->pattern()));
			encode_u32(&payload[1], encode_micros(i->first.first.x));
			encode_u32(&payload[5], encode_micros(i->first.first.y));
			encode_u32(&payload[9], encode_micros(i->first.second));
			encode_u64(&payload[13], i->second.tv_sec);
			encode_u32(&payload[21], static_cast<uint32_t>(i->second.tv_nsec));
			write_packet(fd, Log::T_PATH_ELEMENT, payload, sizeof(payload));
		}
	}
	for (std::size_t i = 0; i < ai.backend.enemy_team().size(); ++i) {
		AI::BE::Robot::Ptr p = ai.backend.enemy_team().get(i);
		uint8_t payload[37];
		encode_u8(&payload[0], static_cast<uint8_t>(p->pattern()));
		encode_u32(&payload[1], encode_micros(p->position(0.0).x));
		encode_u32(&payload[5], encode_micros(p->position(0.0).y));
		encode_u32(&payload[9], encode_micros(p->orientation(0.0)));
		encode_u32(&payload[13], encode_micros(p->velocity(0.0).x));
		encode_u32(&payload[17], encode_micros(p->velocity(0.0).y));
		encode_u32(&payload[21], encode_micros(p->avelocity(0.0)));
		encode_u32(&payload[25], encode_micros(p->acceleration(0.0).x));
		encode_u32(&payload[29], encode_micros(p->acceleration(0.0).y));
		encode_u32(&payload[33], encode_micros(p->aacceleration(0.0)));
		write_packet(fd, Log::T_ENEMY_ROBOT, payload, sizeof(payload));
	}
	{
		const AI::BE::Ball &ball = ai.backend.ball();
		uint8_t payload[24];
		encode_u32(&payload[0], encode_micros(ball.position(0.0).x));
		encode_u32(&payload[4], encode_micros(ball.position(0.0).y));
		encode_u32(&payload[8], encode_micros(ball.velocity(0.0).x));
		encode_u32(&payload[12], encode_micros(ball.velocity(0.0).y));
		encode_u32(&payload[16], encode_micros(ball.acceleration(0.0).x));
		encode_u32(&payload[20], encode_micros(ball.acceleration(0.0).y));
		write_packet(fd, Log::T_BALL, payload, sizeof(payload));
	}
	{
		uint8_t payload[24];
		timespec now;
		timespec_now(now, CLOCK_REALTIME);
		encode_u64(&payload[0], now.tv_sec);
		encode_u32(&payload[8], static_cast<uint32_t>(now.tv_nsec));
		now = ai.backend.monotonic_time();
		encode_u64(&payload[12], now.tv_sec);
		encode_u32(&payload[20], static_cast<uint32_t>(now.tv_nsec));
		write_packet(fd, Log::T_AI_TICK, payload, sizeof(payload));
	}
}

void AI::Logger::on_high_level_changed() {
	AI::HL::HighLevel::Ptr high_level = ai.high_level;
	if (high_level.is()) {
		const std::string &utf8name = high_level->factory().name();
		write_packet(fd, Log::T_HIGH_LEVEL, utf8name.data(), utf8name.size());
	} else {
		write_packet(fd, Log::T_HIGH_LEVEL, 0, 0);
	}
}

