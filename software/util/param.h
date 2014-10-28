#ifndef UTIL_PARAM_H
#define UTIL_PARAM_H

#include "util/noncopyable.h"
#include "util/property.h"
#include <cstddef>
#include <memory>
#include <string>
#include <gtkmm/adjustment.h>

class ParamTreeInternalNode;

namespace Log {
	class Parameter;
}

namespace xmlpp {
	class Element;
}

/**
 * \brief A node in the tree of parameters.
 */
class ParamTreeNode : public NonCopyable {
	public:
		/**
		 * \brief Returns the root node of the tree.
		 *
		 * \return the root node.
		 */
		static ParamTreeNode *root();

		/**
		 * \brief Sets all parameters to their default values.
		 */
		static void default_all();

		/**
		 * \brief Loads all parameters from the configuration file's DOM tree.
		 */
		static void load_all();

		/**
		 * \brief Saves all parameters to the configuration file's DOM tree.
		 */
		static void save_all();

		/**
		 * \brief Constructs a new \c ParamTreeNode that is not yet linked into the tree.
		 *
		 * \param[in] name the name of the node.
		 */
		explicit ParamTreeNode(const Glib::ustring &name);

		/**
		 * \brief Destroys a ParamTreeNode.
		 */
		virtual ~ParamTreeNode();

		/**
		 * \brief Returns the name of the node.
		 *
		 * \return the name.
		 */
		const Glib::ustring &name() const;

		/**
		 * \brief Returns the index of the node within its parent.
		 *
		 * \return the index.
		 */
		std::size_t index() const;

		/**
		 * \brief Returns the next sibling of the node.
		 *
		 * \return the next sibling, or null if there is no next sibling.
		 */
		const ParamTreeNode *next_sibling() const;

		/**
		 * \brief Returns the next sibling of the node.
		 *
		 * \return the next sibling, or null if there is no next sibling.
		 */
		ParamTreeNode *next_sibling();

		/**
		 * \brief Returns the previous sibling of the node.
		 *
		 * \return the previous sibling, or null if there is no previous sibling.
		 */
		const ParamTreeNode *prev_sibling() const;

		/**
		 * \brief Returns the previous sibling of the node.
		 *
		 * \return the previous sibling, or null if there is no previous sibling.
		 */
		ParamTreeNode *prev_sibling();

		/**
		 * \brief Returns the parent of the node.
		 *
		 * \return the parent, or null if there is no parent.
		 */
		const ParamTreeNode *parent() const;

		/**
		 * \brief Returns the parent of the node.
		 *
		 * \return the parent, or null if there is no parent.
		 */
		ParamTreeNode *parent();

		/**
		 * \brief Returns the number of children.
		 *
		 * \return the number of children.
		 */
		virtual std::size_t num_children() const = 0;

		/**
		 * \brief Returns a child of the node.
		 *
		 * \param[in] index the index of the child to retrieve.
		 *
		 * \return the child, or null if that child does not exist.
		 */
		virtual const ParamTreeNode *child(std::size_t index) const = 0;

		/**
		 * \brief Returns a child of the node.
		 *
		 * \param[in] index the index of the child to retrieve.
		 *
		 * \return the child.
		 */
		virtual ParamTreeNode *child(std::size_t index) = 0;

		/**
		 * \brief Finds or creates an internal node that is a descendent of this node.
		 *
		 * \param[in] path the path to the node.
		 *
		 * \return the internal node found at \p path.
		 */
		virtual ParamTreeInternalNode *internal_node(const Glib::ustring &path) = 0;

		/**
		 * \brief Returns a string consisting of slash-separated components constituting the full path to the node.
		 *
		 * \return the path.
		 */
		Glib::ustring path() const;

		/**
		 * \brief Initializes the structure of this node and any child nodes.
		 */
		virtual void initialize() = 0;

		/**
		 * \brief Sets the values of the parameters to their defaults.
		 */
		virtual void set_default() = 0;

		/**
		 * \brief Loads the values of parameters from an XML tree.
		 *
		 * \param[in] elt the element representing this tree node.
		 */
		virtual void load(const xmlpp::Element *elt) = 0;

		/**
		 * \brief Stores the values of parameters into an XML tree.
		 *
		 * \param[out] elt the element to save to.
		 */
		virtual void save(xmlpp::Element *elt) const = 0;

