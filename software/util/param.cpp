#include "util/param.h"
#include "util/algorithm.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <functional>
#include <iomanip>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <libxml++/libxml++.h>

namespace {
	class ParamTreeNodeNameComparator {
		public:
			bool operator()(const ParamTreeNode *const p1, const ParamTreeNode *const p2) {
				return *p1 < *p2;
			}
	};

	double compute_step(double min, double max) {
		return (max - min) / 1000;
	}

	double compute_page(double min, double max) {
		return compute_step(min, max) * 10;
	}
}

class ParamTreeInternalNode : public ParamTreeNode {
	public:
		ParamTreeInternalNode *internal_node(const char *path) {
			if (path) {
				if (!path[0] || path[0] == '/') {
					throw std::invalid_argument("Path component in parameter tree path is empty");
				}
				const char *pch = std::strchr(path, '/');
				Glib::ustring component;
				if (pch) {
					component.assign(path, pch - path);
				} else {
					component.assign(path);
				}
				ParamTreeInternalNode *child = 0;
				for (auto i = children.begin(), iend = children.end(); i != iend; ++i) {
					if ((*i)->name() == component) {
						child = dynamic_cast<ParamTreeInternalNode *>(*i);
						if (!child) {
							throw std::invalid_argument(Glib::ustring::compose("Duplicate name \"%1\" in parameter tree path \"%2\" (both parameter and category)", component, this->path()));
						}
						break;
					}
				}
				if (!child) {
					child = Allocator::instance().alloc(component);
					children.push_back(child);
				}
				return child->internal_node(pch ? pch + 1 : 0);
			} else {
				return this;
			}
		}

		void add_child(ParamTreeNode *child) {
#warning check duplicates
			children.push_back(child);
		}

		void initialize() {
			// Sort all of my children by name.
			std::sort(children.begin(), children.end(), ParamTreeNodeNameComparator());
			std::for_each(children.begin(), children.end(), std::mem_fun(&ParamTreeNode::casefold_collate_key_clear));

			// Link my children with respect to sibling and parent relationships.
			for (std::size_t i = 0; i < children.size(); ++i) {
				children[i]->link(i, i < children.size() - 1 ? children[i + 1] : 0, i ? children[i - 1] : 0, this);
				children[i]->initialize();
				children[i]->casefold_collate_key_clear();
			}
		}

		void load(const xmlpp::Element *elt) {
			// Check whether the XML element is appropriate.
			if (name() == "<<<ROOT>>>") {
				assert(elt->get_name() == "params");
			} else {
				assert(elt->get_name() == "category");
				assert(elt->get_attribute_value("name") == name());
			}

			// Make a mapping from an XML child element's name's case-folded collation key to the node.
			std::unordered_map<std::string, const xmlpp::Element *> child_elts_byname;
			const xmlpp::Node::NodeList &child_elts = elt->get_children();
			for (auto i = child_elts.begin(), iend = child_elts.end(); i != iend; ++i) {
				const xmlpp::Node *child_node = *i;
				const xmlpp::Element *child_elt = dynamic_cast<const xmlpp::Element *>(child_node);
				if (child_elt) {
					const xmlpp::Attribute *name_attr = child_elt->get_attribute("name");
					if (name_attr) {
						child_elts_byname[name_attr->get_value().casefold_collate_key()] = child_elt;
					}
				}
			}

			// Go through the child nodes in the parameter tree and apply values to them.
			for (auto i = children.begin(), iend = children.end(); i != iend; ++i) {
				auto iter = child_elts_byname.find((*i)->name().casefold_collate_key());
				if (iter != child_elts_byname.end()) {
					(*i)->load(iter->second);
				}
			}
		}

		void save(xmlpp::Element *elt) const {
			if (name() != "<<<ROOT>>>") {
				elt->set_name("category");
				elt->set_attribute("name", name());
			} else {
				assert(elt->get_name() == "params");
			}
			for (auto i = children.begin(), iend = children.end(); i != iend; ++i) {
				xmlpp::Element *child_elt = elt->add_child("x");
				(*i)->save(child_elt);
			}
		}

		std::size_t num_children() const {
			return children.size();
		}

		const ParamTreeNode *child(std::size_t index) const {
			return index < num_children() ? children[index] : 0;
		}

		ParamTreeNode *child(std::size_t index) {
			return index < num_children() ? children[index] : 0;
		}

		void set_default() {
			std::for_each(children.begin(), children.end(), std::mem_fun(&ParamTreeNode::set_default));
		}

	private:
		class Allocator : public NonCopyable {
			public:
				~Allocator() {
					for (auto i = nodes.begin(), iend = nodes.end(); i != iend; ++i) {
						delete *i;
					}
				}

				static Allocator &instance() {
					static Allocator obj;
					return obj;
				}

				ParamTreeInternalNode *alloc(const Glib::ustring &name) {
					nodes.resize(nodes.size() + 1, 0);
					nodes[nodes.size() - 1] = new ParamTreeInternalNode(name);
					return nodes[nodes.size() - 1];
				}

			private:
				std::vector<ParamTreeInternalNode *> nodes;
		};

		std::vector<ParamTreeNode *> children;

		ParamTreeInternalNode(const Glib::ustring &name) : ParamTreeNode(name) {
		}

		friend ParamTreeNode *ParamTreeNode::root();
};

ParamTreeNode *ParamTreeNode::root() {
	static ParamTreeInternalNode obj("<<<ROOT>>>");
	return &obj;
}

void ParamTreeNode::default_all() {
	root()->set_default();
}

