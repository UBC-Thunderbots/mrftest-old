#ifndef TEST_MAPPING_H
#define TEST_MAPPING_H

#include <string>
#include <glibmm/ustring.h>
#include <libxml++/libxml++.h>

/**
 * \brief A mapping between logical and physical axes and buttons.
 */
class JoystickMapping {
	public:
		/**
		 * \brief The button indices.
		 */
		enum ButtonIndex {
			/**
			 * \brief The button used to toggle the dribbler.
			 */
			BUTTON_DRIBBLE,

			/**
			 * \brief The button used to fire the kicker.
			 */
			BUTTON_KICK,

			/**
			 * \brief The button used to halt all robot operations.
			 */
			BUTTON_SCRAM,

			/**
			 * \brief The number of buttons recognized.
			 */
			N_BUTTONS
		};

		/**
		 * \brief Descriptive human-readable labels for the buttons.
		 */
		static const char *const BUTTON_LABELS[];

		/**
		 * \brief The number of axes to map.
		 */
		static const unsigned int N_AXES = 4;

		/**
		 * \brief Constructs a new mapping.
		 *
		 * \param[in] name the name of the joystick to map.
		 */
		JoystickMapping(const Glib::ustring &name);

		/**
		 * \brief Constructs a new mapping from an XML element.
		 *
		 * \param[in] elt the element.
		 */
		JoystickMapping(const xmlpp::Element *elt);

		/**
		 * \brief Returns the name of the joystick handled by this mapping.
		 *
		 * \return the name of the joystick.
		 */
		const Glib::ustring &name() const;

		/**
		 * \brief Checks if a logical axis has a mapping.
		 *
		 * \param[in] axis the logical axis to check.
		 *
		 * \return \c true if \p axis has a corresponding physical mapping, or \c false if not.
		 */
		bool has_axis(unsigned int axis) const;

		/**
		 * \brief Returns the mapping of an axis.
		 *
		 * \param[in] axis the logical axis to fetch.
		 *
		 * \return the physical axes corresponding to the logical axis.
		 *
		 * \pre the axis has a mapping.
		 */
		unsigned int axis(unsigned int axis) const;

		/**
		 * \brief Removes the mapping for an axis.
		 *
		 * \param[in] axis the logical axis to unmap.
		 */
		void clear_axis(unsigned int axis);

		/**
		 * \brief Sets the mapping for an axis.
		 *
		 * \param[in] logical the logical axis to map.
		 *
		 * \param[in] physical the physical axis to map to.
		 */
		void set_axis(unsigned int logical, unsigned int physical);

		/**
		 * \brief Checks if a logical button has a mapping.
		 *
		 * \param[in] button the logical button to check.
		 *
		 * \return \c true if \p button has a corresponding physical mapping, or \c false if not.
		 */
		bool has_button(unsigned int button) const;

		/**
		 * \brief Returns the mapping of an button.
		 *
		 * \param[in] button the logical button to fetch.
		 *
		 * \return the physical axes corresponding to the logical button.
		 *
		 * \pre the button has a mapping.
		 */
		unsigned int button(unsigned int button) const;

		/**
		 * \brief Removes the mapping for an button.
		 *
		 * \param[in] button the logical button to unmap.
		 */
		void clear_button(unsigned int button);

		/**
		 * \brief Sets the mapping for an button.
		 *
		 * \param[in] logical the logical button to map.
		 *
		 * \param[in] physical the physical button to map to.
		 */
		void set_button(unsigned int logical, unsigned int physical);

		/**
		 * \brief Encodes this joystick mapping into an XML element.
		 *
		 * \param[out] elt the element to save to.
		 */
		void save(xmlpp::Element *elt) const;

	private:
		Glib::ustring name_;
		std::string name_collate;
		int axes[N_AXES];
		int buttons[N_BUTTONS];

		friend bool operator<(const JoystickMapping &, const JoystickMapping &);
};

/**
 * \brief Compares two JoystickMapping objects.
 *
 * \param[in] m1 the first object.
 *
 * \param[in] m2 the second object.
 *
 * \return \c true if \p m1 is ordered before \p m2, or \c false if not.
 */
bool operator<(const JoystickMapping &m1, const JoystickMapping &m2);

inline const Glib::ustring &JoystickMapping::name() const {
	return name_;
}

#endif