		/**
		 * \brief Fetches a collation key for this node's name.
		 *
		 * \return the collation key.
		 */
		const std::string &collate_key() const;

		/**
		 * \brief Clears the cached collation key for this node's name.
		 */
		void collate_key_clear() const;

		/**
		 * \brief Links the node into the tree.
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
		 * \brief The name of the parameter.
		 */
		const Glib::ustring name_;

		/**
		 * \brief The position of this node within its list of siblings.
		 */
		std::size_t index_;

		/**
		 * \brief The next sibling node in the tree.
		 */
		ParamTreeNode *next;

		/**
		 * \brief The previous sibling node in the tree.
		 */
		ParamTreeNode *prev;

		/**
		 * \brief The node's parent.
		 */
		ParamTreeNode *parent_;

		/**
		 * \brief The collation key of this node's name.
		 */
		mutable std::string collate_key_;
};

/**
 * \brief A generic parameter that can be edited and whose value can be stored in the configuration file.
 */
class Param : public ParamTreeNode {
	public:
		/**
		 * \brief Finds or creates an internal node that is a descendent of this node.
		 *
		 * \param[in] path the path to the node.
		 *
		 * \return the internal node found at \p path.
		 */
		ParamTreeInternalNode *internal_node(const Glib::ustring &path) override;

		/**
		 * \brief Returns a signal fired when the value of the parameter changes.
		 *
		 * \return the signal.
		 */
		virtual sigc::signal<void> &signal_changed() const = 0;

		/**
		 * \brief Writes the current value of the parameter into a log structure.
		 *
		 * \param[in, out] param the structure to write into.
		 */
		virtual void encode_value_to_log(Log::Parameter &param) const = 0;

	protected:
		/**
		 * \brief Constructs a new parameter.
		 *
		 * Should only happen at application startup.
		 *
		 * \param[in] name the name of the parameter.
		 *
		 * \param[in] location the location in the parameter tree of the parameter.
		 */
		explicit Param(const Glib::ustring &name, const Glib::ustring &location);

		/**
		 * \brief Initializes the structure of this node and any child nodes.
		 */
		void initialize() override;

	private:
		/**
		 * \brief Returns the number of children.
		 *
		 * \return the number of children.
		 */
		std::size_t num_children() const override;

		/**
		 * \brief Returns a child of the node.
		 *
		 * \param[in] index the index of the child to retrieve.
		 *
		 * \return the child, or null if that child does not exist.
		 */
		const ParamTreeNode *child(std::size_t index) const override;

		/**
		 * \brief Returns a child of the node.
		 *
		 * \param[in] index the index of the child to retrieve.
		 *
		 * \return the child.
		 */
		ParamTreeNode *child(std::size_t index) override;
};

/**
 * \brief A boolean parameter that can be edited by means of a checkbox.
 */
class BoolParam final : public Param {
	public:
		/**
		 * \brief Constructs a new boolean parameter.
		 *
		 * Should only happen at application startup.
		 *
		 * \param[in] name the name of the parameter.
		 *
		 * \param[in] location the location in the parameter tree of the parameter.
		 *
		 * \param[in] def the default value of the parameter.
		 */
		explicit BoolParam(const Glib::ustring &name, const Glib::ustring &location, bool def);

		/**
		 * \brief Returns the value of the parameter.
		 *
		 * \return the value of the parameter.
		 */
		bool get() const {
			return value_;
		}

		/**
		 * \brief Returns the value of the parameter.
		 *
		 * \return the value of the parameter.
		 */
		operator bool() const {
			return value_;
		}

		/**
		 * \brief Returns the property implementing the parameter.
		 *
		 * \return the property implementing the parameter.
		 */
		Property<bool> &prop() {
			return value_;
		}

		/**
		 * \brief Returns the property implementing the parameter.
		 *
		 * \return the property implementing the parameter.
		 */
		const Property<bool> &prop() const {
			return value_;
		}

		/**
		 * \brief Returns a signal fired when the value of the parameter changes.
		 *
		 * \return the signal.
		 */
		sigc::signal<void> &signal_changed() const override {
			return value_.signal_changed();
		}

		/**
		 * \brief Writes the current value of the parameter into a log structure.
		 *
		 * \param[in, out] param the structure to write into.
		 */
		void encode_value_to_log(Log::Parameter &param) const override;

	private:
		Property<bool> value_;
		const bool default_;

