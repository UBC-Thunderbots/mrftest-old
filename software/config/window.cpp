#include "config/window.h"
#include "uicomponents/abstract_list_model.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <functional>
#include <iomanip>
#include <sstream>
#include <stdint.h>

namespace {
	class BotInfoDialog : public Gtk::Dialog {
		public:
			BotInfoDialog(Gtk::Window &parent, const Config::RobotSet &robots, const Config::RobotInfo *old) : Gtk::Dialog("Add Bot", parent, true), robots(robots), old(old), table(3, 2, false), yellow_button(colour_group, "Yellow"), blue_button(colour_group, "Blue") {
				unsigned int y = 0;

				address_entry.set_text("0000000000000000");
				address_entry.set_width_chars(16);
				address_entry.set_max_length(16);
				address_entry.set_activates_default();
				address_entry.set_tooltip_text("The 64-bit hexadecimal hardware address of the XBee radio module on this robot. For real robots, read the serial number from the XBee chip. For simulation, pick a random number (e.g. 1, 2, 3, 4, etc.).");
				address_entry.signal_changed().connect(sigc::mem_fun(this, &BotInfoDialog::update_enable));
				add_row(address_entry, "XBee Address:", y);

				pattern_index_spin.get_adjustment()->configure(0.0, 0.0, 127.0, 1.0, 10.0, 0.0);
				pattern_index_spin.set_digits(0);
				pattern_index_spin.set_activates_default();
				pattern_index_spin.set_tooltip_text("The offset into SSL-Vision's pattern image file of this robot's pattern. Also, the ID number of the robot as it appears in SSL-Vision's graphical client program.");
				pattern_index_spin.signal_value_changed().connect(sigc::mem_fun(this, &BotInfoDialog::update_enable));
				add_row(pattern_index_spin, "Lid Pattern Index:", y);

				name_entry.set_activates_default();
				name_entry.set_tooltip_text("A human-readable name for the robot. Does not affect the system.");
				name_entry.signal_changed().connect(sigc::mem_fun(this, &BotInfoDialog::update_enable));
				add_row(name_entry, "Name:", y);

				get_vbox()->pack_start(table, Gtk::PACK_EXPAND_WIDGET);

				get_vbox()->pack_start(error_label, Gtk::PACK_SHRINK);

				add_button(Gtk::Stock::OK, Gtk::RESPONSE_ACCEPT);
				add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
				set_default_response(Gtk::RESPONSE_ACCEPT);

				update_enable();

				if (old) {
					address_entry.set_text(tohex(old->address, 16));
					pattern_index_spin.set_value(old->pattern_index);
					name_entry.set_text(old->name);
				}

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
			const Config::RobotSet &robots;
			const Config::RobotInfo *old;

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
				uint64_t addr = address();
				if (!addr) {
					set_response_sensitive(Gtk::RESPONSE_ACCEPT, false);
					error_label.set_markup("<i>The XBee address is invalid</i>");
					return;
				}
				if ((!old || addr != old->address) && robots.contains_address(addr)) {
					set_response_sensitive(Gtk::RESPONSE_ACCEPT, false);
					error_label.set_markup("<i>The XBee address is already in use</i>");
					return;
				}
				if ((!old || pattern_index() != old->pattern_index) && robots.contains_pattern(pattern_index())) {
					set_response_sensitive(Gtk::RESPONSE_ACCEPT, false);
					error_label.set_markup("<i>The lid pattern is already in use</i>");
					return;
				}
				if (name().empty()) {
					set_response_sensitive(Gtk::RESPONSE_ACCEPT, false);
					error_label.set_markup("<i>A name must be provided</i>");
					return;
				}
				if ((!old || name() != old->name) && robots.contains_name(name())) {
					set_response_sensitive(Gtk::RESPONSE_ACCEPT, false);
					error_label.set_markup("<i>The name is already in use</i>");
					return;
				}
				set_response_sensitive(Gtk::RESPONSE_ACCEPT, true);
				error_label.set_text("");
			}
	};

	class RobotsModel : public Glib::Object, public AbstractListModel {
		public:
			Gtk::TreeModelColumn<uint64_t> address_column;
			Gtk::TreeModelColumn<unsigned int> pattern_index_column;
			Gtk::TreeModelColumn<Glib::ustring> name_column;

			static Glib::RefPtr<RobotsModel> create(const Config::RobotSet &robots) {
				Glib::RefPtr<RobotsModel> p(new RobotsModel(robots));
				return p;
			}

		private:
			const Config::RobotSet &robots;
			unsigned int size_;

