#ifndef UTIL_PARAM_H
#define UTIL_PARAM_H

#include "util/noncopyable.h"
#include "util/property.h"
#include "util/scoped_ptr.h"
#include <cstddef>
#include <gtkmm.h>
#include <string>

class ParamTreeInternalNode;

namespace xmlpp {
	class Element;
}

/**
 * A node in the tree of parameters.
 */
class ParamTreeNode : public NonCopyable {
	public:
		/**
		 * Returns the root node of the tree.
		 *
		 * \return the root node.
		 */
		static ParamTreeNode *root();

		/**
		 * Sets all parameters to their default values.
		 */
		static void default_all();

		/**
		 * Loads all parameters from the configuration file.
		 */
		static void load_all();

		/**
		 * Saves all parameters to the configuration file.
		 */
		static void save_all();

		/**
		 * Constructs a new \c ParamTreeNode that is not yet linked into the tree.
		 *
		 * \param[in] name the name of the node.
		 */
		ParamTreeNode(const Glib::ustring &name);

		/**
		 * Destroys a ParamTreeNode.
		 */
		virtual ~ParamTreeNode();

		/**
		 * Returns the name of the node.
		 *
		 * \return the name.
		 */
		const Glib::ustring &name() const;

		/**
		 * Returns the index of the node within its parent.
		 *
		 * \return the index.
		 */
		std::size_t index() const;

		/**
		 * Returns the next sibling of the node.
		 *
		 * \return the next sibling, or null if there is no next sibling.
		 */
		const ParamTreeNode *next_sibling() const;

		/**
		 * Returns the next sibling of the node.
		 *
		 * \return the next sibling, or null if there is no next sibling.
		 */
		ParamTreeNode *next_sibling();

		/**
		 * Returns the previous sibling of the node.
		 *
		 * \return the previous sibling, or null if there is no previous sibling.
		 */
		const ParamTreeNode *prev_sibling() const;

		/**
		 * Returns the previous sibling of the node.
		 *
		 * \return the previous sibling, or null if there is no previous sibling.
		 */
		ParamTreeNode *prev_sibling();

		/**
		 * Returns the parent of the node.
		 *
		 * \return the parent, or null if there is no parent.
		 */
		const ParamTreeNode *parent() const;

		/**
		 * Returns the parent of the node.
		 *
		 * \return the parent, or null if there is no parent.
		 */
		ParamTreeNode *parent();

		/**
		 * Returns the number of children.
		 *
		 * \return the number of children.
		 */
		virtual std::size_t num_children() const = 0;

		/**
		 * Returns a child of the node.
		 *
		 * \param[in] index the index of the child to retrieve.
		 *
		 * \return the child, or null if that child does not exist.
		 */
		virtual const ParamTreeNode *child(std::size_t index) const = 0;

		/**
		 * Returns a child of the node.
		 *
		 * \param[in] index the index of the child to retrieve.
		 *
		 * \return the child.
		 */
		virtual ParamTreeNode *child(std::size_t index) = 0;

		/**
		 * Finds or creates an internal node that is a descendent of this node.
		 *
		 * \param[in] path the path to the node.
		 *
		 * \return the internal node found at \p path.
		 */
		virtual ParamTreeInternalNode *internal_node(const char *path) = 0;

		/**
		 * Returns a string consisting of slash-separated components constituting the full path to the node.
		 *
		 * \return the path.
		 */
		Glib::ustring path() const;

		/**
		 * Initializes the structure of this node and any child nodes.
		 */
		virtual void initialize() = 0;

		/**
		 * Sets the values of the parameters to their defaults.
		 */
		virtual void set_default() = 0;

		/**
		 * Loads the values of parameters from an XML tree.
		 *
		 * \param[in] elt the element representing this tree node.
		 */
		virtual void load(const xmlpp::Element *elt) = 0;

		/**
		 * Stores the values of parameters into an XML tree.
		 *
		 * \param[in] elt the element representing this tree node.
		 *
		 * \param[in] indent the number of tabs to indent the current element.
		 */
		virtual void save(xmlpp::Element *elt, unsigned int indent) const = 0;

		/**
		 * Compares two nodes using a case-insensitive comparison of their names.
		 *
		 * \param[in] other the other object to compare to.
		 *
		 * \return \c true if \c this is less than \p other, or \c false if not.
		 */
		bool operator<(const ParamTreeNode &other) const;

		/**
		 * Fetches a case-folded collation key for this node's name.
		 *
		 * \return the collation key.
		 */
		const std::string &casefold_collate_key() const;

		/**
		 * Clears the cached case-folded collation key for this node's name.
		 */
		void casefold_collate_key_clear() const;

