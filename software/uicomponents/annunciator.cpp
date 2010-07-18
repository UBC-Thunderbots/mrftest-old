#include "uicomponents/abstract_list_model.h"
#include "uicomponents/annunciator.h"
#include "util/dprint.h"
#include "util/misc.h"
#include "util/noncopyable.h"
#include <cstdlib>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>

namespace {
	const unsigned int MAX_AGE = 20;

	bool can_siren() {
		static const std::string cmdline_raw[] = { "beep", "-f", "1", "-l", "10" };
		static const std::vector<std::string> cmdline(&cmdline_raw[0], &cmdline_raw[sizeof(cmdline_raw) / sizeof(*cmdline_raw)]);
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

	unsigned int next_id = 0;
	std::unordered_map<unsigned int, Annunciator::message *> registered;
	std::vector<Annunciator::message *> displayed;

	class SirenAvailabilityWarner : public NonCopyable {
		public:
			static void ensure_exists() {
				static SirenAvailabilityWarner instance;
			}

		private:
			Annunciator::message msg;

			SirenAvailabilityWarner() : msg("\"beep\" executable unavailable, no annunciator sirens") {
				if (!can_siren()) {
					msg.activate(true);
				}
			}
	};

	class MessagesALM : public Glib::Object, public AbstractListModel, public NonCopyable {
		public:
			Gtk::TreeModelColumn<unsigned int> age_column;
			Gtk::TreeModelColumn<Glib::ustring> message_column;
			Gtk::TreeModelColumn<bool> active_column;

			static Glib::RefPtr<MessagesALM> instance() {
				static Glib::RefPtr<MessagesALM> inst;
				if (!inst) {
					inst = Glib::RefPtr<MessagesALM>(new MessagesALM);
				}
				return inst;
			}

		private:
			MessagesALM() : Glib::ObjectBase(typeid(MessagesALM)) {
				alm_column_record.add(age_column);
				alm_column_record.add(message_column);
				alm_column_record.add(active_column);
			}

			~MessagesALM() {
			}

			unsigned int alm_rows() const {
				return displayed.size();
			}

			void alm_get_value(unsigned int row, unsigned int col, Glib::ValueBase &value) const {
				if (col == static_cast<unsigned int>(age_column.index())) {
					Glib::Value<unsigned int> v;
					v.init(age_column.type());
					v.set(displayed[row]->age());
					value.init(age_column.type());
					value = v;
				} else if (col == static_cast<unsigned int>(message_column.index())) {
					Glib::Value<Glib::ustring> v;
					v.init(message_column.type());
					v.set(displayed[row]->text);
					value.init(message_column.type());
					value = v;
				} else if (col == static_cast<unsigned int>(active_column.index())) {
					Glib::Value<bool> v;
					v.init(active_column.type());
					v.set(displayed[row]->active());
					value.init(active_column.type());
					value = v;
				} else {
					std::abort();
				}
			}

			void alm_set_value(unsigned int, unsigned int, const Glib::ValueBase &) {
			}

			friend class Annunciator::message;
	};

	void message_cell_data_func(Gtk::CellRenderer *r, const Gtk::TreeModel::iterator &iter) {
		Gtk::CellRendererText *rt = dynamic_cast<Gtk::CellRendererText *>(r);
		rt->property_text() = iter->get_value(MessagesALM::instance()->message_column);
		bool active = iter->get_value(MessagesALM::instance()->active_column);
		rt->property_foreground() = active ? "white" : "black";
		rt->property_background() = active ? "red" : "white";
	}
}

Annunciator::message::message(const Glib::ustring &text) : text(text), id(next_id++), active_(false), age_(0), displayed_(false) {
	registered[id] = this;
}

Annunciator::message::~message() {
	hide();
	registered.erase(id);
}

void Annunciator::message::activate(bool actv) {
	if (actv != active_) {
		active_ = actv;
		for (unsigned int i = 0; i < displayed.size(); ++i) {
			if (displayed[i] == this) {
				MessagesALM::instance()->alm_row_changed(i);
			}
		}
		one_second_connection.disconnect();
		if (actv) {
			age_ = 0;
			if (!displayed_) {
				unsigned int index = displayed.size();
				displayed.push_back(this);
				MessagesALM::instance()->alm_row_inserted(index);
				displayed_ = true;
			}
			siren();
			LOG_ERROR(text);
		} else {
			one_second_connection = Glib::signal_timeout().connect_seconds(sigc::mem_fun(this, &Annunciator::message::on_one_second), 1);
		}
	}
}

bool Annunciator::message::on_one_second() {
	if (age_ < MAX_AGE) {
		++age_;
		for (unsigned int i = 0; i < displayed.size(); ++i) {
			if (displayed[i] == this) {
				MessagesALM::instance()->alm_row_changed(i);
			}
		}
		return true;
	} else {
		hide();
		return false;
	}
}

void Annunciator::message::hide() {
	for (unsigned int i = 0; i < displayed.size(); ++i) {
		if (displayed[i] == this) {
			MessagesALM::instance()->alm_row_deleted(i);
			displayed.erase(displayed.begin() + i);
			--i;
		}
	}
	displayed_ = false;
}

Annunciator::Annunciator() {
	SirenAvailabilityWarner::ensure_exists();
	Gtk::TreeView *view = Gtk::manage(new Gtk::TreeView(MessagesALM::instance()));
	view->get_selection()->set_mode(Gtk::SELECTION_SINGLE);
	view->append_column("Age", MessagesALM::instance()->age_column);
	Gtk::CellRendererText *message_renderer = Gtk::manage(new Gtk::CellRendererText);
	int message_colnum = view->append_column("Message", *message_renderer) - 1;
	Gtk::TreeViewColumn *message_column = view->get_column(message_colnum);
	message_column->set_cell_data_func(*message_renderer, &message_cell_data_func);
	add(*view);
	set_shadow_type(Gtk::SHADOW_IN);
	set_size_request(-1, 100);
}

Annunciator::~Annunciator() {
}

