#include "configeditor/window.h"
#include "util/abstract_list_model.h"
#include "util/algorithm.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <functional>
#include <sstream>
#include <stdint.h>

namespace {
	class add_bot_dialog : public Gtk::Dialog {
		public:
			add_bot_dialog(Gtk::Window &parent, const config::robot_set &robots) : Gtk::Dialog("Add Bot", parent, true), robots(robots), table(4, 2, false), yellow_button(colour_group, "Yellow"), blue_button(colour_group, "Blue") {
				unsigned int y = 0;

				address_entry.set_text("0000000000000000");
				address_entry.set_width_chars(16);
				address_entry.set_max_length(16);
				address_entry.set_activates_default();
				address_entry.set_tooltip_text("The 64-bit hexadecimal hardware address of the XBee radio module on this robot. For real robots, read the serial number from the XBee chip. For simulation, pick a random number (e.g. 1, 2, 3, 4, etc.).");
				address_entry.signal_changed().connect(sigc::mem_fun(this, &add_bot_dialog::update_enable));
				add_row(address_entry, "XBee Address:", y);

				yellow_button.set_tooltip_text("Sets the colour of the central dot on the lid to be yellow. This does not necessarily have anything to do with which team the robot plays for.");
				yellow_button.signal_toggled().connect(sigc::mem_fun(this, &add_bot_dialog::update_enable));
				colour_hbox.pack_start(yellow_button, Gtk::PACK_EXPAND_WIDGET);
				blue_button.set_tooltip_text("Sets the colour of the central dot on the lid to be blue. This does not necessarily have anything to do with which team the robot plays for.");
				blue_button.signal_toggled().connect(sigc::mem_fun(this, &add_bot_dialog::update_enable));
				colour_hbox.pack_start(blue_button, Gtk::PACK_EXPAND_WIDGET);
				add_row(colour_hbox, "Lid Colour:", y);

				pattern_index_spin.get_adjustment()->configure(0.0, 0.0, 127.0, 1.0, 10.0, 0.0);
				pattern_index_spin.set_digits(0);
				pattern_index_spin.set_activates_default();
				pattern_index_spin.set_tooltip_text("The offset into SSL-Vision's pattern image file of this robot's pattern. Also, the ID number of the robot as it appears in SSL-Vision's graphical client program.");
				pattern_index_spin.signal_value_changed().connect(sigc::mem_fun(this, &add_bot_dialog::update_enable));
				add_row(pattern_index_spin, "Lid Pattern Index:", y);

				name_entry.set_activates_default();
				name_entry.set_tooltip_text("A human-readable name for the robot. Does not affect the system.");
				add_row(name_entry, "Name:", y);

				get_vbox()->pack_start(table, Gtk::PACK_EXPAND_WIDGET);

				get_vbox()->pack_start(error_label, Gtk::PACK_SHRINK);

				add_button(Gtk::Stock::OK, Gtk::RESPONSE_ACCEPT);
				add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
				set_default_response(Gtk::RESPONSE_ACCEPT);

				update_enable();

				show_all();
			}

			uint64_t address() const {
				std::istringstream iss(address_entry.get_text());
				iss.setf(std::ios_base::hex, std::ios_base::basefield);
				uint64_t addr;
				iss >> addr;
				return addr;
			}

			bool yellow() const {
				return yellow_button.get_active();
			}

			unsigned int pattern_index() const {
				return pattern_index_spin.get_value_as_int();
			}

			Glib::ustring name() const {
				return name_entry.get_text();
			}

		private:
			const config::robot_set &robots;

			Gtk::Table table;
			Gtk::Entry address_entry;
			Gtk::HBox colour_hbox;
			Gtk::RadioButton::Group colour_group;
			Gtk::RadioButton yellow_button, blue_button;
			Gtk::SpinButton pattern_index_spin;
			Gtk::Entry name_entry;
			Gtk::Label error_label;

			void add_row(Gtk::Widget &widget, const Glib::ustring &label, unsigned int &y) {
				table.attach(*Gtk::manage(new Gtk::Label(label)), 0, 1, y, y + 1, Gtk::SHRINK | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);
				table.attach(widget, 1, 2, y, y + 1, Gtk::EXPAND | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);
				++y;
			}

			void update_enable() {
				const Glib::ustring &address_string(address_entry.get_text());
				if (address_string.size() < 1 || address_string.size() > 16) {
					set_response_sensitive(Gtk::RESPONSE_ACCEPT, false);
					error_label.set_markup("<i>The XBee address is invalid</i>");
					return;
				}
				if (exists_if(address_string.begin(), address_string.end(), std::not1(std::ptr_fun<int, int>(&std::isxdigit)))) {
					set_response_sensitive(Gtk::RESPONSE_ACCEPT, false);
					error_label.set_markup("<i>The XBee address is invalid</i>");
					return;
				}
				if (address_string.size() != 16) {
					set_response_sensitive(Gtk::RESPONSE_ACCEPT, false);
					error_label.set_markup("<i>The XBee address is invalid</i>");
					return;
				}
				if (!address()) {
					set_response_sensitive(Gtk::RESPONSE_ACCEPT, false);
					error_label.set_markup("<i>The XBee address is invalid</i>");
					return;
				}
				if (robots.contains_address(address())) {
					set_response_sensitive(Gtk::RESPONSE_ACCEPT, false);
					error_label.set_markup("<i>The XBee address is already in use</i>");
					return;
				}
				if (robots.contains_pattern(yellow(), pattern_index())) {
					set_response_sensitive(Gtk::RESPONSE_ACCEPT, false);
					error_label.set_markup("<i>The lid pattern is already in use</i>");
					return;
				}
				set_response_sensitive(Gtk::RESPONSE_ACCEPT, true);
				error_label.set_text("");
			}
	};

