#include "firmware/upload.h"
#include "firmware/window.h"
#include "uicomponents/world_add_bot_dialog.h"
#include "util/xml.h"
#include "world/config.h"
#include <fstream>
#include <iomanip>

namespace {
	const uint8_t CMD_CHIP_ERASE = 0x29;
	const uint8_t CMD_WRITE1     = 0x48;
	const uint8_t CMD_WRITE2     = 0x67;
	const uint8_t CMD_WRITE3     = 0x86;
	const uint8_t CMD_SUM_PAGES  = 0xA5;
	const uint8_t CMD_GET_STATUS = 0xC4;
	const uint8_t CMD_IDENT      = 0xE3;

	class bot_chooser : public Gtk::ComboBoxText {
		private:
			class item {
				public:
					item(const Glib::ustring &name, uint64_t address, xmlpp::Element *xml) : name(name), address(address), xml(xml) {
					}

					Glib::ustring format() const {
						const Glib::ustring &address_string = Glib::ustring::format(std::setfill(L'0'), std::setw(16), std::hex, address);
						return Glib::ustring::compose("%1 [%2]", name, address_string);
					}

					Glib::ustring name;
					uint64_t address;
					xmlpp::Element *xml;
			};

		public:
			bot_chooser(xmlpp::Element *xmlplayers, Gtk::Window &window) : xmlplayers(xmlplayers), window(window) {
				const xmlpp::Node::NodeList &children = xmlplayers->get_children("player");
				for (xmlpp::Node::NodeList::const_iterator i = children.begin(), iend = children.end(); i != iend; ++i) {
					xmlpp::Node *node = *i;
					xmlpp::Element *elem = dynamic_cast<xmlpp::Element *>(node);
					if (elem) {
						const Glib::ustring &name = elem->get_attribute_value("name");
						const Glib::ustring &address_string = elem->get_attribute_value("address");
						uint64_t address;
						{
							std::istringstream iss(Glib::locale_from_utf8(address_string));
							iss.setf(std::ios_base::hex, std::ios_base::basefield);
							iss >> address;
						}
						item itm(name, address, elem);
						items.push_back(itm);
						append_text(itm.format());
					}
				}
			}

			sigc::signal<void, uint64_t> &signal_address_changed() {
				return sig_address_changed;
			}

			void add_player() {
				world_add_bot_dialog dlg(window);
				if (dlg.run() == Gtk::RESPONSE_ACCEPT) {
					xmlpp::Element *elem = xmlplayers->add_child("player");
					elem->set_attribute("name", dlg.name());
					elem->set_attribute("address", Glib::ustring::format(std::hex, dlg.address()));
					elem->set_attribute("colour", dlg.is_yellow() ? "yellow" : "blue");
					item itm(dlg.name(), dlg.address(), elem);
					items.push_back(itm);
					append_text(itm.format());
					config::dirty();
				}
			}

			void del_player() {
				int cur = get_active_row_number();
				if (cur >= 0) {
					item itm = items[cur];
					items.erase(items.begin() + cur);
					xmlplayers->remove_child(itm.xml);
					remove_text(itm.format());
				}
			}

		protected:
			void on_changed() {
				int cur = get_active_row_number();
				if (cur >= 0) {
					sig_address_changed.emit(items[cur].address);
				} else {
					sig_address_changed.emit(0);
				}
			}

		private:
			xmlpp::Element *xmlplayers;
			Gtk::Window &window;
			std::vector<item> items;
			sigc::signal<void, uint64_t> sig_address_changed;
	};

	class bot_panel : public Gtk::VBox {
		public:
			bot_panel(xmlpp::Element *xmlworld, Gtk::Window &window) : xmlplayers(xmlutil::get_only_child(xmlworld, "players")), chooser(xmlplayers, window), button_box(Gtk::BUTTONBOX_SPREAD), add_button(Gtk::Stock::ADD), del_button(Gtk::Stock::DELETE) {
				pack_start(chooser, false, false);

				add_button.signal_clicked().connect(sigc::mem_fun(chooser, &bot_chooser::add_player));
				button_box.pack_start(add_button);
				del_button.signal_clicked().connect(sigc::mem_fun(chooser, &bot_chooser::del_player));
				button_box.pack_start(del_button);
				pack_start(button_box, false, false);
			}

