#include "log/player.h"
#include "ai/common/playtype.h"
#include "log/loader.h"
#include "log/shared/enums.h"
#include "proto/log_record.pb.h"
#include "util/algorithm.h"
#include "util/box.h"
#include "util/noncopyable.h"
#include "util/string.h"
#include <chrono>
#include <array>
#include <cstdlib>
#include <iterator>
#include <vector>
#include <gdk/gdkkeysyms.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/box.h>
#include <gtkmm/frame.h>
#include <gtkmm/label.h>
#include <gtkmm/scale.h>
#include <gtkmm/stock.h>
#include <gtkmm/table.h>
#include <gtkmm/toggletoolbutton.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/toolbutton.h>
#include <sigc++/bind.h>
#include <sigc++/bind_return.h>
#include <sigc++/hide.h>
#include <sigc++/functors/mem_fun.h>

namespace {
	std::chrono::steady_clock::time_point make_monotonic_time(const Log::MonotonicTimeSpec &target, const Log::MonotonicTimeSpec &reference) {
		std::chrono::duration<intmax_t, std::nano> target_nanos(static_cast<intmax_t>(target.seconds()) * 1000000000 + target.nanoseconds());
		std::chrono::duration<intmax_t, std::nano> reference_nanos(static_cast<intmax_t>(reference.seconds()) * 1000000000 + target.nanoseconds());
		return std::chrono::steady_clock::time_point(std::chrono::duration_cast<std::chrono::steady_clock::duration>(target_nanos - reference_nanos));
	}

	double decode_micros(int32_t x) {
		return x / 1.0e6;
	}

	double decode_micros_unsigned(uint32_t x) {
		return x / 1.0e6;
	}

	class Field : public NonCopyable, public Visualizable::Field {
		public:
			Field() : valid_(false) {
			}

			bool valid() const {
				return valid_;
			}

			double length() const {
				return length_;
			}

			double total_length() const {
				return total_length_;
			}

			double width() const {
				return width_;
			}

			double total_width() const {
				return total_width_;
			}

			double goal_width() const {
				return goal_width_;
			}

			double centre_circle_radius() const {
				return centre_circle_radius_;
			}

			double defense_area_radius() const {
				return defense_area_radius_;
			}

			double defense_area_stretch() const {
				return defense_area_stretch_;
			}

			void update(const Log::Field &field) {
				valid_ = true;
				length_ = decode_micros_unsigned(field.length());
				total_length_ = decode_micros_unsigned(field.total_length());
				width_ = decode_micros_unsigned(field.width());
				total_width_ = decode_micros_unsigned(field.total_width());
				goal_width_ = decode_micros_unsigned(field.goal_width());
				centre_circle_radius_ = decode_micros_unsigned(field.centre_circle_radius());
				defense_area_radius_ = decode_micros_unsigned(field.defense_area_radius());
				defense_area_stretch_ = decode_micros_unsigned(field.defense_area_stretch());
				signal_changed.emit();
			}

		private:
			bool valid_;
			double length_, total_length_, width_, total_width_, goal_width_, centre_circle_radius_, defense_area_radius_, defense_area_stretch_;
	};

	class Ball : public NonCopyable, public Visualizable::Ball {
		public:
			Point position(double) const {
				return position_;
			}

			Point velocity(double) const {
				return velocity_;
			}

			bool highlight() const {
				return false;
			}

			Visualizable::Colour highlight_colour() const {
				std::abort();
			}

			void update(const Log::Tick::Ball &ball) {
				position_.x = decode_micros(ball.position().x());
				position_.y = decode_micros(ball.position().y());
				velocity_.x = decode_micros(ball.velocity().x());
				velocity_.y = decode_micros(ball.velocity().y());
			}

		private:
			Point position_, velocity_;
	};

	class Robot : public Visualizable::Robot {
		public:
			typedef BoxPtr<Robot> Ptr;

			void update(const Log::Tick::EnemyRobot &bot) {
				update(bot.pattern(), bot.position(), bot.velocity());
			}

			void update(unsigned int pattern, const Log::Vector3 &position, const Log::Vector3 &velocity) {
				pattern_ = pattern;
				position_.x = decode_micros(position.x());
				position_.y = decode_micros(position.y());
				velocity_.x = decode_micros(velocity.x());
				velocity_.y = decode_micros(velocity.y());
				orientation_ = Angle::of_radians(decode_micros(position.t()));
			}