		/**
		 * Links the node into the tree.
		 *
		 * \param[in] index the position of this node within its list of siblings.
		 *
		 * \param[in] next the next sibling node in the tree.
		 *
		 * \param[in] prev the previous sibling node in the tree.
		 *
		 * \param[in] parent the node's parent.
		 */
		void link(std::size_t index, ParamTreeNode *next, ParamTreeNode *prev, ParamTreeNode *parent);

	private:
		/**
		 * The name of the parameter.
		 */
		const Glib::ustring name_;

		/**
		 * The position of this node within its list of siblings.
		 */
		std::size_t index_;

		/**
		 * The next sibling node in the tree.
		 */
		ParamTreeNode *next;

		/**
		 * The previous sibling node in the tree.
		 */
		ParamTreeNode *prev;

		/**
		 * The node's parent.
		 */
		ParamTreeNode *parent_;

		/**
		 * The case-folded collation key of this node's name.
		 */
		mutable std::string casefold_collate_key_;
};

/**
 * A generic parameter that can be edited and whose value can be stored in the configuration file.
 */
class Param : public ParamTreeNode {
	public:
		/**
		 * Finds or creates an internal node that is a descendent of this node.
		 *
		 * \param[in] path the path to the node.
		 *
		 * \return the internal node found at \p path.
		 */
		ParamTreeInternalNode *internal_node(const char *path);

		/**
		 * Returns a signal fired when the value of the parameter changes.
		 *
		 * \return the signal.
		 */
		virtual sigc::signal<void> &signal_changed() const = 0;

	protected:
		/**
		 * Constructs a new parameter.
		 * Should only happen at startup time.
		 *
		 * \param[in] name the name of the parameter.
		 *
		 * \param[in] location the location in the parameter tree of the parameter.
		 */
		Param(const char *name, const char *location);

		/**
		 * Returns the name of the parameter.
		 *
		 * \return the name.
		 */
		const Glib::ustring &name() const;

		/**
		 * Initializes the structure of this node and any child nodes.
		 */
		void initialize();

	private:
		/**
		 * Returns the number of children.
		 *
		 * \return the number of children.
		 */
		std::size_t num_children() const;

		/**
		 * Returns a child of the node.
		 *
		 * \param[in] index the index of the child to retrieve.
		 *
		 * \return the child, or null if that child does not exist.
		 */
		const ParamTreeNode *child(std::size_t index) const;

		/**
		 * Returns a child of the node.
		 *
		 * \param[in] index the index of the child to retrieve.
		 *
		 * \return the child.
		 */
		ParamTreeNode *child(std::size_t index);
};

/**
 * A boolean parameter that can be edited by means of a checkbox.
 */
class BoolParam : public Param {
	public:
		/**
		 * Constructs a new boolean parameter.
		 * Should only happen at startup time.
		 *
		 * \param[in] name the name of the parameter.
		 *
		 * \param[in] def the default value of the parameter.
		 *
		 * \deprecated in favour of BoolParam(const Glib::ustring &, const Glib::ustring &, bool).
		 */
		BoolParam(const char *name, bool def) __attribute__((deprecated));

		/**
		 * Constructs a new boolean parameter.
		 * Should only happen at startup time.
		 *
		 * \param[in] name the name of the parameter.
		 *
		 * \param[in] location the location in the parameter tree of the parameter.
		 *
		 * \param[in] def the default value of the parameter.
		 */
		BoolParam(const char *name, const char *location, bool def);

		/**
		 * Returns the property implementing the parameter.
		 *
		 * \return the property implementing the parameter.
		 */
		Property<bool> &prop() {
			return value_;
		}

		/**
		 * Returns the property implementing the parameter.
		 *
		 * \return the property implementing the parameter.
		 */
		const Property<bool> &prop() const {
			return value_;
		}

		/**
		 * Returns the value of the parameter.
		 *
		 * \return the value of the parameter.
		 */
		operator bool() const {
			return value_;
		}

		/**
		 * Returns a signal fired when the value of the parameter changes.
		 *
		 * \return the signal.
		 */
		sigc::signal<void> &signal_changed() const {
			return value_.signal_changed();
		}

	private:
		Property<bool> value_;
		const bool default_;

		/**
		 * Sets the values of the parameters to their defaults.
		 */
		void set_default();

		/**
		 * Loads the values of parameters from an XML tree.
		 *
		 * \param[in] elt the element representing this tree node.
		 */
		void load(const xmlpp::Element *elt);

		/**
		 * Stores the values of parameters into an XML tree.
		 *
		 * \param[in] elt the element representing this tree node.
		 */
		void save(xmlpp::Element *elt, unsigned int) const;
};

/**
 * A parameter whose value is numeric and is backed by a Gtk::Adjustment.
 */
class NumericParam : public Param {
	public:
		/**
		 * Returns a signal fired when the value of the parameter changes.
		 *
		 * \return the signal.
		 */
		sigc::signal<void> &signal_changed() const {
			return signal_changed_reflector;
		}