	class robots_model : public Glib::Object, public abstract_list_model {
		public:
			Gtk::TreeModelColumn<uint64_t> address_column;
			Gtk::TreeModelColumn<bool> yellow_column;
			Gtk::TreeModelColumn<unsigned int> pattern_index_column;
			Gtk::TreeModelColumn<Glib::ustring> name_column;

			static Glib::RefPtr<robots_model> create(const config::robot_set &robots) {
				Glib::RefPtr<robots_model> p(new robots_model(robots));
				return p;
			}

		private:
			const config::robot_set &robots;
			unsigned int size_;

			void alm_get_value(unsigned int row, unsigned int col, Glib::ValueBase &value) const {
				if (col == 0) {
					Glib::Value<uint64_t> v;
					v.init(address_column.type());
					v.set(robots[row].address);
					value.init(address_column.type());
					value = v;
				} else if (col == 1) {
					Glib::Value<bool> v;
					v.init(yellow_column.type());
					v.set(robots[row].yellow);
					value.init(yellow_column.type());
					value = v;
				} else if (col == 2) {
					Glib::Value<unsigned int> v;
					v.init(pattern_index_column.type());
					v.set(robots[row].pattern_index);
					value.init(pattern_index_column.type());
					value = v;
				} else if (col == 3) {
					Glib::Value<Glib::ustring> v;
					v.init(name_column.type());
					v.set(robots[row].name);
					value.init(name_column.type());
					value = v;
				}
			}

			void alm_set_value(unsigned int, unsigned int, const Glib::ValueBase &) {
			}

			robots_model(const config::robot_set &robots) : Glib::ObjectBase(typeid(robots_model)), Glib::Object(), abstract_list_model(), robots(robots), size_(0) {
				alm_column_record.add(address_column);
				alm_column_record.add(yellow_column);
				alm_column_record.add(pattern_index_column);
				alm_column_record.add(name_column);
				robots.signal_robot_added.connect(sigc::mem_fun(this, &robots_model::alm_row_inserted));
				robots.signal_robot_removed.connect(sigc::mem_fun(this, &robots_model::alm_row_deleted));
			}

			unsigned int alm_rows() const {
				return robots.size();
			}
	};

	class robots_page : public Gtk::VBox {
		public:
			robots_page(config::robot_set &robots) : robots(robots), model(robots_model::create(robots)), view(model), button_box(Gtk::BUTTONBOX_SPREAD), add_button(Gtk::Stock::ADD), remove_button(Gtk::Stock::DELETE) {
				view.get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
				view.get_selection()->signal_changed().connect(sigc::mem_fun(this, &robots_page::selection_changed));
				view.append_column_numeric("Address", model->address_column, "%016llX");
				view.append_column("Yellow?", model->yellow_column);
				view.append_column("Pattern Index", model->pattern_index_column);
				view.append_column("Name", model->name_column);
				scroller.add(view);
				scroller.set_shadow_type(Gtk::SHADOW_IN);
				pack_start(scroller, Gtk::PACK_EXPAND_WIDGET);

				add_button.signal_clicked().connect(sigc::mem_fun(this, &robots_page::add));
				remove_button.signal_clicked().connect(sigc::mem_fun(this, &robots_page::remove));
				remove_button.set_sensitive(false);
				button_box.pack_start(add_button);
				button_box.pack_start(remove_button);
				pack_start(button_box, Gtk::PACK_SHRINK);
			}

		private:
			config::robot_set &robots;
			Glib::RefPtr<robots_model> model;
			Gtk::TreeView view;
			Gtk::ScrolledWindow scroller;
			Gtk::HButtonBox button_box;
			Gtk::Button add_button, remove_button;

			void selection_changed() {
				remove_button.set_sensitive(view.get_selection()->count_selected_rows() > 0);
			}

			void add() {
				Gtk::Container *parent = get_parent();
				Gtk::Window *window = 0;
				while (!(window = dynamic_cast<Gtk::Window *>(parent))) {
					parent = parent->get_parent();
				}
				add_bot_dialog dlg(*window, robots);
				if (dlg.run() == Gtk::RESPONSE_ACCEPT) {
					robots.add(dlg.address(), dlg.yellow(), dlg.pattern_index(), dlg.name());
				}
			}

			void remove() {
				const Gtk::TreeSelection::ListHandle_Path &sel = view.get_selection()->get_selected_rows();
				std::vector<uint64_t> addresses;
				for (typeof(sel.begin()) i = sel.begin(), iend = sel.end(); i != iend; ++i) {
					const Gtk::TreePath &path = *i;
					if (path.size() == 1) {
						addresses.push_back(robots[path[0]].address);
					}
				}
				std::for_each(addresses.begin(), addresses.end(), sigc::mem_fun(robots, &config::robot_set::remove));
			}
	};
}

window::window(config &conf) : conf(conf) {
	set_title("Thunderbots Configuration");
	Gtk::Notebook *notebook = Gtk::manage(new Gtk::Notebook);
	add(*notebook);

	notebook->append_page(*Gtk::manage(new robots_page(conf.robots())), "Robots");
}

bool window::on_delete_event(GdkEventAny *) {
	conf.save();
	return false;
}

