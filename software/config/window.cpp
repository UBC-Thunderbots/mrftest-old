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

			std::size_t alm_rows() const {
				return MAX_CHANNEL - MIN_CHANNEL + 1;
			}

			void alm_get_value(std::size_t row, unsigned int col, Glib::ValueBase &value) const {
				if (col == static_cast<unsigned int>(channel_column.index())) {
					Glib::Value<unsigned int> v;
					v.init(channel_column.type());
					v.set(static_cast<unsigned int>(row + MIN_CHANNEL));
					value.init(channel_column.type());
					value = v;
				} else {
					std::abort();
				}
			}

			void alm_set_value(std::size_t, unsigned int, const Glib::ValueBase &) {
			}
	};

	const unsigned int ChannelsModel::MIN_CHANNEL;

	const unsigned int ChannelsModel::MAX_CHANNEL;

	class RadioPage : public Gtk::Table {
		public:
			RadioPage(Config &conf) : Gtk::Table(2, 2), conf(conf), model(new ChannelsModel), out_view(model), in_view(model) {
				out_view.pack_start(out_renderer);
				out_view.set_cell_data_func(out_renderer, sigc::bind(sigc::mem_fun(this, &RadioPage::cell_data_func), sigc::ref(out_renderer)));
				out_view.set_active(model->iter_for_channel(conf.out_channel()));
				out_view.signal_changed().connect(sigc::mem_fun(this, &RadioPage::on_changed));
				attach(*Gtk::manage(new Gtk::Label("Out Channel:")), 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				attach(out_view, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);

				in_view.pack_start(in_renderer);
				in_view.set_cell_data_func(in_renderer, sigc::bind(sigc::mem_fun(this, &RadioPage::cell_data_func), sigc::ref(in_renderer)));
				in_view.set_active(model->iter_for_channel(conf.in_channel()));
				in_view.signal_changed().connect(sigc::mem_fun(this, &RadioPage::on_changed));
				attach(*Gtk::manage(new Gtk::Label("In Channel:")), 0, 1, 1, 2, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				attach(in_view, 1, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
			}

		private:
			Config &conf;
			Glib::RefPtr<ChannelsModel> model;
			Gtk::ComboBox out_view, in_view;
			Gtk::CellRendererText out_renderer, in_renderer;

			void on_changed() {
				conf.channels(model->channel_for_iter(out_view.get_active()), model->channel_for_iter(in_view.get_active()));
			}

			void cell_data_func(const Gtk::TreeModel::const_iterator &iter, Gtk::CellRendererText &renderer) {
				unsigned int chan = model->channel_for_iter(iter);
				renderer.property_text() = tohex(chan, 2);
			}
	};
}

Window::Window(Config &conf) : conf(conf) {
	set_title("Thunderbots Configuration");
	Gtk::Notebook *notebook = Gtk::manage(new Gtk::Notebook);
	add(*notebook);

	notebook->append_page(*Gtk::manage(new RadioPage(conf)), "Radio");

	set_default_size(400, 400);
}

bool Window::on_delete_event(GdkEventAny *) {
	conf.save();
	return false;
}

