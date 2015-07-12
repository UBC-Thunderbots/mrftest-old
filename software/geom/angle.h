#ifndef GEOM_ANGLE_H
#define GEOM_ANGLE_H

#include <cmath>
#include <ostream>

/**
 * \brief A typesafe representation of an angle.
 *
 * This class helps prevent accidentally combining values in degrees and radians without proper conversion.
 */
class Angle final {
	public:
		/**
		 * \brief The zero angle.
		 */
		static constexpr Angle zero();

		/**
		 * \brief The quarter-turn angle (90°).
		 */
		static constexpr Angle quarter();

		/**
		 * \brief The half-turn angle (180°).
		 */
		static constexpr Angle half();

		/**
		 * \brief The three-quarter turn angle (270°).
		 */
		static constexpr Angle three_quarter();

		/**
		 * \brief The full-turn angle (360°).
		 */
		static constexpr Angle full();

		/**
		 * \brief Constructs an angle from a value in radians.
		 *
		 * \param[in] t the angle.
		 *
		 * \return the angle.
		 */
		static constexpr Angle of_radians(double t);

		/**
		 * \brief Constructs an angle from a value in degrees.
		 *
		 * \param[in] t the angle.
		 *
		 * \return the angle.
		 */
		static constexpr Angle of_degrees(double t);

		/**
		 * \brief Computes the arc sine of a value.
		 *
		 * \param[in] x the value.
		 *
		 * \return the angle.
		 */
		static Angle asin(double x);

		/**
		 * \brief Computes the arc cosine of a value.
		 *
		 * \param[in] x the value.
		 *
		 * \return the angle.
		 */
		static Angle acos(double x);

		/**
		 * \brief Computes the arc tangent of a value.
		 *
		 * \param[in] x the value.
		 *
		 * \return the angle.
		 */
		static Angle atan(double x);

		/**
		 * \brief Constructs the "zero" angle.
		 */
		explicit constexpr Angle();

		/**
		 * \brief Converts this angle to a value in radians.
		 *
		 * \return the number of radians in this angle.
		 */
		constexpr double to_radians() const;

		/**
		 * \brief Converts this angle to a value in degrees.
		 *
		 * \return the number of degrees in this angle.
		 */
		constexpr double to_degrees() const;

		/**
		 * \brief Computes the modulus of a division between this angle and another angle.
		 *
		 * \param[in] divisor the divisor.
		 *
		 * \return the modulus of \c *this ÷ \p divisor.
		 */
		constexpr Angle mod(Angle divisor) const;

		/**
		 * \brief Computes the remainder of a division between this angle and another angle.
		 *
		 * \param[in] divisor the divisor.
		 *
		 * \return the remainder of \c *this ÷ \p divisor.
		 */
		constexpr Angle remainder(Angle divisor) const;

		/**
		 *
		 * \return the absolute value of this angle.
		 */
		constexpr Angle abs() const;

		/**
		 * \brief Checks whether the angle is finite.
		 *
		 * \return \c true if the angle is finite, or \c false if it is ±∞ or NaN.
		 */
		bool isfinite() const;

		/**
		 * \brief Computes the sine of this angle.
		 *
		 * \return the sine of this angle.
		 */
		double sin() const;

		/**
		 * \brief Computes the cosine of this angle.
		 *
		 * \return the cosine of this angle.
		 */
		double cos() const;

		/**
		 * \brief Computes the tangent of this angle.
		 *
		 * \return teh tangent of this angle.
		 */
		double tan() const;

		/**
		 * \brief Limits this angle to [−π, π].
		 *
		 * The angle is rotated by a multiple of 2π until it lies within the target interval.
		 *
		 * \return the clamped angle.
		 */
		constexpr Angle angle_mod() const;

		/**
		 * Returns the smallest possible rotational difference between this angle and another angle.
		 *
		 * \param[in] other the second angle.
		 *
		 * \return the angle between \c *this and \p other, in the range [0, π].
		 */
		constexpr Angle angle_diff(Angle other) const;

	private:
		double rads;

		explicit constexpr Angle(double rads);
};

/**
 * \brief Negates an angle.
 *
 * \param[in] angle the angle to negate.
 *
 * \return −\p angle.
 */
constexpr Angle operator-(Angle angle);

/**
 * \brief Adds two angles.
 *
 * \param[in] x the first addend.
 *
 * \param[in] y the second addend.
 *
 * \return the sum \p x + \p y.
 */