		/**
		 * Returns the Gtk::Adjustment backing the parameter.
		 *
		 * \return the Gtk::Adjustment.
		 */
		Gtk::Adjustment *adjustment() const {
			return adjustment_;
		}

		/**
		 * Returns the proper number of digits to display after the decimal point when rendering or editing this parameter.
		 *
		 * \return the number of decimal places.
		 */
		unsigned int fractional_digits() const;

	protected:
		/**
		 * Constructs a new NumericParam.
		 *
		 * \param[in] name the name of the parameter.
		 *
		 * \param[in] location the location in the parameter tree of the parameter.
		 *
		 * \param[in] def the default value of the parameter.
		 *
		 * \param[in] min the minimum value of the parameter.
		 *
		 * \param[in] max the maximum value of the parameter.
		 *
		 * \param[in] integer \c true if the parameter should take integer values, or \c false if it may take non-integer values.
		 */
		NumericParam(const char *name, const char *location, double def, double min, double max, bool integer);

		/**
		 * Destroys a NumericParam.
		 */
		~NumericParam();

	private:
		const double def, min, max;
		const bool integer;
		Gtk::Adjustment *adjustment_;
		mutable sigc::signal<void> signal_changed_reflector;

		/**
		 * Sets the values of the parameters to their defaults.
		 */
		void set_default();

		/**
		 * Constructs the backing Gtk::Adjustment.
		 */
		void initialize();
};

/**
 * An integer parameter that can be edited by means of a scale slider.
 */
class IntParam : public NumericParam {
	public:
		/**
		 * Constructs a new integer parameter.
		 * Should only happen at startup time.
		 *
		 * \param[in] name the name of the parameter.
		 *
		 * \param[in] def the default value of the parameter.
		 *
		 * \param[in] min the minimum value of the parameter.
		 *
		 * \param[in] max the maximum value of the parameter.
		 *
		 * \deprecated in favour of IntParam(const Glib::ustring &, const Glib::ustring &, int, int, int).
		 */
		IntParam(const char *name, int def, int min, int max) __attribute__((deprecated));

		/**
		 * Constructs a new integer parameter.
		 * Should only happen at startup time.
		 *
		 * \param[in] name the name of the parameter.
		 *
		 * \param[in] location the location in the parameter tree of the parameter.
		 *
		 * \param[in] def the default value of the parameter.
		 *
		 * \param[in] min the minimum value of the parameter.
		 *
		 * \param[in] max the maximum value of the parameter.
		 */
		IntParam(const char *name, const char *location, int def, int min, int max);

		/**
		 * Returns the value of the parameter.
		 *
		 * \return the value of the parameter.
		 */
		operator int() const {
			return static_cast<int>(adjustment()->get_value());
		}

	private:
		/**
		 * Loads the values of parameters from an XML tree.
		 *
		 * \param[in] elt the element representing this tree node.
		 */
		void load(const xmlpp::Element *elt);

		/**
		 * Stores the values of parameters into an XML tree.
		 *
		 * \param[in] elt the element representing this tree node.
		 */
		void save(xmlpp::Element *elt, unsigned int) const;
};

/**
 * A floating-point parameter that can be edited by means of a text field.
 */
class DoubleParam : public NumericParam {
	public:
		/**
		 * Constructs a new double parameter.
		 * Should only happen at startup time.
		 *
		 * \param[in] name the name of the parameter.
		 *
		 * \param[in] def the default value of the parameter.
		 *
		 * \param[in] min the minimum value of the parameter.
		 *
		 * \param[in] max the maximum value of the parameter.
		 *
		 * \deprecated in favour of DoubleParam(const Glib::ustring &, const Glib::ustring &, double, double, double).
		 */
		DoubleParam(const char *name, double def, double min, double max) __attribute__((deprecated));

		/**
		 * Constructs a new double parameter.
		 * Should only happen at startup time.
		 *
		 * \param[in] name the name of the parameter.
		 *
		 * \param[in] location the location in the parameter tree of the parameter.
		 *
		 * \param[in] def the default value of the parameter.
		 *
		 * \param[in] min the minimum value of the parameter.
		 *
		 * \param[in] max the maximum value of the parameter.
		 */
		DoubleParam(const char *name, const char *location, double def, double min, double max);

		/**
		 * Returns the value of the parameter.
		 *
		 * \return the value of the parameter.
		 */
		operator double() const {
			return adjustment()->get_value();
		}

	private:
		/**
		 * Loads the values of parameters from an XML tree.
		 *
		 * \param[in] elt the element representing this tree node.
		 */
		void load(const xmlpp::Element *elt);

		/**
		 * Stores the values of parameters into an XML tree.
		 *
		 * \param[in] elt the element representing this tree node.
		 */
		void save(xmlpp::Element *elt, unsigned int) const;
};

#endif

