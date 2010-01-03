#ifndef UICOMPONENTS_BOT_CHOOSER_H
#define UICOMPONENTS_BOT_CHOOSEr_H

#include <stdint.h>
#include <libxml++/libxml++.h>
#include <gtkmm.h>
	
class bot_chooser : public Gtk::VBox {
	public:
		bot_chooser(xmlpp::Element *xmlworld, Gtk::Window &window);

		~bot_chooser();

		sigc::signal<void, uint64_t> &signal_address_changed();

		uint64_t address() const;

	private:
		class chooser_combo : public Gtk::ComboBoxText {
			private:
				class item {
					public:
						item(const Glib::ustring &name, uint64_t address, xmlpp::Element *xml);

						~item();

						Glib::ustring format() const;

						Glib::ustring name;
						uint64_t address;
						xmlpp::Element *xml;
				};

			public:
				chooser_combo(xmlpp::Element *xmlplayers, Gtk::Window &window);

				~chooser_combo();

				sigc::signal<void, uint64_t> &signal_address_changed();

				void add_player();

				void del_player();

				uint64_t address() const;

			protected:
				void on_changed();

			private:
				xmlpp::Element *xmlplayers;
				Gtk::Window &window;
				std::vector<item> items;
				sigc::signal<void, uint64_t> sig_address_changed;
		};

		xmlpp::Element *xmlplayers;

		chooser_combo chooser;

		Gtk::HButtonBox button_box;
		Gtk::Button add_button, del_button;
};

#endif