			Point position(double) const {
				return position_;
			}

			Angle orientation(double) const {
				return orientation_;
			}

			Point velocity(double) const {
				return velocity_;
			}

			Visualizable::Colour visualizer_colour() const {
				return Visualizable::Colour(1, 0, 0);
			}

			Glib::ustring visualizer_label() const {
				return Glib::ustring::format(pattern_);
			}

			bool highlight() const {
				return false;
			}

			Visualizable::Colour highlight_colour() const {
				std::abort();
			}

			bool has_destination() const {
				return false;
			}

			std::pair<Point, Angle> destination() const {
				std::abort();
			}

			bool has_path() const {
				return false;
			}

			const std::vector<std::pair<std::pair<Point, Angle>, std::chrono::steady_clock::time_point>> &path() const {
				std::abort();
			}

			unsigned int num_bar_graphs() const {
				return 0;
			}

			double bar_graph_value(unsigned int) const {
				std::abort();
			}

			Visualizable::Colour bar_graph_colour(unsigned int) const {
				std::abort();
			}

		private:
			unsigned int pattern_;
			Point position_, velocity_;
			Angle orientation_;
	};

	class Player : public Robot {
		public:
			typedef BoxPtr<Player> Ptr;

			void update(const Log::MonotonicTimeSpec &reference_time, const Log::Tick::FriendlyRobot &bot) {
				Robot::update(bot.pattern(), bot.position(), bot.velocity());
				destination_.first.x = decode_micros(bot.target().x());
				destination_.first.y = decode_micros(bot.target().y());
				destination_.second = Angle::of_radians(decode_micros(bot.target().t()));
				path_.clear();
				for (auto i = bot.path().begin(), iend = bot.path().end(); i != iend; ++i) {
					double x = decode_micros(i->point().x());
					double y = decode_micros(i->point().y());
					Angle t = Angle::of_radians(decode_micros(i->point().t()));
					path_.push_back(std::make_pair(std::make_pair(Point(x, y), t), make_monotonic_time(i->timestamp(), reference_time)));
				}
			}

			Visualizable::Colour visualizer_colour() const {
				return Visualizable::Colour(0, 1, 0);
			}

			bool has_destination() const {
				return true;
			}

			std::pair<Point, Angle> destination() const {
				return destination_;
			}

			bool has_path() const {
				return true;
			}

			const std::vector<std::pair<std::pair<Point, Angle>, std::chrono::steady_clock::time_point>> &path() const {
				return path_;
			}

		private:
			std::pair<Point, Angle> destination_;
			std::vector<std::pair<std::pair<Point, Angle>, std::chrono::steady_clock::time_point>> path_;
	};
}