		/**
		 * \brief Sets the values of the parameters to their defaults.
		 */
		void set_default() override;

		/**
		 * \brief Loads the values of parameters from an XML tree.
		 *
		 * \param[in] elt the element representing this tree node.
		 */
		void load(const xmlpp::Element *elt) override;

		/**
		 * \brief Stores the values of parameters into an XML tree.
		 *
		 * \param[in] elt the element representing this tree node.
		 */
		void save(xmlpp::Element *elt) const override;
};

/**
 * \brief A parameter whose value is numeric and is backed by a Gtk::Adjustment.
 */
class NumericParam : public Param {
	public:
		/**
		 * \brief Returns a signal fired when the value of the parameter changes.
		 *
		 * \return the signal.
		 */
		sigc::signal<void> &signal_changed() const override {
			return signal_changed_reflector;
		}

		/**
		 * \brief Returns the Gtk::Adjustment backing the parameter.
		 *
		 * \return the Gtk::Adjustment.
		 */
		Gtk::Adjustment *adjustment() const {
			return adjustment_.get();
		}

		/**
		 * \brief Returns the proper number of digits to display after the decimal point when rendering or editing this parameter.
		 *
		 * \return the number of decimal places.
		 */
		unsigned int fractional_digits() const;

	protected:
		/**
		 * \brief Constructs a new NumericParam.
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
		explicit NumericParam(const Glib::ustring &name, const Glib::ustring &location, double def, double min, double max, bool integer);

	private:
		const double def, min, max;
		const bool integer;
		std::unique_ptr<Gtk::Adjustment> adjustment_;
		mutable sigc::signal<void> signal_changed_reflector;

		/**
		 * \brief Sets the values of the parameters to their defaults.
		 */
		void set_default() override;

		/**
		 * \brief Constructs the backing Gtk::Adjustment.
		 */
		void initialize() override;
};

/**
 * \brief An integer parameter that can be edited by means of a scale slider.
 */
class IntParam final : public NumericParam {
	public:
		/**
		 * \brief Constructs a new integer parameter.
		 *
		 * Should only happen at application startup.
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
		explicit IntParam(const Glib::ustring &name, const Glib::ustring &location, int def, int min, int max);

		/**
		 * \brief Returns the value of the parameter.
		 *
		 * \return the value of the parameter.
		 */
		int get() const {
			return static_cast<int>(adjustment()->get_value());
		}

		/**
		 * \brief Returns the value of the parameter.
		 *
		 * \return the value of the parameter.
		 */
		operator int() const {
			return get();
		}

		/**
		 * \brief Writes the current value of the parameter into a log structure.
		 *
		 * \param[in, out] param the structure to write into.
		 */
		void encode_value_to_log(Log::Parameter &param) const override;

	private:
		/**
		 * \brief Loads the values of parameters from an XML tree.
		 *
		 * \param[in] elt the element representing this tree node.
		 */
		void load(const xmlpp::Element *elt) override;

		/**
		 * \brief Stores the values of parameters into an XML tree.
		 *
		 * \param[in] elt the element representing this tree node.
		 */
		void save(xmlpp::Element *elt) const override;
};

/**
 * \brief A floating-point parameter that can be edited by means of a text field.
 */
class DoubleParam final : public NumericParam {
	public:
		/**
		 * \brief Constructs a new double parameter.
		 *
		 * Should only happen at application startup.
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
		explicit DoubleParam(const Glib::ustring &name, const Glib::ustring &location, double def, double min, double max);

		/**
		 * \brief Returns the value of the parameter.
		 *
		 * \return the value of the parameter.
		 */
		double get() const {
			return adjustment()->get_value();
		}

		/**
		 * \brief Returns the value of the parameter.
		 *
		 * \return the value of the parameter.
		 */
		operator double() const {
			return get();
		}

		/**
		 * \brief Writes the current value of the parameter into a log structure.
		 *
		 * \param[in, out] param the structure to write into.
		 */
		void encode_value_to_log(Log::Parameter &param) const override;

	private:
		/**
		 * \brief Loads the values of parameters from an XML tree.
		 *
		 * \param[in] elt the element representing this tree node.
		 */
		void load(const xmlpp::Element *elt) override;

		/**
		 * \brief Stores the values of parameters into an XML tree.
		 *
		 * \param[in] elt the element representing this tree node.
		 */
		void save(xmlpp::Element *elt) const override;
};

#endif

