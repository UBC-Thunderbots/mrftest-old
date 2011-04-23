#include "ai/param_panel.h"
#include "util/config.h"
#include "util/exception.h"
#include "util/param.h"
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <iomanip>

namespace {
	bool emit_numeric_row_changed = false;

	class ParamTreeModel : public Glib::Object, public Gtk::TreeModel {
		public:
			Gtk::TreeModelColumn<Glib::ustring> name_column;
			Gtk::TreeModelColumn<bool> has_bool_column;
			Gtk::TreeModelColumn<bool> bool_column;
			Gtk::TreeModelColumn<bool> has_numeric_column;
			Gtk::TreeModelColumn<Gtk::Adjustment *> numeric_adjustment_column;
			Gtk::TreeModelColumn<Glib::ustring> numeric_value_column;
			Gtk::TreeModelColumn<unsigned int> numeric_digits_column;
			Gtk::TreeModelColumnRecord column_record;

			static Glib::RefPtr<ParamTreeModel> create() {
				Glib::RefPtr<ParamTreeModel> p(new ParamTreeModel);
				return p;
			}

			void toggle_bool(const iterator &iter) {
				assert(iter_is_valid(iter));
				ParamTreeNode *node = static_cast<ParamTreeNode *>(iter.gobj()->user_data);
				BoolParam *bp = dynamic_cast<BoolParam *>(node);
				if (bp) {
					bp->prop() = !*bp;
				}
			}

		private:
			static const int STAMP = 123498723;

			static bool make_iter(ParamTreeNode *node, iterator &iter) {
				if (node) {
					iter.gobj()->stamp = STAMP;
					iter.gobj()->user_data = node;
					return true;
				} else {
					iter.gobj()->stamp = 0;
					iter.gobj()->user_data = 0;
					return false;
				}
			}

			ParamTreeModel() : Glib::ObjectBase(typeid(this)) {
				column_record.add(name_column);
				column_record.add(has_bool_column);
				column_record.add(bool_column);
				column_record.add(has_numeric_column);
				column_record.add(numeric_adjustment_column);
				column_record.add(numeric_value_column);
				column_record.add(numeric_digits_column);

				attach_change_signals(ParamTreeNode::root());
			}

			void attach_change_signals(ParamTreeNode *node) {
				const Param *param = dynamic_cast<const Param *>(node);
				if (param) {
					param->signal_changed().connect(sigc::bind(sigc::mem_fun(this, &ParamTreeModel::on_param_changed), node));
				}
				for (std::size_t i = 0; i < node->num_children(); ++i) {
					attach_change_signals(node->child(i));
				}
			}

			void on_param_changed(ParamTreeNode *p) {
				if (emit_numeric_row_changed || !dynamic_cast<NumericParam *>(p)) {
					Gtk::TreeIter iter;
					make_iter(p, iter);
					Gtk::TreePath path = get_path(iter);
					row_changed(path, iter);
				}
			}

			Gtk::TreeModelFlags get_flags_vfunc() const {
				return Gtk::TREE_MODEL_ITERS_PERSIST;
			}

			int get_n_columns_vfunc() const {
				return column_record.size();
			}

			GType get_column_type_vfunc(int index) const {
				assert(0 <= index && index < get_n_columns_vfunc());
				return column_record.types()[index];
			}

			bool iter_next_vfunc(const iterator &iter, iterator &iter_next) const {
				assert(iter_is_valid(iter));
				ParamTreeNode *node = static_cast<ParamTreeNode *>(iter.gobj()->user_data);
				return make_iter(node->next_sibling(), iter_next);
			}

			bool get_iter_vfunc(const Gtk::TreePath &path, iterator &iter) const {
				ParamTreeNode *node = ParamTreeNode::root();
				for (auto i = path.begin(), iend = path.end(); node && i != iend; ++i) {
					node = node->child(*i);
				}
				return make_iter(node, iter);
			}

			bool iter_children_vfunc(const iterator &parent, iterator &iter) const {
				assert(iter_is_valid(parent));
				ParamTreeNode *node = static_cast<ParamTreeNode *>(parent.gobj()->user_data);
				return make_iter(node->child(0), iter);
			}

