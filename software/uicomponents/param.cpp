#include "uicomponents/param.h"
#include "util/algorithm.h"
#include "util/config.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iomanip>
#include <stdexcept>

namespace {
	std::vector<param *> instances;
	config *conf = 0;

	bool order_params_by_name(const param * const p1, const param * const p2) {
		return p1->name.casefold() < p2->name.casefold();
	}
}

void param::initialized(config *c) {
	assert(c);
	assert(!conf);
	conf = c;
	std::sort(instances.begin(), instances.end(), &order_params_by_name);
	for (typeof(instances.begin()) i = instances.begin(); i != instances.end(); ++i) {
		(*i)->load();
	}
}

param::param(const Glib::ustring &name) : name(name) {
	instances.push_back(this);
}

param::~param() {
	for (typeof(instances.begin()) i = instances.begin(); i != instances.end(); ) {
		if (*i == this) {
			i = instances.erase(i);
		} else {
			++i;
		}
	}
}

bool_param::bool_param(const Glib::ustring &name, bool def) : param(name), default_(def) {
	value_ = def;
}

Gtk::Widget &bool_param::widget() {
	if (!widget_) {
		widget_.reset(new Gtk::CheckButton(name));
		widget_->set_active(value_);
	}
	return widget_.ref();
}

void bool_param::apply() {
	if (widget_) {
		value_ = widget_->get_active();
		conf->bool_params[name] = value_;
	}
}

void bool_param::revert() {
	if (widget_) {
		widget_->set_active(value_);
	}
}

bool bool_param::needs_label() const {
	return false;
}

void bool_param::load() {
	typeof(conf->bool_params.begin()) iter = conf->bool_params.find(name);
	if (iter != conf->bool_params.end()) {
		value_ = iter->second;
	}
	if (widget_) {
		widget_->set_active(value_);
	}
}

void bool_param::set_default() {
	value_ = default_;
	if (widget_) {
		widget_->set_active(value_);
	}
}

int_param::int_param(const Glib::ustring &name, int def, int min, int max) : param(name), min_(min), max_(max), default_(def) {
	if (!(min <= def && def <= max)) {
		throw std::runtime_error("Parameter default value out of valid range.");
	}
	value_ = def;
}

Gtk::Widget &int_param::widget() {
	if (!widget_) {
		widget_.reset(new Gtk::HScale);
		widget_->set_digits(0);
		widget_->get_adjustment()->configure(value_, min_, max_, 1, 10, 0);
	}
	return widget_.ref();
}

void int_param::apply() {
	if (widget_) {
		value_ = static_cast<int>(widget_->get_value());
		conf->int_params[name] = value_;
	}
}

void int_param::revert() {
	if (widget_) {
		widget_->set_value(value_);
	}
}

bool int_param::needs_label() const {
	return true;
}

void int_param::load() {
	typeof(conf->int_params.begin()) iter = conf->int_params.find(name);
	if (iter != conf->int_params.end()) {
		value_ = clamp(iter->second, min_, max_);
	}
	if (widget_) {
		widget_->set_value(value_);
	}
}

void int_param::set_default() {
	value_ = default_;
	if (widget_) {
		widget_->set_value(value_);
	}
}

double_param::double_param(const Glib::ustring &name, double def, double min, double max) : param(name), min_(min), max_(max), default_(def) {
	if (!(min <= def && def <= max)) {
		throw std::runtime_error("Parameter default value out of valid range.");
	}
	value_ = def;
}

Gtk::Widget &double_param::widget() {
	if (!widget_) {
		widget_.reset(new Gtk::Entry);
		widget_->set_text(Glib::ustring::format(std::setprecision(8), value_));
	}
	return widget_.ref();
}

void double_param::apply() {
	if (widget_) {
		const std::string &txt(widget_->get_text());
		if (txt.empty()) {
			return;
		}
		char *endptr;
		double newval = std::strtod(txt.c_str(), &endptr);
		if (*endptr) {
			return;
		}
		value_ = clamp(newval, min_, max_);
		conf->double_params[name] = value_;
	}
}

void double_param::revert() {
	if (widget_) {
		widget_->set_text(Glib::ustring::format(std::setprecision(8), value_));
	}
}

bool double_param::needs_label() const {
	return true;
}

void double_param::load() {
	typeof(conf->double_params.begin()) iter = conf->double_params.find(name);
	if (iter != conf->double_params.end()) {
		value_ = clamp(iter->second, min_, max_);
	}
	if (widget_) {
		widget_->set_text(Glib::ustring::format(std::setprecision(8), value_));
	}
}

void double_param::set_default() {
	value_ = default_;
	if (widget_) {
		widget_->set_text(Glib::ustring::format(std::setprecision(8), value_));
	}
}

param_panel::param_panel() {
	if (!instances.empty()) {
		Gtk::Table *param_table = Gtk::manage(new Gtk::Table(instances.size(), 2));
		unsigned int y = 0;
		for (typeof(instances.begin()) i = instances.begin(); i != instances.end(); ++i) {
			param *par = *i;
			if (par->needs_label()) {
				param_table->attach(*Gtk::manage(new Gtk::Label(par->name)), 0, 1, y, y + 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				param_table->attach(par->widget(), 1, 2, y, y + 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
			} else {
				param_table->attach(par->widget(), 0, 2, y, y + 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
			}
			++y;
		}
		Gtk::ScrolledWindow *scroller = Gtk::manage(new Gtk::ScrolledWindow);
		scroller->add(*param_table);
		pack_start(*scroller, Gtk::PACK_EXPAND_WIDGET);
	}

	Gtk::HButtonBox *hbox = Gtk::manage(new Gtk::HButtonBox(Gtk::BUTTONBOX_CENTER));
	Gtk::Button *apply_button = Gtk::manage(new Gtk::Button(Gtk::Stock::APPLY));
	apply_button->signal_clicked().connect(sigc::mem_fun(this, &param_panel::on_apply_clicked));
	hbox->pack_start(*apply_button);
	Gtk::Button *save_button = Gtk::manage(new Gtk::Button(Gtk::Stock::SAVE));
	save_button->signal_clicked().connect(sigc::mem_fun(this, &param_panel::on_save_clicked));
	hbox->pack_start(*save_button);
	Gtk::Button *revert_button = Gtk::manage(new Gtk::Button(Gtk::Stock::CLEAR));
	revert_button->signal_clicked().connect(sigc::mem_fun(this, &param_panel::on_revert_clicked));
	hbox->pack_start(*revert_button);
	Gtk::Button *defaults_button = Gtk::manage(new Gtk::Button("Defaults"));
	defaults_button->signal_clicked().connect(sigc::mem_fun(this, &param_panel::on_defaults_clicked));
	hbox->pack_start(*defaults_button);
	pack_start(*hbox, Gtk::PACK_SHRINK);
}

void param_panel::on_apply_clicked() {
	for (typeof(instances.begin()) i = instances.begin(); i != instances.end(); ++i) {
		(*i)->apply();
	}
}

void param_panel::on_save_clicked() {
	for (typeof(instances.begin()) i = instances.begin(); i != instances.end(); ++i) {
		(*i)->apply();
	}
	conf->save();
}

void param_panel::on_revert_clicked() {
	for (typeof(instances.begin()) i = instances.begin(); i != instances.end(); ++i) {
		(*i)->revert();
	}
}

void param_panel::on_defaults_clicked() {
	for (typeof(instances.begin()) i = instances.begin(); i != instances.end(); ++i) {
		(*i)->set_default();
	}
}