class LogPlayer::Impl : public Gtk::VBox, public Visualizable::World {
	public:
		Impl(Gtk::Window &parent, const std::string &pathname) :
				records(LogLoader::load(pathname)),
				top_info_table(11, 2, false),
				ball_filter_label("Ball Filter:"),
				strategy_label("Strategy:"),
				backend_label("Backend:"),
				high_level_label("HL:"),
				robot_controller_label("Controller:"),
				friendly_colour_label("Colour:"),
				ticks_per_second_label("Tick Rate:"),
				play_type_label("Play Type:"),
				friendly_score_label("Points Us:"),
				enemy_score_label("Points Them:"),
				visualizer(*this),
				full_screen_button(Gtk::Stock::FULLSCREEN),
				start_button(Gtk::Stock::MEDIA_PREVIOUS),
				play_button(Gtk::Stock::MEDIA_PLAY),
				end_button(Gtk::Stock::MEDIA_NEXT) {

			find_ticks_per_second();
			scan_records();
			field_.update(field_records_by_tick[0]->field());
			if (tick_records.empty()) {
				throw std::runtime_error("No ticks in this log.");
			}
			game_start_monotonic = tick_records[0]->tick().start_time();

			ball_filter_value.set_width_chars(25);

			ball_filter_frame.set_shadow_type(Gtk::SHADOW_IN);
			strategy_frame.set_shadow_type(Gtk::SHADOW_IN);
			backend_frame.set_shadow_type(Gtk::SHADOW_IN);
			high_level_frame.set_shadow_type(Gtk::SHADOW_IN);
			robot_controller_frame.set_shadow_type(Gtk::SHADOW_IN);
			friendly_colour_frame.set_shadow_type(Gtk::SHADOW_IN);
			ticks_per_second_frame.set_shadow_type(Gtk::SHADOW_IN);
			play_type_frame.set_shadow_type(Gtk::SHADOW_IN);
			friendly_score_frame.set_shadow_type(Gtk::SHADOW_IN);
			enemy_score_frame.set_shadow_type(Gtk::SHADOW_IN);
			ai_notes_frame.set_shadow_type(Gtk::SHADOW_IN);

			ticks_per_second_value.set_text(Glib::ustring::format(ticks_per_second));

			ball_filter_frame.add(ball_filter_value);
			strategy_frame.add(strategy_value);
			backend_frame.add(backend_value);
			high_level_frame.add(high_level_value);
			robot_controller_frame.add(robot_controller_value);
			friendly_colour_frame.add(friendly_colour_value);
			ticks_per_second_frame.add(ticks_per_second_value);
			play_type_frame.add(play_type_value);
			friendly_score_frame.add(friendly_score_value);
			enemy_score_frame.add(enemy_score_value);
			ai_notes_frame.add(ai_notes_value);

			top_info_table.attach(ball_filter_label, 0, 1, 0, 1);
			top_info_table.attach(ball_filter_frame, 1, 2, 0, 1);
			top_info_table.attach(strategy_label, 0, 1, 1, 2);
			top_info_table.attach(strategy_frame, 1, 2, 1, 2);
			top_info_table.attach(backend_label, 0, 1, 2, 3);
			top_info_table.attach(backend_frame, 1, 2, 2, 3);
			top_info_table.attach(high_level_label, 0, 1, 3, 4);
			top_info_table.attach(high_level_frame, 1, 2, 3, 4);
			top_info_table.attach(robot_controller_label, 0, 1, 4, 5);
			top_info_table.attach(robot_controller_frame, 1, 2, 4, 5);
			top_info_table.attach(friendly_colour_label, 0, 1, 5, 6);
			top_info_table.attach(friendly_colour_frame, 1, 2, 5, 6);
			top_info_table.attach(ticks_per_second_label, 0, 1, 6, 7);
			top_info_table.attach(ticks_per_second_frame, 1, 2, 6, 7);
			top_info_table.attach(play_type_label, 0, 1, 7, 8);
			top_info_table.attach(play_type_frame, 1, 2, 7, 8);
			top_info_table.attach(friendly_score_label, 0, 1, 8, 9);
			top_info_table.attach(friendly_score_frame, 1, 2, 8, 9);
			top_info_table.attach(enemy_score_label, 0, 1, 9, 10);
			top_info_table.attach(enemy_score_frame, 1, 2, 9, 10);
			top_info_table.attach(ai_notes_frame, 0, 2, 10, 11);

			top_info_vbox.pack_start(top_info_table, Gtk::PACK_SHRINK);

			upper_hbox.pack_start(top_info_vbox, Gtk::PACK_SHRINK);

			visualizer.show_robots_v = true;

			upper_hbox.pack_start(visualizer, Gtk::PACK_EXPAND_WIDGET);

			time_slider.set_digits(0);
			time_slider.set_draw_value();
			time_slider.set_tooltip_text("Tick Index");
			time_slider.get_adjustment()->configure(0, 0, static_cast<double>(tick_records.size() - 1), 1, ticks_per_second, 0);
			time_slider.get_adjustment()->signal_value_changed().connect(sigc::mem_fun(this, &Impl::update_with_tick));

			lower_hbox.pack_start(time_slider, Gtk::PACK_EXPAND_WIDGET);

			full_screen_button.set_tooltip_text("Toggle Full-Screen Mode");
			start_button.set_tooltip_text("Seek to Start of Log");
			play_button.set_tooltip_text("Play Log");
			end_button.set_tooltip_text("Seek to End of Log");
			full_screen_button.signal_toggled().connect(sigc::mem_fun(this, &Impl::toggle_full_screen));
			start_button.signal_clicked().connect(sigc::mem_fun(this, &Impl::seek_to_start));
			play_button.signal_clicked().connect(sigc::mem_fun(this, &Impl::play_or_stop));
			end_button.signal_clicked().connect(sigc::mem_fun(this, &Impl::seek_to_end));
			media_toolbar.set_toolbar_style(Gtk::TOOLBAR_ICONS);
			media_toolbar.set_show_arrow(false);
			media_toolbar.append(full_screen_button);
			media_toolbar.append(start_button);
			media_toolbar.append(play_button);
			media_toolbar.append(end_button);

			lower_hbox.pack_start(media_toolbar, Gtk::PACK_SHRINK);

			timestamp_label.set_width_chars(15);
			timestamp_label.set_tooltip_text("Time from Start of Log");
			timestamp_frame.set_shadow_type(Gtk::SHADOW_IN);
			timestamp_frame.add(timestamp_label);

			lower_hbox.pack_start(timestamp_frame, Gtk::PACK_SHRINK);

			packet_label.set_width_chars(8);
			packet_label.set_tooltip_text("Packet Index of Tick Record");
			packet_frame.set_shadow_type(Gtk::SHADOW_IN);
			packet_frame.add(packet_label);

			lower_hbox.pack_start(packet_frame, Gtk::PACK_SHRINK);

			pack_start(upper_hbox, Gtk::PACK_EXPAND_WIDGET);
			pack_start(lower_hbox, Gtk::PACK_SHRINK);

			full_screen_window.set_title(Glib::ustring::compose("Thunderbots Log Tools - Player - %1", Glib::filename_display_basename(pathname)));
			full_screen_window.set_transient_for(parent);
			full_screen_window.set_modal();
			full_screen_window.signal_delete_event().connect(sigc::bind_return(sigc::hide(sigc::bind(sigc::mem_fun(full_screen_button, &Gtk::ToggleToolButton::set_active), false)), true));
			full_screen_window.signal_key_press_event().connect(sigc::mem_fun(this, &Impl::check_for_full_screen_escape));

			update_with_tick();
		}