void ParamTreeNode::load_all(const xmlpp::Element *params_elt) {
	root()->load(params_elt);
}

void ParamTreeNode::save_all(xmlpp::Element *params_elt) {
	root()->save(params_elt);
}

ParamTreeNode::ParamTreeNode(const Glib::ustring &name) : name_(name), index_(static_cast<std::size_t>(-1)), next(0), prev(0), parent_(0) {
	if (name_.empty()) {
		throw std::invalid_argument("Illegal parameter name (must be nonempty)");
	}
	if (exists(name_.begin(), name_.end(), '/')) {
		throw std::invalid_argument(Glib::ustring::compose("Illegal parameter name \"%1\" (must not contain a slash)", name_));
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

bool ParamTreeNode::operator<(const ParamTreeNode &other) const {
	return casefold_collate_key() < other.casefold_collate_key();
}

const std::string &ParamTreeNode::casefold_collate_key() const {
	if (casefold_collate_key_.empty()) {
		casefold_collate_key_ = name().casefold_collate_key();
	}
	return casefold_collate_key_;
}

void ParamTreeNode::casefold_collate_key_clear() const {
	casefold_collate_key_.clear();
}

void ParamTreeNode::link(std::size_t index, ParamTreeNode *next, ParamTreeNode *prev, ParamTreeNode *parent) {
	assert(this->index_ == static_cast<std::size_t>(-1));
	this->index_ = index;
	this->next = next;
	this->prev = prev;
	this->parent_ = parent;
}

ParamTreeInternalNode *Param::internal_node(const char *) {
	throw std::logic_error("Leaf nodes cannot have children");
}

Param::Param(const char *name, const char *location) : ParamTreeNode(name) {
	assert(location[0]);
	ParamTreeInternalNode *parent = ParamTreeNode::root()->internal_node(location);
	parent->add_child(this);
}

const Glib::ustring &Param::name() const {
	return ParamTreeNode::name();
}

void Param::initialize() {
}

std::size_t Param::num_children() const {
	return 0;
}

const ParamTreeNode *Param::child(std::size_t) const {
	return 0;
}

ParamTreeNode *Param::child(std::size_t) {
	return 0;
}

BoolParam::BoolParam(const char *name, const char *location, bool def) : Param(name, location), value_(def), default_(def) {
}

void BoolParam::set_default() {
	value_ = default_;
}

void BoolParam::load(const xmlpp::Element *elt) {
	if (elt->get_name() == "boolean") {
		const xmlpp::TextNode *text_node = elt->get_child_text();
		if (text_node) {
			value_ = text_node->get_content().find("true") != Glib::ustring::npos;
		}
	}
}

void BoolParam::save(xmlpp::Element *elt) const {
	elt->set_name("boolean");
	elt->set_attribute("name", name());
	elt->set_child_text(value_ ? "true" : "false");
}

unsigned int NumericParam::fractional_digits() const {
	if (integer) {
		return 0;
	} else {
		double step = adjustment()->get_step_increment();
		double log10 = std::log10(step);
		if (log10 < 0) {
			return static_cast<int>(-log10 + 0.9);
		} else {
			return 0;
		}
	}
}

NumericParam::NumericParam(const char *name, const char *location, double def, double min, double max, bool integer) : Param(name, location), def(def), min(min), max(max), integer(integer), adjustment_(0) {
	if (!(min <= def && def <= max)) {
		throw std::invalid_argument("Parameter default value out of valid range.");
	}
}

NumericParam::~NumericParam() {
	delete adjustment_;
}

void NumericParam::set_default() {
	adjustment()->set_value(def);
}

void NumericParam::initialize() {
	Param::initialize();
	adjustment_ = new Gtk::Adjustment(def, min, max, integer ? 1 : compute_step(min, max), integer ? 10 : compute_page(min, max), 0);
	adjustment_->signal_value_changed().connect(signal_changed_reflector.make_slot());
}

IntParam::IntParam(const char *name, const char *location, int def, int min, int max) : NumericParam(name, location, def, min, max, true) {
}

void IntParam::load(const xmlpp::Element *elt) {
	if (elt->get_name() == "integer") {
		const xmlpp::TextNode *text_node = elt->get_child_text();
		std::wostringstream oss;
		oss << text_node->get_content();
		std::wistringstream iss(oss.str());
		iss.imbue(std::locale("C"));
		int v;
		iss >> v;
		adjustment()->set_value(v);
	}
}

void IntParam::save(xmlpp::Element *elt) const {
	elt->set_name("integer");
	elt->set_attribute("name", name());
	std::wostringstream oss;
	oss.imbue(std::locale("C"));
	oss << static_cast<int>(adjustment()->get_value());
	elt->set_child_text(Glib::ustring::format(oss.str()));
}

DoubleParam::DoubleParam(const char *name, const char *location, double def, double min, double max) : NumericParam(name, location, def, min, max, false) {
}

void DoubleParam::load(const xmlpp::Element *elt) {
	if (elt->get_name() == "double") {
		const xmlpp::TextNode *text_node = elt->get_child_text();
		std::wostringstream oss;
		oss << text_node->get_content();
		std::wistringstream iss(oss.str());
		iss.imbue(std::locale("C"));
		double v;
		iss >> v;
		adjustment()->set_value(v);
	}
}

void DoubleParam::save(xmlpp::Element *elt) const {
	elt->set_name("double");
	elt->set_attribute("name", name());
	std::wostringstream oss;
	oss.imbue(std::locale("C"));
	oss << std::fixed << std::setprecision(fractional_digits()) << adjustment()->get_value();
	elt->set_child_text(Glib::ustring::format(oss.str()));
}

