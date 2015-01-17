#include "util/param.h"
#include "proto/log_record.pb.h"
#include "util/algorithm.h"
#include "util/config.h"
#include "util/string.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <libxml++/libxml++.h>

namespace {
	double compute_step(double min, double max) {
		return (max - min) / 1000;
	}

	double compute_page(double min, double max) {
		return compute_step(min, max) * 10;
	}
}

class ParamTreeInternalNode final : public ParamTreeNode {
	public:
		ParamTreeInternalNode *internal_node(const Glib::ustring &path) {
			if (!path.empty()) {
				if (path[0] == 47 /* slash */) {
					throw std::invalid_argument("Path component in parameter tree path is empty");
				}
				Glib::ustring::size_type index_of_slash = path.find(static_cast<gunichar>(47) /* slash */);
				Glib::ustring component;
				if (index_of_slash != Glib::ustring::npos) {
					component.assign(path, 0, index_of_slash);
				} else {
					component = path;
				}
				ParamTreeInternalNode *child = nullptr;
				for (ParamTreeNode *i : children) {
					if (i->name() == component) {
						child = dynamic_cast<ParamTreeInternalNode *>(i);
						if (!child) {
							throw std::invalid_argument(Glib::locale_from_utf8(Glib::ustring::compose(u8"Duplicate name “%1” in parameter tree path “%2” (both parameter and category)", component, this->path())));
						}
						break;
					}
				}
				if (!child) {
					child = Allocator::instance().alloc(component);
					children.push_back(child);
				}
				return child->internal_node(index_of_slash != Glib::ustring::npos ? path.substr(index_of_slash + 1) : Glib::ustring());
			} else {
				return this;
			}
		}

		void add_child(ParamTreeNode *child) {
			children.push_back(child);
		}

		void initialize() {
			// Sort all of my children by name.
			std::sort(children.begin(), children.end(), [](const ParamTreeNode *n1, const ParamTreeNode *n2) { return n1->collate_key() < n2->collate_key(); });

			// The cached collation keys are no longer needed.
			std::for_each(children.begin(), children.end(), [](ParamTreeNode *n) { n->collate_key_clear(); });

			// Check for duplicates.
			{
				auto i = std::adjacent_find(children.begin(), children.end(), [](const ParamTreeNode *n1, const ParamTreeNode *n2) { return n1->name() == n2->name(); });
				if (i != children.end()) {
					throw std::invalid_argument(Glib::locale_from_utf8(Glib::ustring::compose(u8"Duplicate name “%1” in parameter tree path “%2”", (*i)->name(), path())));
				}
			}

			// Link my children with respect to sibling and parent relationships.
			for (std::size_t i = 0; i < children.size(); ++i) {
				children[i]->link(i, i < (children.size() - 1) ? children[i + 1] : nullptr, i ? children[i - 1] : nullptr, this);
				children[i]->initialize();
				children[i]->collate_key_clear();
			}
		}

		void load(const xmlpp::Element *elt) override {
			// Check whether the XML element is appropriate.
			if (name() == u8"<<<ROOT>>>") {
				assert(elt->get_name() == u8"params");
			} else {
				assert(elt->get_name() == u8"category");
				assert(elt->get_attribute_value(u8"name") == name());
			}

			// Make a mapping from an XML child element's name's collation key to the node.
			std::unordered_map<std::string, const xmlpp::Element *> child_elts_byname;
			for (const xmlpp::Node *child_node : elt->get_children()) {
				const xmlpp::Element *child_elt = dynamic_cast<const xmlpp::Element *>(child_node);
				if (child_elt) {
					const xmlpp::Attribute *name_attr = child_elt->get_attribute(u8"name");
					if (name_attr) {
						child_elts_byname[name_attr->get_value().collate_key()] = child_elt;
					}
				}
			}

			// Go through the child nodes in the parameter tree and apply values to them.
			for (ParamTreeNode *i : children) {
				auto iter = child_elts_byname.find(i->name().collate_key());
				if (iter != child_elts_byname.end()) {
					i->load(iter->second);
				}
			}
		}