		const Visualizable::Field &field() const {
			return field_;
		}

		const Visualizable::Ball &ball() const {
			return ball_;
		}

		std::size_t visualizable_num_robots() const {
			return players_.size() + robots_.size();
		}

		Visualizable::Robot::Ptr visualizable_robot(std::size_t index) const {
			assert(index < visualizable_num_robots());
			if (index < players_.size()) {
				return players_[index].ptr();
			} else {
				return robots_[index - players_.size()].ptr();
			}
		}

		sigc::signal<void> &signal_tick() const {
			return signal_tick_;
		}

		void mouse_pressed(Point, unsigned int) {
			if (full_screen_button.get_active()) {
				play_or_stop();
			}
		}

		void mouse_released(Point, unsigned int) {
		}

		void mouse_exited() {
		}

		void mouse_moved(Point) {
		}

		void draw_overlay(Cairo::RefPtr<Cairo::Context> ) const {
		}

	private:
		std::vector<Log::Record> records;
		std::vector<std::vector<Log::Record>::const_iterator> tick_records;
		std::vector<std::vector<Log::Record>::const_iterator> field_records_by_tick;
		std::vector<std::vector<Log::Record>::const_iterator> config_records_by_tick;
		std::vector<std::vector<Log::Record>::const_iterator> scores_records_by_tick;
		std::vector<std::vector<Log::Record>::const_iterator> ai_notes_records_by_tick;
		unsigned int ticks_per_second;
		Log::MonotonicTimeSpec game_start_monotonic;
		Field field_;
		Ball ball_;
		std::array< ::Box<Player>, 16> players_;
		std::array< ::Box<Robot>, 16> robots_;
		sigc::connection play_timer_connection;
		mutable sigc::signal<void> signal_tick_;

		Gtk::HBox upper_hbox;
		Gtk::VBox top_info_vbox;
		Gtk::Table top_info_table;
		Gtk::Label ball_filter_label, strategy_label, backend_label, high_level_label, robot_controller_label, friendly_colour_label, ticks_per_second_label, play_type_label, friendly_score_label, enemy_score_label;
		Gtk::Frame ball_filter_frame, strategy_frame, backend_frame, high_level_frame, robot_controller_frame, friendly_colour_frame, ticks_per_second_frame, play_type_frame, friendly_score_frame, enemy_score_frame, ai_notes_frame;
		Gtk::Label ball_filter_value, strategy_value, backend_value, high_level_value, robot_controller_value, friendly_colour_value, ticks_per_second_value, play_type_value, friendly_score_value, enemy_score_value, ai_notes_value;
		Visualizer visualizer;