			void alm_get_value(unsigned int row, unsigned int col, Glib::ValueBase &value) const {
				if (col == static_cast<unsigned int>(address_column.index())) {
					Glib::Value<uint64_t> v;
					v.init(address_column.type());
					v.set(robots[row].address);
					value.init(address_column.type());
					value = v;
				} else if (col == static_cast<unsigned int>(pattern_index_column.index())) {
					Glib::Value<unsigned int> v;
					v.init(pattern_index_column.type());
					v.set(robots[row].pattern_index);
					value.init(pattern_index_column.type());
					value = v;
				} else if (col == static_cast<unsigned int>(name_column.index())) {
					Glib::Value<Glib::ustring> v;
					v.init(name_column.type());
					v.set(robots[row].name);
					value.init(name_column.type());
					value = v;
				}
			}

			void alm_set_value(unsigned int, unsigned int, const Glib::ValueBase &) {
			}

			RobotsModel(const Config::RobotSet &robots) : Glib::ObjectBase(typeid(RobotsModel)), Glib::Object(), AbstractListModel(), robots(robots), size_(0) {
				alm_column_record.add(address_column);
				alm_column_record.add(pattern_index_column);
				alm_column_record.add(name_column);
				robots.signal_robot_added.connect(sigc::mem_fun(this, &RobotsModel::alm_row_inserted));
				robots.signal_robot_removed.connect(sigc::mem_fun(this, &RobotsModel::alm_row_deleted));
				robots.signal_robot_replaced.connect(sigc::mem_fun(this, &RobotsModel::alm_row_changed));
				robots.signal_sorted.connect(sigc::mem_fun(this, &RobotsModel::on_all_rows_changed));
			}

			unsigned int alm_rows() const {
				return robots.size();
			}

			void on_all_rows_changed() {
				for (unsigned int i = 0; i < robots.size(); ++i) {
					alm_row_changed(i);
				}
			}
	};

	class RobotsPage : public Gtk::VBox {
		public:
			RobotsPage(Config::RobotSet &robots) : robots(robots), model(RobotsModel::create(robots)), view(model), button_box(Gtk::BUTTONBOX_SPREAD), add_button(Gtk::Stock::ADD), edit_button(Gtk::Stock::EDIT), remove_button(Gtk::Stock::DELETE), sort_button("_Sort", true) {
				view.get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
				view.get_selection()->signal_changed().connect(sigc::mem_fun(this, &RobotsPage::selection_changed));
				view.append_column_numeric("Address", model->address_column, "%016llX");
				view.append_column("Pattern Index", model->pattern_index_column);
				view.append_column("Name", model->name_column);
				scroller.add(view);
				scroller.set_shadow_type(Gtk::SHADOW_IN);
				pack_start(scroller, Gtk::PACK_EXPAND_WIDGET);

				add_button.signal_clicked().connect(sigc::mem_fun(this, &RobotsPage::add));
				edit_button.signal_clicked().connect(sigc::mem_fun(this, &RobotsPage::edit));
				edit_button.set_sensitive(false);
				remove_button.signal_clicked().connect(sigc::mem_fun(this, &RobotsPage::remove));
				remove_button.set_sensitive(false);
				sort_button.signal_clicked().connect(sigc::mem_fun(this, &RobotsPage::sort));
				button_box.pack_start(add_button);
				button_box.pack_start(edit_button);
				button_box.pack_start(remove_button);
				button_box.pack_start(sort_button);
				pack_start(button_box, Gtk::PACK_SHRINK);
			}

		private:
			Config::RobotSet &robots;
			Glib::RefPtr<RobotsModel> model;
			Gtk::TreeView view;
			Gtk::ScrolledWindow scroller;
			Gtk::HButtonBox button_box;
			Gtk::Button add_button, edit_button, remove_button, sort_button;

			Gtk::Window &find_window() {
				Gtk::Container *parent = get_parent();
				Gtk::Window *window = 0;
				while (!(window = dynamic_cast<Gtk::Window *>(parent))) {
					parent = parent->get_parent();
				}
				return *window;
			}

			void selection_changed() {
				unsigned int nrows = view.get_selection()->count_selected_rows();
				edit_button.set_sensitive(nrows == 1);
				remove_button.set_sensitive(nrows > 0);
			}

			void add() {
				BotInfoDialog dlg(find_window(), robots, 0);
				if (dlg.run() == Gtk::RESPONSE_ACCEPT) {
					robots.add(dlg.address(), dlg.pattern_index(), dlg.name());
				}
			}

			void edit() {
				const Gtk::TreeSelection::ListHandle_Path &sel = view.get_selection()->get_selected_rows();
				if (sel.size() == 1) {
					const Gtk::TreePath &path = *sel.begin();
					if (path.size() == 1) {
						const Config::RobotInfo &old = robots[path[0]];
						BotInfoDialog dlg(find_window(), robots, &old);
						if (dlg.run() == Gtk::RESPONSE_ACCEPT) {
							robots.replace(old.address, dlg.address(), dlg.pattern_index(), dlg.name());
						}
					}
				}
			}