		void save(xmlpp::Element *elt) const override {
			if (name() != u8"<<<ROOT>>>") {
				elt->set_name(u8"category");
				elt->set_attribute(u8"name", name());
			} else {
				assert(elt->get_name() == u8"params");
			}
			for (const ParamTreeNode *i : children) {
				xmlpp::Element *child_elt = elt->add_child(u8"x");
				i->save(child_elt);
			}
		}

		std::size_t num_children() const override {
			return children.size();
		}

		const ParamTreeNode *child(std::size_t index) const override {
			return index < num_children() ? children[index] : nullptr;
		}

		ParamTreeNode *child(std::size_t index) override {
			return index < num_children() ? children[index] : nullptr;
		}

		void set_default() override {
			std::for_each(children.begin(), children.end(), [](ParamTreeNode *n) { n->set_default(); });
		}

	private:
		class Allocator final : public NonCopyable {
			public:
				static Allocator &instance() {
					static Allocator obj;
					return obj;
				}

				ParamTreeInternalNode *alloc(const Glib::ustring &name) {
					std::unique_ptr<ParamTreeInternalNode> p(new ParamTreeInternalNode(name));
					nodes.push_back(std::move(p));
					return nodes[nodes.size() - 1].get();
				}

			private:
				std::vector<std::unique_ptr<ParamTreeInternalNode>> nodes;
		};

		std::vector<ParamTreeNode *> children;

		explicit ParamTreeInternalNode(const Glib::ustring &name) : ParamTreeNode(name) {
		}

		friend ParamTreeNode *ParamTreeNode::root();
};

ParamTreeNode *ParamTreeNode::root() {
	static ParamTreeInternalNode obj(u8"<<<ROOT>>>");
	return &obj;
}

void ParamTreeNode::default_all() {
	root()->set_default();
}

void ParamTreeNode::load_all() {
	const xmlpp::Element *params_elt = Config::params();
	root()->load(params_elt);
}

void ParamTreeNode::save_all() {
	xmlpp::Element *params_elt = Config::params();
	while (!params_elt->get_children().empty()) {
		params_elt->remove_child(params_elt->get_children().front());
	}
	root()->save(params_elt);
}

ParamTreeNode::ParamTreeNode(const Glib::ustring &name) : name_(name), index_(static_cast<std::size_t>(-1)), next(nullptr), prev(nullptr), parent_(nullptr) {
	if (name_.empty()) {
		throw std::invalid_argument("Illegal parameter name (must be nonempty)");
	}
	if (exists(name_.begin(), name_.end(), '/')) {
		throw std::invalid_argument(Glib::locale_from_utf8(Glib::ustring::compose(u8"Illegal parameter name “%1” (must not contain a slash)", name_)));
	}
}

ParamTreeNode::~ParamTreeNode() = default;

const Glib::ustring &ParamTreeNode::name() const {
	return name_;
}

std::size_t ParamTreeNode::index() const {
	return index_;
}

const ParamTreeNode *ParamTreeNode::next_sibling() const {
	return next;
}

ParamTreeNode *ParamTreeNode::next_sibling() {
	return next;
}

const ParamTreeNode *ParamTreeNode::prev_sibling() const {
	return prev;
}

ParamTreeNode *ParamTreeNode::prev_sibling() {
	return prev;
}

const ParamTreeNode *ParamTreeNode::parent() const {
	return parent_;
}

ParamTreeNode *ParamTreeNode::parent() {
	return parent_;
}

Glib::ustring ParamTreeNode::path() const {
	// Avoid writing out <<<ROOT>>> in the path.
	if (parent_ && parent_->parent_) {
		return parent_->path() + '/' + name();
	} else {
		return name();
	}
}

const std::string &ParamTreeNode::collate_key() const {
	if (collate_key_.empty()) {
		collate_key_ = name().collate_key();
	}
	return collate_key_;
}

void ParamTreeNode::collate_key_clear() const {
	collate_key_.clear();
}

void ParamTreeNode::link(std::size_t index, ParamTreeNode *next, ParamTreeNode *prev, ParamTreeNode *parent) {
	assert(this->index_ == static_cast<std::size_t>(-1));
	this->index_ = index;
	this->next = next;
	this->prev = prev;
	this->parent_ = parent;
}

ParamTreeInternalNode *Param::internal_node(const Glib::ustring &) {
	throw std::logic_error("Leaf nodes cannot have children");
}

