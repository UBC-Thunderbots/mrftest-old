#ifndef AI_LOGGER_H
#define AI_LOGGER_H

#include "ai/ai.h"
#include "mrf/packet_logger.h"
#include "proto/log_record.pb.h"
#include "proto/referee.pb.h"
#include "util/fd.h"
#include "util/noncopyable.h"
#include "util/param.h"
#include "util/signal.h"
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <unordered_map>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <sigc++/trackable.h>

extern "C" {
	void ai_logger_signal_handler_thunk(int sig);
}

class ParamTreeNode;

namespace AI {
	/**
	 * Records a log of a game for later playback.
	 */
	class Logger final : public MRFPacketLogger, public NonCopyable, public sigc::trackable {
		public:
			/**
			 * Constructs a new Logger and opens a log file.
			 *
			 * \param[in] ai the AI package to log.
			 */
			explicit Logger(const AI::AIPackage &ai);

			/**
			 * Destroys a Logger and closes the log.
			 */
			~Logger();

			/**
			 * Records termination of the application due to an exception.
			 *
			 * \param[in] msg a message describing the exception.
			 */
			void end_with_exception(const Glib::ustring &msg);

			void log_mrf_drive(const void *data, std::size_t length) override;
			void log_mrf_message_out(unsigned int index, bool reliable, unsigned int id, const void *data, std::size_t length) override;
			void log_mrf_message_in(unsigned int index, const void *data, std::size_t length, unsigned int lqi, unsigned int rssi) override;
			void log_mrf_mdr(unsigned int id, unsigned int code) override;

		private:
			const AI::AIPackage &ai;
			const FileDescriptor fd;
			google::protobuf::io::FileOutputStream fos;
			bool ended;
			unsigned char sigstack[65536];
			SignalStackScopedRegistration sigstack_registration;
			SignalHandlerScopedRegistration SIGHUP_registration;
			SignalHandlerScopedRegistration SIGINT_registration;
			SignalHandlerScopedRegistration SIGQUIT_registration;
			SignalHandlerScopedRegistration SIGILL_registration;
			SignalHandlerScopedRegistration SIGTRAP_registration;
			SignalHandlerScopedRegistration SIGABRT_registration;
			SignalHandlerScopedRegistration SIGBUS_registration;
			SignalHandlerScopedRegistration SIGFPE_registration;
			SignalHandlerScopedRegistration SIGSEGV_registration;
			SignalHandlerScopedRegistration SIGPIPE_registration;
			SignalHandlerScopedRegistration SIGTERM_registration;
			SignalHandlerScopedRegistration SIGSTKFLT_registration;
			Log::Record config_record;

			void write_record(const Log::Record &record);
			void flush();
			void add_params_to_record(Log::Record &record, const ParamTreeNode *node);
			void attach_param_change_handler(ParamTreeNode *node);
			void signal_handler(int sig);
			void on_message_logged(Log::DebugMessageLevel level, const Glib::ustring &msg);
			void log_annunciator(std::size_t i, bool activated);
			void on_annunciator_message_activated();
			void on_annunciator_message_deactivated(std::size_t i);
			void on_annunciator_message_reactivated(std::size_t i);
			void on_annunciator_message_text_changed(std::size_t i);
			void on_param_changed(const Param *p);
			void on_vision_packet(AI::Timestamp ts, const SSL_WrapperPacket &vision_packet);
			void on_refbox_packet(AI::Timestamp ts, const SSL_Referee &packet);
			void on_field_changed();
			void on_friendly_colour_changed();
			void on_high_level_changed();
			void on_score_changed();
			void on_ai_notes_changed(const Glib::ustring &notes);
			void on_tick(AI::Timediff compute_time);

			static void encode_hlp_vec23(const AI::PrimitiveInfo &hl_request, Log::Tick::FriendlyRobot::HLPrimitive &log);
			static void encode_vec2(Point p, Log::Vector2 &log);
			static void encode_vec3(Point p, Angle a, Log::Vector3 &log);

			friend void ::ai_logger_signal_handler_thunk(int sig);
	};
}

#endif