		Gtk::HBox lower_hbox;
		Gtk::HScale time_slider;
		Gtk::Toolbar media_toolbar;
		Gtk::ToggleToolButton full_screen_button;
		Gtk::ToolButton start_button, play_button, end_button;
		Gtk::Frame timestamp_frame;
		Gtk::Label timestamp_label;
		Gtk::Frame packet_frame;
		Gtk::Label packet_label;

		Gtk::Window full_screen_window;

		void find_ticks_per_second() {
			if (records.size() < 2 || !records[1].has_config()) {
				throw std::runtime_error("Log truncated.");
			}
			ticks_per_second = records[1].config().nominal_ticks_per_second();
		}

		void scan_records() {
			std::vector<Log::Record>::const_iterator field_iter = records.end(), config_iter = records.end(), scores_iter = records.end(), ai_notes_iter = records.end();
			for (auto i = records.begin(), iend = records.end(); i != iend; ++i) {
				const Log::Record &record = *i;
				if (record.has_field()) {
					field_iter = i;
				} else if (record.has_config()) {
					config_iter = i;
				} else if (record.has_scores()) {
					scores_iter = i;
				} else if (record.has_ai_notes()) {
					ai_notes_iter = i;
				} else if (record.has_tick() && field_iter != records.end() && config_iter != records.end() && scores_iter != records.end()) {
					tick_records.push_back(i);
					field_records_by_tick.push_back(field_iter);
					config_records_by_tick.push_back(config_iter);
					scores_records_by_tick.push_back(scores_iter);
					ai_notes_records_by_tick.push_back(ai_notes_iter);
				}
			}
			if (field_iter == records.end()) {
				throw std::runtime_error("No field geometry.");
			}
			if (config_iter == records.end()) {
				throw std::runtime_error("No config record.");
			}
			if (scores_iter == records.end()) {
				throw std::runtime_error("No scores record.");
			}
		}

		void toggle_full_screen() {
			if (full_screen_button.get_active()) {
				upper_hbox.remove(visualizer);
				full_screen_window.add(visualizer);
				full_screen_window.fullscreen();
				full_screen_window.show_all();
			} else {
				full_screen_window.remove();
				upper_hbox.pack_start(visualizer, Gtk::PACK_EXPAND_WIDGET);
				full_screen_window.hide();
			}
		}

		void seek_to_start() {
			time_slider.set_value(0);
		}

		void play_or_stop() {
			if (play_timer_connection.connected()) {
				stop();
			} else {
				play();
			}
		}

		void seek_to_end() {
			time_slider.set_value(static_cast<double>(tick_records.size() - 1));
		}

		void play() {
			play_timer_connection.disconnect();
			play_timer_connection = Glib::signal_timeout().connect(sigc::mem_fun(this, &Impl::step_time), (1000 + ticks_per_second / 2) / ticks_per_second);
			play_button.set_stock_id(Gtk::Stock::MEDIA_PAUSE);
			play_button.set_tooltip_text("Stop Playback");
		}

		void stop() {
			play_timer_connection.disconnect();
			play_button.set_stock_id(Gtk::Stock::MEDIA_PLAY);
			play_button.set_tooltip_text("Play Log");
		}

		bool check_for_full_screen_escape(GdkEventKey *evt) {
			if (evt->keyval == GDK_KEY_Escape) {
				full_screen_button.set_active(false);
				return true;
			} else {
				return false;
			}
		}

		bool step_time() {
			std::size_t position = clamp<std::size_t>(static_cast<std::size_t>(time_slider.get_value() + 0.5) + 1, 0, tick_records.size() - 1);
			time_slider.set_value(static_cast<double>(position));
			if (position == tick_records.size() - 1) {
				stop();
			}
			return true;
		}