			sigc::signal<void, uint64_t> &signal_address_changed() {
				return chooser.signal_address_changed();
			}

		private:
			xmlpp::Element *xmlplayers;

			bot_chooser chooser;

			Gtk::HButtonBox button_box;
			Gtk::Button add_button, del_button;
	};

	class upload_dialog : public Gtk::Dialog {
		public:
			upload_dialog(Gtk::Window &win, upload &up) : Gtk::Dialog("Upload Progress", win, true), up(up) {
				up.signal_progress_made().connect(sigc::mem_fun(*this, &upload_dialog::status_update));
				up.signal_upload_finished().connect(sigc::bind(sigc::mem_fun(static_cast<Gtk::Dialog &>(*this), &Gtk::Dialog::response), Gtk::RESPONSE_ACCEPT));
				pb.set_text(up.get_status());
				get_vbox()->pack_start(pb, false, false);
				add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
				show_all();
			}

		private:
			upload &up;
			Gtk::ProgressBar pb;

			void status_update(double fraction) {
				pb.set_fraction(fraction);
				pb.set_text(up.get_status());
			}
	};
}

class firmware_window_impl : public Gtk::Window {
	public:
		firmware_window_impl(xbee &modem, xmlpp::Element *xmlworld) : modem(modem), bot_frame("Bot"), bot_controls(xmlworld, *this), file_frame("Firmware File"), work_frame("Execute Upload"), start_upload_button(Gtk::Stock::EXECUTE) {
			bot_controls.signal_address_changed().connect(sigc::mem_fun(*this, &firmware_window_impl::address_changed));
			bot_frame.add(bot_controls);
			vbox.pack_start(bot_frame, false, false);

			file_frame.add(file_chooser);
			vbox.pack_start(file_frame, false, false);

			start_upload_button.signal_clicked().connect(sigc::mem_fun(*this, &firmware_window_impl::start_upload));
			work_box.pack_start(start_upload_button, false, false);
			work_box.pack_start(work_progress, false, false);
			work_frame.add(work_box);
			vbox.pack_start(work_frame, false, false);

			add(vbox);

			show_all();
		}

	protected:
		bool on_delete_event(GdkEventAny *) {
			Gtk::Main::quit();
			return true;
		}

	private:
		xbee &modem;

		uint64_t current_address;

		Gtk::VBox vbox;

		Gtk::Frame bot_frame;
		bot_panel bot_controls;

		Gtk::Frame file_frame;
		Gtk::FileChooserButton file_chooser;

		Gtk::Frame work_frame;
		Gtk::VBox work_box;
		Gtk::Button start_upload_button;
		Gtk::ProgressBar work_progress;

		void address_changed(uint64_t address) {
			current_address = address;
			start_upload_button.set_sensitive(!!address);
		}

		void start_upload() {
			const Glib::ustring &filename = file_chooser.get_filename();
			std::ifstream ifs;
			ifs.exceptions(std::ios_base::badbit);
			ifs.open(Glib::locale_from_utf8(filename).c_str(), std::ios_base::in | std::ios_base::binary);
			std::vector<std::vector<uint8_t> > pages;
			while (ifs) {
				uint8_t buffer[256];
				ifs.read(reinterpret_cast<char *>(buffer), sizeof(buffer));
				if (ifs.gcount()) {
					std::fill(&buffer[ifs.gcount()], &buffer[sizeof(buffer)], 0xFF);
					pages.push_back(std::vector<uint8_t>(&buffer[0], &buffer[sizeof(buffer)]));
				}
			}
			ifs.close();

			upload up(modem, current_address, pages);
			upload_dialog dlg(*this, up);
			dlg.run();
		}
};

firmware_window::firmware_window(xbee &modem, xmlpp::Element *xmlworld) : impl(new firmware_window_impl(modem, xmlworld)) {
}

firmware_window::~firmware_window() {
	delete impl;
}