constexpr Angle operator+(Angle x, Angle y);

/**
 * \brief Subtracts two angles.
 *
 * \param[in] x the minuend.
 *
 * \param[in] y the subtrahend.
 *
 * \return the difference \p x − \p y.
 */
constexpr Angle operator-(Angle x, Angle y);

/**
 * \brief Multiplies an angle by a scalar factor.
 *
 * \param[in] angle the angle.
 *
 * \param[in] scale the scalar factor.
 *
 * \return the product \p angle × \p scale.
 */
constexpr Angle operator*(Angle angle, double scale);

/**
 * \brief Multiplies an angle by a scalar factor.
 *
 * \param[in] scale the scalar factor.
 *
 * \param[in] angle the angle.
 *
 * \return the product \p scale × \p angle.
 */
constexpr Angle operator*(double scale, Angle angle);

/**
 * \brief Divides an angle by a scalar divisor.
 *
 * \param[in] angle the angle.
 *
 * \param[in] divisor the scalar divisor.
 *
 * \return the quotient \p angle ÷ \p divisor.
 */
constexpr Angle operator/(Angle angle, double divisor);

/**
 * \brief Divides two angles.
 *
 * \param[in] x the divident.
 *
 * \param[in] y the divisor.
 *
 * \return the quotient \p x ÷ \p y.
 */
constexpr double operator/(Angle x, Angle y);

/**
 * \brief Adds an angle to an angle.
 *
 * \param[in] x the angle to add to.
 *
 * \param[in] y the angle to add.
 *
 * \return \p x.
 */
Angle &operator+=(Angle &x, Angle y);

/**
 * \brief Subtracts an angle from an angle.
 *
 * \param[in] x the angle to subtract from.
 *
 * \param[in] y the angle to subtract.
 *
 * \return \p x.
 */
Angle &operator-=(Angle &x, Angle y);

/**
 * \brief Scales an angle by a factor.
 *
 * \param[in] angle the angle to scale.
 *
 * \param[in] scale the scalar factor.
 *
 * \return \p angle.
 */
Angle &operator*=(Angle &angle, double scale);

/**
 * \brief Divides an angle by a scalar divisor.
 *
 * \param[in] angle the angle to scale.
 *
 * \param[in] divisor the scalar divisor.
 *
 * \return \p angle.
 */
Angle &operator/=(Angle &angle, double divisor);

/**
 * \brief Compares two angles.
 *
 * \param[in] x the first angle.
 *
 * \param[in] y the second angle.
 *
 * \return \c true if \p x < \p y, or \c false if not.
 */
constexpr bool operator<(Angle x, Angle y);

/**
 * \brief Compares two angles.
 *
 * \param[in] x the first angle.
 *
 * \param[in] y the second angle.
 *
 * \return \c true if \p x > \p y, or \c false if not.
 */
constexpr bool operator>(Angle x, Angle y);

/**
 * \brief Compares two angles.
 *
 * \param[in] x the first angle.
 *
 * \param[in] y the second angle.
 *
 * \return \c true if \p x ≤ \p y, or \c false if not.
 */
constexpr bool operator<=(Angle x, Angle y);

/**
 * \brief Compares two angles.
 *
 * \param[in] x the first angle.
 *
 * \param[in] y the second angle.
 *
 * \return \c true if \p x ≥ \p y, or \c false if not.
 */
constexpr bool operator>=(Angle x, Angle y);

/**
 * \brief Compares two angles.
 *
 * \param[in] x the first angle.
 *
 * \param[in] y the second angle.
 *
 * \return \c true if \p x = \p y, or \c false if not.
 */
constexpr bool operator==(Angle x, Angle y);

/**
 * \brief Compares two angles.
 *
 * \param[in] x the first angle.
 *
 * \param[in] y the second angle.
 *
 * \return \c true if \p x ≠ \p y, or \c false if not.
 */
constexpr bool operator!=(Angle x, Angle y);

/**
 * \brief Converts an angle to a string representation.
 *
 * \param[in] s the stream to write to.
 *
 * \param[in] a the angle to write.
 *
 * \return \p s.
 */
template<typename CharT, typename Traits> std::basic_ostream<CharT, Traits> &operator<<(std::basic_ostream<CharT, Traits> &s, Angle a);





inline constexpr Angle Angle::zero() {
	return Angle();
}

inline constexpr Angle Angle::quarter() {
	return Angle(M_PI / 2.0);
}