Param::Param(const Glib::ustring &name, const Glib::ustring &location) : ParamTreeNode(name) {
	assert(location[0]);
	ParamTreeInternalNode *parent = ParamTreeNode::root()->internal_node(location);
	parent->add_child(this);
}

void Param::initialize() {
}

std::size_t Param::num_children() const {
	return 0;
}

const ParamTreeNode *Param::child(std::size_t) const {
	return nullptr;
}

ParamTreeNode *Param::child(std::size_t) {
	return nullptr;
}

BoolParam::BoolParam(const Glib::ustring &name, const Glib::ustring &location, bool def) : Param(name, location), value_(def), default_(def) {
}

void BoolParam::encode_value_to_log(Log::Parameter &param) const {
	param.set_bool_value(get());
}

void BoolParam::set_default() {
	value_ = default_;
}

void BoolParam::load(const xmlpp::Element *elt) {
	if (elt->get_name() == u8"boolean") {
		const xmlpp::TextNode *text_node = elt->get_child_text();
		if (text_node) {
			value_ = text_node->get_content().find(u8"true") != Glib::ustring::npos;
		}
	}
}

void BoolParam::save(xmlpp::Element *elt) const {
	elt->set_name(u8"boolean");
	elt->set_attribute(u8"name", name());
	elt->set_child_text(value_ ? u8"true" : u8"false");
}

unsigned int NumericParam::fractional_digits() const {
	if (integer) {
		return 0;
	} else {
		double step = adjustment()->get_step_increment();
		double log10 = std::log10(step);
		if (log10 < 0) {
			return static_cast<unsigned int>(-log10 + 0.9);
		} else {
			return 0;
		}
	}
}

NumericParam::NumericParam(const Glib::ustring &name, const Glib::ustring &location, double def, double min, double max, bool integer) : Param(name, location), def(def), min(min), max(max), integer(integer), adjustment_() {
	if (!(min <= def && def <= max)) {
		throw std::invalid_argument(Glib::locale_from_utf8(Glib::ustring::compose(u8"Parameter default value for %1/%2 out of valid range.", location, name)));
	}
}

void NumericParam::set_default() {
	adjustment()->set_value(def);
}

void NumericParam::initialize() {
	Param::initialize();
	adjustment_.reset(new Gtk::Adjustment(def, min, max, integer ? 1 : compute_step(min, max), integer ? 10 : compute_page(min, max), 0));
	adjustment_->signal_value_changed().connect(signal_changed_reflector.make_slot());
}

IntParam::IntParam(const Glib::ustring &name, const Glib::ustring &location, int def, int min, int max) : NumericParam(name, location, def, min, max, true) {
}

void IntParam::encode_value_to_log(Log::Parameter &param) const {
	param.set_int_value(get());
}

void IntParam::load(const xmlpp::Element *elt) {
	if (elt->get_name() == u8"integer") {
		const xmlpp::TextNode *text_node = elt->get_child_text();
		adjustment()->set_value(std::stoi(ustring2wstring(text_node->get_content())));
	}
}

void IntParam::save(xmlpp::Element *elt) const {
	elt->set_name(u8"integer");
	elt->set_attribute(u8"name", name());
	elt->set_child_text(todecs(static_cast<int>(adjustment()->get_value())));
}

DoubleParam::DoubleParam(const Glib::ustring &name, const Glib::ustring &location, double def, double min, double max) : NumericParam(name, location, def, min, max, false) {
}

void DoubleParam::encode_value_to_log(Log::Parameter &param) const {
	param.set_double_value(get());
}

void DoubleParam::load(const xmlpp::Element *elt) {
	if (elt->get_name() == u8"double") {
		const xmlpp::TextNode *text_node = elt->get_child_text();
		adjustment()->set_value(std::stod(ustring2wstring(text_node->get_content())));
	}
}

void DoubleParam::save(xmlpp::Element *elt) const {
	elt->set_name(u8"double");
	elt->set_attribute(u8"name", name());
	std::wostringstream oss;
	oss.imbue(std::locale("C"));
	oss.setf(std::ios_base::fixed, std::ios_base::floatfield);
	oss.precision(static_cast<int>(fractional_digits()));
	oss << adjustment()->get_value();
	elt->set_child_text(wstring2ustring(oss.str()));
}
