#ifndef UICOMPONENTS_PARAM_H
#define UICOMPONENTS_PARAM_H

#include "util/noncopyable.h"
#include "util/property.h"
#include "util/scoped_ptr.h"
#include <gtkmm.h>
#include <vector>

class Config;

/**
 * A generic parameter that can be edited and whose value can be stored in the configuration file.
 */
class Param : public NonCopyable {
	public:
		/**
		 * Informs the subsystem that the environment has been initialized and provides a configuration object to load values from.
		 *
		 * \param[in] conf the configuration to use.
		 */
		static void initialized(Config *conf);

		/**
		 * Returns all the registered Param objects.
		 *
		 * \return the parameters.
		 */
		static const std::vector<Param *> &all();

		/**
		 * The name of the parameter.
		 */
		const Glib::ustring name;

		/**
		 * Returns the UI control used to edit the parameter.
		 *
		 * \return the UI control used to edit the parameter.
		 */
		virtual Gtk::Widget &widget() = 0;

		/**
		 * Commits the current value into play.
		 */
		virtual void apply() = 0;

		/**
		 * Resets the UI controls to reflect the currently-applied value.
		 */
		virtual void revert() = 0;

		/**
		 * Loads the parameter value from the configuration file.
		 */
		virtual void load() = 0;

		/**
		 * Sets the parameter to its default value.
		 */
		virtual void set_default() = 0;

	protected:
		/**
		 * Constructs a new parameter.
		 * Should only happen at startup time.
		 *
		 * \param[in] name the name of the parameter.
		 */
		Param(const Glib::ustring &name);

		/**
		 * Destroys a parameter.
		 * Should only happen at shutdown time.
		 */
		~Param();
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
		 */
		BoolParam(const Glib::ustring &name, bool def);

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

	private:
		ScopedPtr<Gtk::CheckButton> widget_;
		Property<bool> value_;
		const bool default_;

		Gtk::Widget &widget();
		void apply();
		void revert();
		void load();
		void set_default();
};

/**
 * An integer parameter that can be edited by means of a scale slider.
 */
class IntParam : public Param {
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
		 */
		IntParam(const Glib::ustring &name, int def, int min, int max);

		/**
		 * Returns the property implementing the parameter.
		 *
		 * \return the property implementing the parameter.
		 */
		const Property<int> &prop() const {
			return value_;
		}

		/**
		 * Returns the value of the parameter.
		 *
		 * \return the value of the parameter.
		 */
		operator int() const {
			return value_;
		}

	private:
		ScopedPtr<Gtk::HScale> widget_;
		Property<int> value_;
		const int min_;
		const int max_;
		const int default_;

		Gtk::Widget &widget();
		void apply();
		void revert();
		void load();
		void set_default();
};

/**
 * A floating-point parameter that can be edited by means of a text field.
 */
class DoubleParam : public Param {
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
		 */
		DoubleParam(const Glib::ustring &name, double def, double min, double max);

		/**
		 * Returns the property implementing the parameter.
		 *
		 * \return the property implementing the parameter.
		 */
		const Property<double> &prop() const {
			return value_;
		}

		/**
		 * Returns the value of the parameter.
		 *
		 * \return the value of the parameter.
		 */
		operator double() const {
			return value_;
		}

	private:
		ScopedPtr<Gtk::Entry> widget_;
		Property<double> value_;
		const double min_;
		const double max_;
		const double default_;

		Gtk::Widget &widget();
		void apply();
		void revert();
		void load();
		void set_default();
};

/**
 * A control that allows displaying and editing parameters.
 */
class ParamPanel : public Gtk::VBox {
	public:
		/**
		 * Constructs a new ParamPanel.
		 */
		ParamPanel();

	private:
		void on_apply_clicked();
		void on_save_clicked();
		void on_revert_clicked();
		void on_defaults_clicked();
};

#endif

