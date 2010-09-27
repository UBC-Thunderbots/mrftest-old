#include "uicomponents/param.h"
#include "util/algorithm.h"
#include "util/config.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iomanip>
#include <stdexcept>

namespace {
	std::vector<Param *> instances;
	Config *conf = 0;

	bool order_params_by_name(const Param * const p1, const Param * const p2) {
		return p1->name.casefold() < p2->name.casefold();
	}
}

void Param::initialized(Config *c) {
	assert(c);
	assert(!conf);
	conf = c;
	std::sort(instances.begin(), instances.end(), &order_params_by_name);
	for (std::vector<Param *>::const_iterator i = instances.begin(); i != instances.end(); ++i) {
		(*i)->load();
	}
}

Param::Param(const Glib::ustring &name) : name(name) {
	instances.push_back(this);
}

Param::~Param() {
	for (std::vector<Param *>::iterator i = instances.begin(); i != instances.end(); ) {
		if (*i == this) {
			i = instances.erase(i);
		} else {
			++i;
		}
	}
}

BoolParam::BoolParam(const Glib::ustring &name, bool def) : Param(name), default_(def) {
	value_ = def;
}

Gtk::Widget &BoolParam::widget() {
	if (!widget_.is()) {
		widget_.reset(new Gtk::CheckButton(name));
		widget_->set_active(value_);
	}
	return widget_.ref();
}

void BoolParam::apply() {
	if (widget_.is()) {
		value_ = widget_->get_active();
		conf->bool_params[name] = value_;
	}
}

void BoolParam::revert() {
	if (widget_.is()) {
		widget_->set_active(value_);
	}
}

void BoolParam::load() {
	std::map<Glib::ustring, bool>::const_iterator iter = conf->bool_params.find(name);
	if (iter != conf->bool_params.end()) {
		value_ = iter->second;
	}
	if (widget_.is()) {
		widget_->set_active(value_);
	}
}

void BoolParam::set_default() {
	value_ = default_;
	if (widget_.is()) {
		widget_->set_active(value_);
	}
}

IntParam::IntParam(const Glib::ustring &name, int def, int min, int max) : Param(name), min_(min), max_(max), default_(def) {
	if (!(min <= def && def <= max)) {
		throw std::runtime_error("Parameter default value out of valid range.");
	}
	value_ = def;
}

Gtk::Widget &IntParam::widget() {
	if (!widget_.is()) {
		widget_.reset(new Gtk::HScale);
		widget_->set_digits(0);
		widget_->get_adjustment()->configure(value_, min_, max_, 1, 10, 0);
	}
	return widget_.ref();
}

void IntParam::apply() {
	if (widget_.is()) {
		value_ = static_cast<int>(widget_->get_value());
		conf->int_params[name] = value_;
	}
}

void IntParam::revert() {
	if (widget_.is()) {
		widget_->set_value(value_);
	}
}

void IntParam::load() {
	std::map<Glib::ustring, int>::const_iterator iter = conf->int_params.find(name);
	if (iter != conf->int_params.end()) {
		value_ = clamp(iter->second, min_, max_);
	}
	if (widget_.is()) {
		widget_->set_value(value_);
	}
}

void IntParam::set_default() {
	value_ = default_;
	if (widget_.is()) {
		widget_->set_value(value_);
	}
}

DoubleParam::DoubleParam(const Glib::ustring &name, double def, double min, double max) : Param(name), min_(min), max_(max), default_(def) {
	if (!(min <= def && def <= max)) {
		throw std::runtime_error("Parameter default value out of valid range.");
	}
	value_ = def;
}

Gtk::Widget &DoubleParam::widget() {
	if (!widget_.is()) {
		widget_.reset(new Gtk::Entry);
		widget_->set_text(Glib::ustring::format(std::setprecision(8), value_));
	}
	return widget_.ref();
}

void DoubleParam::apply() {
	if (widget_.is()) {
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

void DoubleParam::revert() {
	if (widget_.is()) {
		widget_->set_text(Glib::ustring::format(std::setprecision(8), value_));
	}
}

void DoubleParam::load() {
	std::map<Glib::ustring, double>::const_iterator iter = conf->double_params.find(name);
	if (iter != conf->double_params.end()) {
		value_ = clamp(iter->second, min_, max_);
	}
	if (widget_.is()) {
		widget_->set_text(Glib::ustring::format(std::setprecision(8), value_));
	}
}

void DoubleParam::set_default() {
	value_ = default_;
	if (widget_.is()) {
		widget_->set_text(Glib::ustring::format(std::setprecision(8), value_));
	}
}

ParamPanel::ParamPanel() {
	if (!instances.empty()) {
		Gtk::Table *param_table = Gtk::manage(new Gtk::Table(instances.size(), 2));
		unsigned int y = 0;
		for (std::vector<Param *>::const_iterator i = instances.begin(); i != instances.end(); ++i) {
			Param *par = *i;
			param_table->attach(par->widget(), 0, 1, y, y + 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
			param_table->attach(*Gtk::manage(new Gtk::Label(par->name, Gtk::ALIGN_LEFT)), 1, 2, y, y + 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
			++y;
		}
		Gtk::ScrolledWindow *scroller = Gtk::manage(new Gtk::ScrolledWindow);
		scroller->add(*param_table);
		pack_start(*scroller, Gtk::PACK_EXPAND_WIDGET);
	}

	Gtk::HButtonBox *hbox = Gtk::manage(new Gtk::HButtonBox(Gtk::BUTTONBOX_CENTER));
	Gtk::Button *apply_button = Gtk::manage(new Gtk::Button(Gtk::Stock::APPLY));
	apply_button->signal_clicked().connect(sigc::mem_fun(this, &ParamPanel::on_apply_clicked));
	hbox->pack_start(*apply_button);
	Gtk::Button *save_button = Gtk::manage(new Gtk::Button(Gtk::Stock::SAVE));
	save_button->signal_clicked().connect(sigc::mem_fun(this, &ParamPanel::on_save_clicked));
	hbox->pack_start(*save_button);
	Gtk::Button *revert_button = Gtk::manage(new Gtk::Button(Gtk::Stock::CLEAR));
	revert_button->signal_clicked().connect(sigc::mem_fun(this, &ParamPanel::on_revert_clicked));
	hbox->pack_start(*revert_button);
	Gtk::Button *defaults_button = Gtk::manage(new Gtk::Button("Defaults"));
	defaults_button->signal_clicked().connect(sigc::mem_fun(this, &ParamPanel::on_defaults_clicked));
	hbox->pack_start(*defaults_button);
	pack_start(*hbox, Gtk::PACK_SHRINK);
}

void ParamPanel::on_apply_clicked() {
	for (std::vector<Param *>::const_iterator i = instances.begin(); i != instances.end(); ++i) {
		(*i)->apply();
	}
}

void ParamPanel::on_save_clicked() {
	for (std::vector<Param *>::const_iterator i = instances.begin(); i != instances.end(); ++i) {
		(*i)->apply();
	}
	conf->save();
}

void ParamPanel::on_revert_clicked() {
	for (std::vector<Param *>::const_iterator i = instances.begin(); i != instances.end(); ++i) {
		(*i)->revert();
	}
}

void ParamPanel::on_defaults_clicked() {
	for (std::vector<Param *>::const_iterator i = instances.begin(); i != instances.end(); ++i) {
		(*i)->set_default();
	}
}

