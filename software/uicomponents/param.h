#ifndef UICOMPONENTS_PARAM_H
#define UICOMPONENTS_PARAM_H

#include "util/noncopyable.h"
#include "util/scoped_ptr.h"
#include <gtkmm.h>
#include <vector>

class config;

/**
 * A generic parameter that can be edited and whose value can be stored in the
 * configuration file.
 */
class param : public noncopyable {
	public:
		/**
		 * Informs the subsystem that the environment has been initialized and
		 * provides a configuration object to load values from.
		 *
		 * \param[in] conf the configuration to use.
		 */
		static void initialized(config *conf);

		/**
		 * The name of the parameter.
		 */
		const Glib::ustring name;

		/**
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
		 * Indicates whether the parameter's name should be displayed in the UI
		 * or whether the controls include a display of the name.
		 *
		 * \return \c true if a label should be attached by the UI framework, or
		 * \c false if the parameter control includes the name.
		 */
		virtual bool needs_label() const = 0;

		/**
		 * Loads the parameter value from the configuration file.
		 */
		virtual void load() = 0;

	protected:
		/**
		 * Constructs a new parameter. Should only happen at startup time.
		 *
		 * \param[in] name the name of the parameter.
		 */
		param(const Glib::ustring &name);

		/**
		 * Destroys a parameter. Should only happen at shutdown time.
		 */
		~param();
};

/**
 * A boolean parameter that can be edited by means of a checkbox.
 */
class bool_param : public param {
	public:
		/**
		 * Constructs a new boolean parameter. Should only happen at startup
		 * time.
		 *
		 * \param[in] name the name of the parameter.
		 *
		 * \param[in] def the default value of the parameter.
		 */
		bool_param(const Glib::ustring &name, bool def);

		/**
		 * \return the value of the parameter.
		 */
		operator bool() const {
			return value_;
		}

	private:
		scoped_ptr<Gtk::CheckButton> widget_;
		bool value_;

		Gtk::Widget &widget();
		void apply();
		void revert();
		bool needs_label() const;
		void load();
};

/**
 * An integer parameter that can be edited by means of a scale slider.
 */
class int_param : public param {
	public:
		/**
		 * Constructs a new integer parameter. Should only happen at startup
		 * time.
		 *
		 * \param[in] name the name of the parameter.
		 *
		 * \param[in] def the default value of the parameter.
		 *
		 * \param[in] min the minimum value of the parameter.
		 *
		 * \param[in] max the maximum value of the parameter.
		 */
		int_param(const Glib::ustring &name, int def, int min, int max);

		/**
		 * \return the value of the parameter.
		 */
		operator int() const {
			return value_;
		}

	private:
		scoped_ptr<Gtk::HScale> widget_;
		int value_;
		const int min_;
		const int max_;

		Gtk::Widget &widget();
		void apply();
		void revert();
		bool needs_label() const;
		void load();
};

/**
 * A floating-point parameter that can be edited by means of a text field.
 */
class double_param : public param {
	public:
		/**
		 * Constructs a new double parameter. Should only happen at startup
		 * time.
		 *
		 * \param[in] name the name of the parameter.
		 *
		 * \param[in] def the default value of the parameter.
		 *
		 * \param[in] min the minimum value of the parameter.
		 *
		 * \param[in] max the maximum value of the parameter.
		 */
		double_param(const Glib::ustring &name, double def, double min, double max);

		/**
		 * \return the value of the parameter.
		 */
		operator double() const {
			return value_;
		}

	private:
		scoped_ptr<Gtk::Entry> widget_;
		double value_;
		const double min_;
		const double max_;

		Gtk::Widget &widget();
		void apply();
		void revert();
		bool needs_label() const;
		void load();
};

/**
 * A control that allows displaying and editing parameters.
 */
class param_panel : public Gtk::VBox {
	public:
		/**
		 * Constructs a new param_panel.
		 */
		param_panel();

	private:
		void on_apply_clicked();
		void on_save_clicked();
		void on_revert_clicked();
};

#endif

