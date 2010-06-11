#include "uicomponents/abstract_list_model.h"
#include "uicomponents/annunciator.h"
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
		Glib::spawn_async("", cmdline, Glib::SPAWN_SEARCH_PATH);
	}

	unsigned int next_id = 0;
	std::unordered_map<unsigned int, annunciator::message *> registered;
	std::vector<annunciator::message *> displayed;

	class siren_availability_warner : public noncopyable {
		public:
			static void ensure_exists() {
				static siren_availability_warner instance;
			}

		private:
			annunciator::message msg;

			siren_availability_warner() : msg("\"beep\" executable unavailable, no annunciator sirens") {
				if (!can_siren()) {
					msg.activate(true);
				}
			}
	};

	class messages_alm : public Glib::Object, public abstract_list_model, public noncopyable {
		public:
			Gtk::TreeModelColumn<unsigned int> age_column;
			Gtk::TreeModelColumn<Glib::ustring> message_column;
			Gtk::TreeModelColumn<bool> active_column;

			static Glib::RefPtr<messages_alm> instance() {
				static Glib::RefPtr<messages_alm> inst;
				if (!inst) {
					inst = Glib::RefPtr<messages_alm>(new messages_alm);
				}
				return inst;
			}

		private:
			messages_alm() : Glib::ObjectBase(typeid(messages_alm)) {
				alm_column_record.add(age_column);
				alm_column_record.add(message_column);
				alm_column_record.add(active_column);
			}

			~messages_alm() {
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

			friend class annunciator::message;
	};

	void message_cell_data_func(Gtk::CellRenderer *r, const Gtk::TreeModel::iterator &iter) {
		Gtk::CellRendererText *rt = dynamic_cast<Gtk::CellRendererText *>(r);
		rt->property_text() = iter->get_value(messages_alm::instance()->message_column);
		bool active = iter->get_value(messages_alm::instance()->active_column);
		rt->property_foreground() = active ? "white" : "black";
		rt->property_background() = active ? "red" : "white";
	}
}

annunciator::message::message(const Glib::ustring &text) : text(text), id(next_id++), active_(false), age_(0), displayed_(false) {
	registered[id] = this;
}

annunciator::message::~message() {
	hide();
	registered.erase(id);
}

void annunciator::message::activate(bool actv) {
	if (actv != active_) {
		active_ = actv;
		for (unsigned int i = 0; i < displayed.size(); ++i) {
			if (displayed[i] == this) {
				messages_alm::instance()->alm_row_changed(i);
			}
		}
		one_second_connection.disconnect();
		if (actv) {
			age_ = 0;
			if (!displayed_) {
				unsigned int index = displayed.size();
				displayed.push_back(this);
				messages_alm::instance()->alm_row_inserted(index);
				displayed_ = true;
			}
			siren();
		} else {
			one_second_connection = Glib::signal_timeout().connect_seconds(sigc::mem_fun(this, &annunciator::message::on_one_second), 1);
		}
	}
}

bool annunciator::message::on_one_second() {
	if (age_ < MAX_AGE) {
		++age_;
		for (unsigned int i = 0; i < displayed.size(); ++i) {
			if (displayed[i] == this) {
				messages_alm::instance()->alm_row_changed(i);
			}
		}
		return true;
	} else {
		hide();
		return false;
	}
}

void annunciator::message::hide() {
	for (unsigned int i = 0; i < displayed.size(); ++i) {
		if (displayed[i] == this) {
			messages_alm::instance()->alm_row_deleted(i);
			displayed.erase(displayed.begin() + i);
			--i;
		}
	}
	displayed_ = false;
}

annunciator::annunciator() {
	siren_availability_warner::ensure_exists();
	Gtk::TreeView *view = Gtk::manage(new Gtk::TreeView(messages_alm::instance()));
	view->get_selection()->set_mode(Gtk::SELECTION_SINGLE);
	view->append_column("Age", messages_alm::instance()->age_column);
	Gtk::CellRendererText *message_renderer = Gtk::manage(new Gtk::CellRendererText);
	int message_colnum = view->append_column("Message", *message_renderer) - 1;
	Gtk::TreeViewColumn *message_column = view->get_column(message_colnum);
	message_column->set_cell_data_func(*message_renderer, &message_cell_data_func);
	add(*view);
	set_shadow_type(Gtk::SHADOW_IN);
	set_size_request(-1, 100);
}

annunciator::~annunciator() {
}

