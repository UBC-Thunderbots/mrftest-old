#include "uicomponents/annunciator.h"
#include "uicomponents/abstract_list_model.h"
#include "util/annunciator.h"
#include <glibmm/object.h>
#include <glibmm/refptr.h>

namespace {
	class MessagesALM final : public Glib::Object, public AbstractListModel, public NonCopyable {
		public:
			Gtk::TreeModelColumn<unsigned int> age_column;
			Gtk::TreeModelColumn<Glib::ustring> message_column;
			Gtk::TreeModelColumn<bool> active_column;
			Gtk::TreeModelColumn<Annunciator::Message::Severity> severity_column;

			static Glib::RefPtr<MessagesALM> instance() {
				static Glib::RefPtr<MessagesALM> inst;
				if (!inst) {
					inst = Glib::RefPtr<MessagesALM>(new MessagesALM);
				}
				return inst;
			}

		private:
			explicit MessagesALM() : Glib::ObjectBase(typeid(MessagesALM)) {
				alm_column_record.add(age_column);
				alm_column_record.add(message_column);
				alm_column_record.add(active_column);
				alm_column_record.add(severity_column);

				Annunciator::signal_message_activated.connect(sigc::mem_fun(this, &MessagesALM::on_message_activated));
				Annunciator::signal_message_deactivated.connect(sigc::mem_fun(this, &MessagesALM::on_message_deactivated));
				Annunciator::signal_message_aging.connect(sigc::mem_fun(this, &MessagesALM::on_message_aging));
				Annunciator::signal_message_reactivated.connect(sigc::mem_fun(this, &MessagesALM::on_message_reactivated));
				Annunciator::signal_message_hidden.connect(sigc::mem_fun(this, &MessagesALM::on_message_hidden));
			}

			std::size_t alm_rows() const override {
				return Annunciator::visible().size();
			}

			void alm_get_value(std::size_t row, unsigned int col, Glib::ValueBase &value) const override {
				if (col == static_cast<unsigned int>(age_column.index())) {
					Glib::Value<unsigned int> v;
					v.init(age_column.type());
					v.set(Annunciator::visible()[row]->age());
					value.init(age_column.type());
					value = v;
				} else if (col == static_cast<unsigned int>(message_column.index())) {
					Glib::Value<Glib::ustring> v;
					v.init(message_column.type());
					v.set(Annunciator::visible()[row]->text);
					value.init(message_column.type());
					value = v;
				} else if (col == static_cast<unsigned int>(active_column.index())) {
					Glib::Value<bool> v;
					v.init(active_column.type());
					v.set(Annunciator::visible()[row]->active());
					value.init(active_column.type());
					value = v;
				} else if (col == static_cast<unsigned int>(severity_column.index())) {
					Glib::Value<Annunciator::Message::Severity> v;
					v.init(severity_column.type());
					v.set(Annunciator::visible()[row]->severity);
					value.init(severity_column.type());
					value = v;
				} else {
					std::abort();
				}
			}

			void alm_set_value(std::size_t, unsigned int, const Glib::ValueBase &) override {
			}

			void on_message_activated() {
				alm_row_inserted(Annunciator::visible().size() - 1);
			}

			void on_message_deactivated(std::size_t i) {
				alm_row_changed(i);
			}

			void on_message_aging(std::size_t i) {
				alm_row_changed(i);
			}

			void on_message_reactivated(std::size_t i) {
				alm_row_changed(i);
			}

			void on_message_hidden(std::size_t i) {
				alm_row_deleted(i);
			}
	};

	void message_cell_data_func(Gtk::CellRenderer *r, const Gtk::TreeModel::iterator &iter) {
		Gtk::CellRendererText *rt = dynamic_cast<Gtk::CellRendererText *>(r);
		rt->property_text() = iter->get_value(MessagesALM::instance()->message_column);
		bool active = iter->get_value(MessagesALM::instance()->active_column);
		Annunciator::Message::Severity severity = iter->get_value(MessagesALM::instance()->severity_column);
		rt->property_foreground() = active && severity == Annunciator::Message::Severity::HIGH ? u8"white" : u8"black";
		rt->property_background() = active ? (severity == Annunciator::Message::Severity::HIGH ? u8"red" : u8"yellow") : u8"white";
	}
}

GUIAnnunciator::GUIAnnunciator() : view(MessagesALM::instance()) {
	view.get_selection()->set_mode(Gtk::SELECTION_SINGLE);
	view.append_column(u8"Age", MessagesALM::instance()->age_column);
	int message_colnum = view.append_column(u8"Message", message_renderer) - 1;
	Gtk::TreeViewColumn *message_column = view.get_column(message_colnum);
	message_column->set_cell_data_func(message_renderer, &message_cell_data_func);
	add(view);
	set_shadow_type(Gtk::SHADOW_IN);
	set_size_request(-1, 100);
}