			void remove() {
				const Gtk::TreeSelection::ListHandle_Path &sel = view.get_selection()->get_selected_rows();
				std::vector<uint64_t> addresses;
				for (Gtk::TreeSelection::ListHandle_Path::const_iterator i = sel.begin(), iend = sel.end(); i != iend; ++i) {
					const Gtk::TreePath &path = *i;
					if (path.size() == 1) {
						addresses.push_back(robots[path[0]].address);
					}
				}
				std::for_each(addresses.begin(), addresses.end(), sigc::mem_fun(robots, &Config::RobotSet::remove));
			}

			void sort() {
				Gtk::Dialog dlg("Sort Robots", find_window(), true);
				Gtk::RadioButtonGroup grp;
				Gtk::RadioButton by_address_button(grp, "Sort by _Address", true);
				by_address_button.set_active(true);
				dlg.get_vbox()->pack_start(by_address_button, Gtk::PACK_SHRINK);
				Gtk::RadioButton by_lid_button(grp, "Sort by _Lid Pattern", true);
				dlg.get_vbox()->pack_start(by_lid_button, Gtk::PACK_SHRINK);
				Gtk::RadioButton by_name_button(grp, "Sort by _Name", true);
				dlg.get_vbox()->pack_start(by_name_button, Gtk::PACK_SHRINK);
				dlg.add_button(Gtk::Stock::OK, Gtk::RESPONSE_ACCEPT);
				dlg.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
				dlg.set_default_response(Gtk::RESPONSE_ACCEPT);
				dlg.show_all();
				int resp = dlg.run();
				if (resp == Gtk::RESPONSE_ACCEPT) {
					if (by_address_button.get_active()) {
						robots.sort_by_address();
					} else if (by_lid_button.get_active()) {
						robots.sort_by_lid();
					} else if (by_name_button.get_active()) {
						robots.sort_by_name();
					}
				}
			}
	};

	class ChannelsModel : public Glib::Object, public AbstractListModel {
		public:
			Gtk::TreeModelColumn<unsigned int> channel_column;

			ChannelsModel() : Glib::ObjectBase(typeid(ChannelsModel)) {
				alm_column_record.add(channel_column);
			}

			iterator iter_for_channel(unsigned int chan) {
				assert(MIN_CHANNEL <= chan && chan <= MAX_CHANNEL);
				Path p;
				p.push_back(chan - MIN_CHANNEL);
				return get_iter(p);
			}

			unsigned int channel_for_iter(const iterator &iter) {
				const Path &p(get_path(iter));
				return p.front() + MIN_CHANNEL;
			}

		private:
			static const unsigned int MIN_CHANNEL = 0x0B;
			static const unsigned int MAX_CHANNEL = 0x1A;

			unsigned int alm_rows() const {
				return MAX_CHANNEL - MIN_CHANNEL + 1;
			}

			void alm_get_value(unsigned int row, unsigned int col, Glib::ValueBase &value) const {
				if (col == static_cast<unsigned int>(channel_column.index())) {
					Glib::Value<unsigned int> v;
					v.init(channel_column.type());
					v.set(row + MIN_CHANNEL);
					value.init(channel_column.type());
					value = v;
				} else {
					std::abort();
				}
			}

			void alm_set_value(unsigned int, unsigned int, const Glib::ValueBase &) {
			}
	};

	class RadioPage : public Gtk::Table {
		public:
			RadioPage(Config &conf) : Gtk::Table(1, 2), conf(conf), model(new ChannelsModel), view(model) {
				view.pack_start(renderer);
				view.set_cell_data_func(renderer, sigc::mem_fun(this, &RadioPage::cell_data_func));
				view.set_active(model->iter_for_channel(conf.channel()));
				view.signal_changed().connect(sigc::mem_fun(this, &RadioPage::on_changed));
				attach(*Gtk::manage(new Gtk::Label("Channel:")), 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				attach(view, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
			}

		private:
			Config &conf;
			Glib::RefPtr<ChannelsModel> model;
			Gtk::ComboBox view;
			Gtk::CellRendererText renderer;

			void on_changed() {
				conf.channel(model->channel_for_iter(view.get_active()));
			}

			void cell_data_func(const Gtk::TreeModel::const_iterator &iter) {
				unsigned int chan = model->channel_for_iter(iter);
				renderer.property_text() = tohex(chan, 2);
			}
	};
}

Window::Window(Config &conf) : conf(conf) {
	set_title("Thunderbots Configuration");
	Gtk::Notebook *notebook = Gtk::manage(new Gtk::Notebook);
	add(*notebook);

	notebook->append_page(*Gtk::manage(new RobotsPage(conf.robots())), "Robots");
	notebook->append_page(*Gtk::manage(new RadioPage(conf)), "Radio");

	set_default_size(400, 400);
}

bool Window::on_delete_event(GdkEventAny *) {
	conf.save();
	return false;
}

