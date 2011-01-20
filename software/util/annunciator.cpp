#include "util/annunciator.h"
#include "util/dprint.h"
#include "util/misc.h"
#include <unordered_map>

namespace {
	const unsigned int MAX_AGE = 20;

	unsigned int next_id = 0;

	std::unordered_map<unsigned int, Annunciator::Message *> &registered() {
		static std::unordered_map<unsigned int, Annunciator::Message *> r;
		return r;
	}

	std::vector<Annunciator::Message *> displayed;
}

Annunciator::Message::Message(const Glib::ustring &text) : text(text), id(next_id++), active_(false), age_(0), displayed_(false) {
	registered()[id] = this;
}

Annunciator::Message::~Message() {
	hide();
	registered().erase(id);
}

void Annunciator::Message::activate(bool actv) {
	if (actv != active_) {
		active_ = actv;
		for (std::size_t i = 0; i < displayed.size(); ++i) {
			if (displayed[i] == this) {
				if (actv) {
					signal_message_reactivated.emit(i);
				} else {
					signal_message_deactivated.emit(i);
				}
			}
		}
		one_second_connection.disconnect();
		if (actv) {
			age_ = 0;
			if (!displayed_) {
				displayed.push_back(this);
				signal_message_activated.emit();
				displayed_ = true;
			}
			LOG_ERROR(text);
		} else {
			one_second_connection = Glib::signal_timeout().connect_seconds(sigc::mem_fun(this, &Annunciator::Message::on_one_second), 1);
		}
	}
}

bool Annunciator::Message::on_one_second() {
	if (age_ < MAX_AGE) {
		++age_;
		for (std::size_t i = 0; i < displayed.size(); ++i) {
			if (displayed[i] == this) {
				signal_message_aging.emit(i);
			}
		}
		return true;
	} else {
		hide();
		return false;
	}
}

void Annunciator::Message::hide() {
	for (std::size_t i = 0; i < displayed.size(); ++i) {
		if (displayed[i] == this) {
			signal_message_hidden.emit(i);
			displayed.erase(displayed.begin() + i);
			--i;
		}
	}
	displayed_ = false;
}

const std::vector<Annunciator::Message *> &Annunciator::visible() {
	return displayed;
}

sigc::signal<void> Annunciator::signal_message_activated;

sigc::signal<void, std::size_t> Annunciator::signal_message_deactivated;

sigc::signal<void, std::size_t> Annunciator::signal_message_aging;

sigc::signal<void, std::size_t> Annunciator::signal_message_reactivated;

sigc::signal<void, std::size_t> Annunciator::signal_message_hidden;

namespace {
	bool can_siren() {
		static const std::string cmdline_raw[] = { "beep", "-f", "1", "-l", "10" };
		static const std::vector<std::string> cmdline(&cmdline_raw[0], &cmdline_raw[G_N_ELEMENTS(cmdline_raw)]);
		try {
			int exit_status;
			Glib::spawn_sync("", cmdline, Glib::SPAWN_SEARCH_PATH, sigc::slot<void>(), 0, 0, &exit_status);
			return wifexited(exit_status) && wexitstatus(exit_status) == 0;
		} catch (const Glib::SpawnError &) {
			return false;
		}
	}

	std::vector<std::string> make_siren_cmdline() {
		std::vector<std::string> v;
		v.push_back("beep");
		for (unsigned int i = 0; i < 2; ++i) {
			for (unsigned int j = 500; j <= 1500; j += 50) {
				v.push_back("-f");
				v.push_back(Glib::ustring::compose("%1", j));
				v.push_back("-l");
				v.push_back("10");
				v.push_back("-n");
			}
			for (unsigned int j = 1500; j >= 500; j -= 50) {
				v.push_back("-f");
				v.push_back(Glib::ustring::compose("%1", j));
				v.push_back("-l");
				v.push_back("10");
				v.push_back("-n");
			}
		}
		v.pop_back();
		return v;
	}

	void siren() {
		static const std::vector<std::string> &cmdline(make_siren_cmdline());
		try {
			Glib::spawn_async("", cmdline, Glib::SPAWN_SEARCH_PATH);
		} catch (const Glib::SpawnError &) {
			// Swallow error.
		}
	}

	class SirenAvailabilityWarner : public NonCopyable {
		public:
			static void ensure_exists() {
				static SirenAvailabilityWarner instance;
			}

		private:
			Annunciator::Message msg;

			SirenAvailabilityWarner() : msg("\"beep\" executable unavailable, no annunciator sirens") {
				if (!can_siren()) {
					msg.activate(true);
				}
			}
	};
}

void Annunciator::activate_siren() {
	static bool activated = false;
	if (!activated) {
		SirenAvailabilityWarner::ensure_exists();
		signal_message_activated.connect(&siren);
		signal_message_reactivated.connect(sigc::hide(&siren));
		activated = true;
	}
}