			bool iter_parent_vfunc(const iterator &child, iterator &iter) const {
				assert(iter_is_valid(child));
				ParamTreeNode *node = static_cast<ParamTreeNode *>(child.gobj()->user_data);
				return make_iter(node->parent(), iter);
			}

			bool iter_nth_child_vfunc(const iterator &parent, int n, iterator &iter) const {
				assert(iter_is_valid(parent));
				ParamTreeNode *node = static_cast<ParamTreeNode *>(parent.gobj()->user_data);
				return make_iter(node->child(n), iter);
			}

			bool iter_nth_root_child_vfunc(int n, iterator &iter) const {
				return make_iter(ParamTreeNode::root()->child(n), iter);
			}

			bool iter_has_child_vfunc(const iterator &iter) const {
				assert(iter_is_valid(iter));
				ParamTreeNode *node = static_cast<ParamTreeNode *>(iter.gobj()->user_data);
				return node->num_children() > 0;
			}

			int iter_n_children_vfunc(const iterator &iter) const {
				assert(iter_is_valid(iter));
				ParamTreeNode *node = static_cast<ParamTreeNode *>(iter.gobj()->user_data);
				return static_cast<int>(node->num_children());
			}

			int iter_n_root_children_vfunc() const {
				return static_cast<int>(ParamTreeNode::root()->num_children());
			}

			Gtk::TreePath get_path_vfunc(const iterator &iter) const {
				assert(iter_is_valid(iter));
				ParamTreeNode *node = static_cast<ParamTreeNode *>(iter.gobj()->user_data);
				Gtk::TreePath path;
				while (node && node->index() != static_cast<std::size_t>(-1)) {
					path.push_front(static_cast<int>(node->index()));
					node = node->parent();
				}
				return path;
			}

			void get_value_vfunc(const iterator &iter, int column, Glib::ValueBase &value) const {
				assert(iter_is_valid(iter));
				ParamTreeNode *node = static_cast<ParamTreeNode *>(iter.gobj()->user_data);

				if (column == name_column.index()) {
					Glib::Value<Glib::ustring> v;
					v.init(name_column.type());
					v.set(node->name());
					value.init(name_column.type());
					value = v;
				} else if (column == has_bool_column.index()) {
					Glib::Value<bool> v;
					v.init(has_bool_column.type());
					v.set(!!dynamic_cast<BoolParam *>(node));
					value.init(has_bool_column.type());
					value = v;
				} else if (column == bool_column.index()) {
					Glib::Value<bool> v;
					v.init(bool_column.type());
					BoolParam *bp = dynamic_cast<BoolParam *>(node);
					if (bp) {
						v.set(*bp);
					}
					value.init(bool_column.type());
					value = v;
				} else if (column == has_numeric_column.index()) {
					Glib::Value<bool> v;
					v.init(has_numeric_column.type());
					v.set(!!dynamic_cast<NumericParam *>(node));
					value.init(has_numeric_column.type());
					value = v;
				} else if (column == numeric_adjustment_column.index()) {
					Glib::Value<Gtk::Adjustment *> v;
					v.init(numeric_adjustment_column.type());
					NumericParam *np = dynamic_cast<NumericParam *>(node);
					if (np) {
						v.set(np->adjustment());
					} else {
						v.set(0);
					}
					value.init(numeric_adjustment_column.type());
					value = v;
				} else if (column == numeric_value_column.index()) {
					Glib::Value<Glib::ustring> v;
					v.init(numeric_value_column.type());
					NumericParam *np = dynamic_cast<NumericParam *>(node);
					if (np) {
						v.set(Glib::ustring::format(std::fixed, std::setprecision(np->fractional_digits()), np->adjustment()->get_value()));
					} else {
						v.set("");
					}
					value.init(numeric_value_column.type());
					value = v;
				} else if (column == numeric_digits_column.index()) {
					Glib::Value<unsigned int> v;
					v.init(numeric_digits_column.type());
					NumericParam *np = dynamic_cast<NumericParam *>(node);
					if (np) {
						v.set(np->fractional_digits());
					} else {
						v.set(0);
					}
					value.init(numeric_digits_column.type());
					value = v;
				} else {
					std::abort();
				}
			}

