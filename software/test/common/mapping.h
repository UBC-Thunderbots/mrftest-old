#ifndef TEST_COMMON_MAPPING_H
#define TEST_COMMON_MAPPING_H

#include "util/joystick.h"
#include <string>
#include <glibmm/ustring.h>
#include <libxml++/libxml++.h>

/**
 * \brief A mapping between logical and physical axes and buttons.
 */
class JoystickMapping final {
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
		static constexpr unsigned int N_AXES = 4;

		/**
		 * \brief Constructs a new mapping.
		 *
		 * \param[in] identifier identifying information about the joystick
		 */
		explicit JoystickMapping(const Joystick::Identifier &identifier);

		/**
		 * \brief Constructs a new mapping from an XML element.
		 *
		 * \param[in] elt the element
		 */
		explicit JoystickMapping(const xmlpp::Element *elt);

		/**
		 * \breif Returns the identifying information of the joystick handled by this mapping.
		 *
		 * \return the identifying information.
		 */
		const Joystick::Identifier &identifier() const;

		/**
		 * \brief Checks if a logical axis has a mapping.
		 *
		 * \param[in] axis the logical axis to check
		 *
		 * \return \c true if \p axis has a corresponding physical mapping, or \c false if not.
		 */
		bool has_axis(unsigned int axis) const;

		/**
		 * \brief Returns the mapping of an axis.
		 *
		 * \param[in] axis the logical axis to fetch
		 *
		 * \return the physical axes corresponding to the logical axis.
		 *
		 * \pre the axis has a mapping.
		 */
		unsigned int axis(unsigned int axis) const;

		/**
		 * \brief Removes the mapping for an axis.
		 *
		 * \param[in] axis the logical axis to unmap
		 */
		void clear_axis(unsigned int axis);

		/**
		 * \brief Sets the mapping for an axis.
		 *
		 * \param[in] logical the logical axis to map
		 *
		 * \param[in] physical the physical axis to map to
		 */
		void set_axis(unsigned int logical, unsigned int physical);

		/**
		 * \brief Checks if a logical button has a mapping.
		 *
		 * \param[in] button the logical button to check
		 *
		 * \return \c true if \p button has a corresponding physical mapping, or \c false if not.
		 */
		bool has_button(unsigned int button) const;

		/**
		 * \brief Returns the mapping of an button.
		 *
		 * \param[in] button the logical button to fetch
		 *
		 * \return the physical axes corresponding to the logical button.
		 *
		 * \pre the button has a mapping.
		 */
		unsigned int button(unsigned int button) const;

		/**
		 * \brief Removes the mapping for an button.
		 *
		 * \param[in] button the logical button to unmap
		 */
		void clear_button(unsigned int button);

		/**
		 * \brief Sets the mapping for an button.
		 *
		 * \param[in] logical the logical button to map
		 *
		 * \param[in] physical the physical button to map to
		 */
		void set_button(unsigned int logical, unsigned int physical);

		/**
		 * \brief Encodes this joystick mapping into an XML element.
		 *
		 * \param[out] elt the element to save to
		 */
		void save(xmlpp::Element *elt) const;

	private:
		Joystick::Identifier identifier_;
		std::string name_collate;
		int axes[N_AXES];
		int buttons[N_BUTTONS];
};



/**
 * \brief Compares two JoystickMapping objects.
 *
 * \param[in] m1 the first object
 *
 * \param[in] m2 the second object
 *
 * \return \c true if \p m1 is ordered before \p m2, or \c false if not.
 */
inline bool operator<(const JoystickMapping &m1, const JoystickMapping &m2) {
	return m1.identifier() < m2.identifier();
}



inline const Joystick::Identifier &JoystickMapping::identifier() const {
	return identifier_;
}

#endif

