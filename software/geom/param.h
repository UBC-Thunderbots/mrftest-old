#ifndef GEOM_PARAM_H
#define GEOM_PARAM_H

#include "geom/angle.h"
#include "util/param.h"

namespace Log {
	class Parameter;
}

namespace xmlpp {
	class Element;
}

/**
 * \brief A floating-point parameter representing an angle that can be edited by means of a text field showing the angle in radians.
 */
class RadianParam final : public NumericParam {
	public:
		/**
		 * \brief Constructs a new radian parameter.
		 *
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
		explicit RadianParam(const char *name, const char *location, double def, double min, double max);

		/**
		 * \brief Returns the value of the parameter.
		 *
		 * \return the value of the parameter.
		 */
		Angle get() const {
			return Angle::of_radians(adjustment()->get_value());
		}

		/**
		 * \brief Returns the value of the parameter.
		 *
		 * \return the value of the parameter.
		 */
		operator Angle() const {
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
 * \brief A floating-point parameter representing an angle that can be edited by means of a text field showing the angle in degrees.
 */
class DegreeParam final : public NumericParam {
	public:
		/**
		 * \brief Constructs a new degree parameter.
		 *
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
		explicit DegreeParam(const char *name, const char *location, double def, double min, double max);

		/**
		 * \brief Returns the value of the parameter.
		 *
		 * \return the value of the parameter.
		 */
		Angle get() const {
			return Angle::of_degrees(adjustment()->get_value());
		}

		/**
		 * \brief Returns the value of the parameter.
		 *
		 * \return the value of the parameter.
		 */
		operator Angle() const {
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