			bool iter_is_valid(const iterator &iter) const {
				return iter.gobj()->stamp == STAMP && iter.gobj()->user_data;
			}
	};

	class ParamTreeView : public Gtk::TreeView {
		public:
			ParamTreeView() : model(ParamTreeModel::create()), name_column("Name"), value_column("Value") {
				set_model(model);

				name_column.pack_start(name_renderer);
				name_column.add_attribute(name_renderer, "text", model->name_column);
				append_column(name_column);

				value_bool_renderer.set_activatable();
				value_bool_renderer.signal_toggled().connect(sigc::mem_fun(this, &ParamTreeView::on_value_bool_toggled));
				value_column.pack_start(value_bool_renderer);
				value_column.add_attribute(value_bool_renderer, "visible", model->has_bool_column);
				value_column.add_attribute(value_bool_renderer, "active", model->bool_column);
				value_spin_renderer.property_editable() = true;
				value_column.pack_start(value_spin_renderer);
				value_column.add_attribute(value_spin_renderer, "visible", model->has_numeric_column);
				value_column.add_attribute(value_spin_renderer, "adjustment", model->numeric_adjustment_column);
				value_column.add_attribute(value_spin_renderer, "text", model->numeric_value_column);
				value_column.add_attribute(value_spin_renderer, "digits", model->numeric_digits_column);
				append_column(value_column);
			}

		private:
			Glib::RefPtr<ParamTreeModel> model;

			Gtk::CellRendererText name_renderer;
			Gtk::TreeViewColumn name_column;

			Gtk::CellRendererToggle value_bool_renderer;
			Gtk::CellRendererSpin value_spin_renderer;
			Gtk::TreeViewColumn value_column;

			void on_value_bool_toggled(const Glib::ustring &path_string) {
				model->toggle_bool(model->get_iter(path_string));
			}
	};

	void on_load_params_clicked() {
		Gtk::MessageDialog dlg("Loading parameter values from file will discard any unsaved changes. Continue?", false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
		if (dlg.run() == Gtk::RESPONSE_YES) {
			emit_numeric_row_changed = true;
			Config::load();
			ParamTreeNode::load_all();
			emit_numeric_row_changed = false;
		}
	}

	void on_default_params_clicked() {
		Gtk::MessageDialog dlg("Setting default parameter values will discard all edited values. Continue?", false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
		if (dlg.run() == Gtk::RESPONSE_YES) {
			emit_numeric_row_changed = true;
			ParamTreeNode::default_all();
			emit_numeric_row_changed = false;
		}
	}

	void on_save_params_clicked() {
		ParamTreeNode::save_all();
		Config::save();
	}
}

ParamPanel::ParamPanel() {
	ParamTreeView *tv = Gtk::manage(new ParamTreeView);
	Gtk::ScrolledWindow *sw = Gtk::manage(new Gtk::ScrolledWindow);
	sw->add(*tv);
	pack_start(*sw, Gtk::PACK_EXPAND_WIDGET);

	Gtk::HButtonBox *hbb = Gtk::manage(new Gtk::HButtonBox(Gtk::BUTTONBOX_SPREAD));
	Gtk::Button *button = Gtk::manage(new Gtk::Button(Gtk::Stock::REVERT_TO_SAVED));
	button->signal_clicked().connect(&on_load_params_clicked);
	hbb->pack_start(*button);
	button = Gtk::manage(new Gtk::Button(Gtk::Stock::CLEAR));
	button->signal_clicked().connect(&on_default_params_clicked);
	hbb->pack_start(*button);
	button = Gtk::manage(new Gtk::Button(Gtk::Stock::SAVE));
	button->signal_clicked().connect(&on_save_params_clicked);
	hbb->pack_start(*button);
	pack_start(*hbb, Gtk::PACK_SHRINK);
}