		void update_with_tick() {
			std::size_t position = clamp<std::size_t>(static_cast<std::size_t>(time_slider.get_value() + 0.5), 0, tick_records.size() - 1);

			const Log::Tick &tick = tick_records[position]->tick();
			const Log::Field &field = field_records_by_tick[position]->field();
			const Log::Config &config = config_records_by_tick[position]->config();
			const Log::Scores &scores = scores_records_by_tick[position]->scores();
			auto ai_notes_iter = ai_notes_records_by_tick[position];
			const Glib::ustring &ai_notes = ai_notes_iter != records.end() ? ai_notes_iter->ai_notes() : u8"";

			std::chrono::steady_clock::time_point tick_time = make_monotonic_time(tick.start_time(), game_start_monotonic);
			std::chrono::duration<intmax_t, std::nano> nanos_from_game_start = std::chrono::duration_cast<std::chrono::duration<intmax_t, std::nano>>(tick_time.time_since_epoch());
			unsigned int milliseconds = static_cast<unsigned int>(nanos_from_game_start.count() / 1000000);
			unsigned int seconds = static_cast<unsigned int>((nanos_from_game_start.count() / 1000000000) % 60);
			unsigned int minutes = static_cast<unsigned int>(((nanos_from_game_start.count() / 1000000000) / 60) % 60);
			unsigned int hours = static_cast<unsigned int>((nanos_from_game_start.count() / 1000000000) / 3600);
			timestamp_label.set_text(Glib::ustring::compose("%1:%2:%3.%4", todecu(hours, 2), todecu(minutes, 2), todecu(seconds, 2), todecu(milliseconds, 3)));

			packet_label.set_text(Glib::ustring::format(std::distance(static_cast<const std::vector<Log::Record> &>(records).begin(), tick_records[position])));

			field_.update(field);

			ball_.update(tick.ball());

			{
				bool seen[players_.size()];
				std::fill(seen, seen + G_N_ELEMENTS(seen), false);
				for (int i = 0; i < tick.friendly_robots_size(); ++i) {
					const Log::Tick::FriendlyRobot &bot = tick.friendly_robots(i);
					if (!players_[bot.pattern()]) {
						players_[bot.pattern()].create();
					}
					players_[bot.pattern()].value().update(game_start_monotonic, bot);
					seen[bot.pattern()] = true;
				}
				for (std::size_t i = 0; i < G_N_ELEMENTS(seen); ++i) {
					if (!seen[i]) {
						players_[i].destroy();
					}
				}
			}

			{
				bool seen[robots_.size()];
				std::fill(seen, seen + G_N_ELEMENTS(seen), false);
				for (int i = 0; i < tick.enemy_robots_size(); ++i) {
					const Log::Tick::EnemyRobot &bot = tick.enemy_robots(i);
					if (!robots_[bot.pattern()]) {
						robots_[bot.pattern()].create();
					}
					robots_[bot.pattern()].value().update(bot);
					seen[bot.pattern()] = true;
				}
				for (std::size_t i = 0; i < G_N_ELEMENTS(seen); ++i) {
					if (!seen[i]) {
						robots_[i].destroy();
					}
				}
			}

			ball_filter_value.set_text(config.has_ball_filter() ? config.ball_filter() : "<None>");
			strategy_value.set_text(config.has_strategy() ? config.strategy() : "<None>");
			backend_value.set_text(config.backend());
			high_level_value.set_text(config.has_high_level() ? config.high_level() : "<None>");
			robot_controller_value.set_text(config.has_robot_controller() ? config.robot_controller() : "<None>");
			switch (config.friendly_colour()) {
				case Log::COLOUR_YELLOW:
					friendly_colour_value.set_text("Yellow");
					break;

				case Log::COLOUR_BLUE:
					friendly_colour_value.set_text("Blue");
					break;
			}
			play_type_value.set_text(AI::Common::PlayTypeInfo::to_string(Log::Util::PlayType::of_protobuf(tick.play_type())));
			friendly_score_value.set_text(Glib::ustring::format(scores.friendly()));
			enemy_score_value.set_text(Glib::ustring::format(scores.enemy()));
			ai_notes_value.set_text(ai_notes);

			signal_tick_.emit();
		}
};

LogPlayer::LogPlayer(Gtk::Window &parent, const std::string &pathname) : impl(new Impl(*this, pathname)) {
	set_title(Glib::ustring::compose("Thunderbots Log Tools - Player - %1", Glib::filename_display_basename(pathname)));
	set_transient_for(parent);
	set_modal(false);
	set_size_request(500, 500);

	add(*impl);

	show_all();
}

bool LogPlayer::on_delete_event(GdkEventAny *) {
	delete this;
	return true;
}