inline constexpr Angle Angle::half() {
	return Angle(M_PI);
}

inline constexpr Angle Angle::three_quarter() {
	return Angle(3.0 / 2.0 * M_PI);
}

inline constexpr Angle Angle::full() {
	return Angle(2.0 * M_PI);
}

inline constexpr Angle Angle::of_radians(double t) {
	return Angle(t);
}

inline constexpr Angle Angle::of_degrees(double t) {
	return Angle(t / 180.0 * M_PI);
}

inline Angle Angle::asin(double x) {
	return Angle::of_radians(std::asin(x));
}

inline Angle Angle::acos(double x) {
	return Angle::of_radians(std::acos(x));
}

inline Angle Angle::atan(double x) {
	return Angle::of_radians(std::atan(x));
}

inline constexpr Angle::Angle() : rads(0.0) {
}

inline constexpr double Angle::to_radians() const {
	return rads;
}

inline constexpr double Angle::to_degrees() const {
	return rads / M_PI * 180.0;
}

inline constexpr Angle Angle::mod(Angle divisor) const {
	return Angle::of_radians(to_radians() - static_cast<double>(static_cast<long>(to_radians() / divisor.to_radians())) * divisor.to_radians());
}

inline constexpr Angle Angle::remainder(Angle divisor) const {
	return Angle::of_radians(to_radians() - static_cast<double>(static_cast<long>((to_radians() / divisor.to_radians()) >= 0 ? (to_radians() / divisor.to_radians() + 0.5) : (to_radians() / divisor.to_radians() - 0.5))) * divisor.to_radians());
}

inline constexpr Angle Angle::abs() const {
	return Angle::of_radians(to_radians() < 0 ? -to_radians() : to_radians());
}

inline bool Angle::isfinite() const {
	return std::isfinite(to_radians());
}

inline double Angle::sin() const {
	return std::sin(to_radians());
}

inline double Angle::cos() const {
	return std::cos(to_radians());
}

inline double Angle::tan() const {
	return std::tan(to_radians());
}

inline constexpr Angle Angle::angle_mod() const {
	return remainder(Angle::full());
}

inline constexpr Angle Angle::angle_diff(Angle other) const {
	return (*this - other).angle_mod().abs();
}

inline constexpr Angle::Angle(double rads) : rads(rads) {
}

inline constexpr Angle operator-(Angle angle) {
	return Angle::of_radians(-angle.to_radians());
}

inline constexpr Angle operator+(Angle x, Angle y) {
	return Angle::of_radians(x.to_radians() + y.to_radians());
}

inline constexpr Angle operator-(Angle x, Angle y) {
	return Angle::of_radians(x.to_radians() - y.to_radians());
}

inline constexpr Angle operator*(Angle angle, double scale) {
	return Angle::of_radians(angle.to_radians() * scale);
}

inline constexpr Angle operator*(double scale, Angle angle) {
	return Angle::of_radians(scale * angle.to_radians());
}

inline constexpr Angle operator/(Angle angle, double divisor) {
	return Angle::of_radians(angle.to_radians() / divisor);
}

inline constexpr double operator/(Angle x, Angle y) {
	return x.to_radians() / y.to_radians();
}

inline Angle &operator+=(Angle &x, Angle y) {
	return x = x + y;
}

inline Angle &operator-=(Angle &x, Angle y) {
	return x = x - y;
}

inline Angle &operator*=(Angle &angle, double scale) {
	return angle = angle * scale;
}

inline Angle &operator/=(Angle &angle, double divisor) {
	return angle = angle / divisor;
}

inline constexpr bool operator<(Angle x, Angle y) {
	return x.to_radians() < y.to_radians();
}

inline constexpr bool operator>(Angle x, Angle y) {
	return x.to_radians() > y.to_radians();
}

inline constexpr bool operator<=(Angle x, Angle y) {
	return x.to_radians() <= y.to_radians();
}

inline constexpr bool operator>=(Angle x, Angle y) {
	return x.to_radians() >= y.to_radians();
}

inline constexpr bool operator==(Angle x, Angle y) {
	return x.to_radians() == y.to_radians();
}

inline constexpr bool operator!=(Angle x, Angle y) {
	return x.to_radians() != y.to_radians();
}

template<typename CharT, typename Traits> inline std::basic_ostream<CharT, Traits> &operator<<(std::basic_ostream<CharT, Traits> &s, Angle a) {
	s << a.to_radians() << 'R';
	return s;
}

#endif

