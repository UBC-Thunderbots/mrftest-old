#pragma once

#include <cstdint>
#include <functional>
#include "drive/primitive.h"
#include "geom/point.h"
#include "util/annunciator.h"
#include "util/noncopyable.h"
#include "util/property.h"

#define MAX_DRIBBLER_RPM 38100

namespace Drive
{
class Dongle;

/**
 * \brief A generic driveable robot.
 */
class Robot : public NonCopyable
{
   public:
    /**
     * \brief The possible states of the charger.
     */
    enum class ChargerState
    {
        /**
         * \brief Safely discharges the capacitor down to battery level.
         */
        DISCHARGE,

        /**
         * \brief Neither charges nor discharges the capacitors.
         */
        FLOAT,

        /**
         * \brief Charges the capacitor to full voltage.
         */
        CHARGE,
    };

    /**
     * \brief The pattern index of the robot.
     */
    const unsigned int index;

    /**
     * \brief The rough maximum full-scale deflection of the laser
     * sensor.
     */
    const double break_beam_scale;

    /**
     * \brief The maximum possible kick speed, in m/s.
     */
    const double kick_speed_max;

    /**
     * \brief The kick resolution for HScale.
     */

    const double kick_speed_resolution;

    /**
     * \brief The maximum possible chip distance, in m.
     */
    const double chip_distance_max;

    /**
     * \brief The chip resolution for HScale.
     */

    const double chip_distance_resolution;

    /**
     * \brief The maximum power level understood by the \ref
     * direct_dribbler function.
     */
    const unsigned int direct_dribbler_max;

    /**
     * \brief Whether or not the robot is currently responding to radio
     * communication.
     */
    Property<bool> alive;

    /**
     * \brief Whether the robot is in direct mode.
     */
    Property<bool> direct_control;

    /**
     * \brief Whether or not the ball is interrupting the robot’s laser
     * beam.
     */
    Property<bool> ball_in_beam;

    /**
     * \brief Whether or not the robot’s capacitor is charged enough to
     * kick the ball.
     */
    Property<bool> capacitor_charged;

    /**
     * \brief The voltage on the robot’s battery, in volts.
     */
    Property<double> battery_voltage;

    /**
     * \brief The voltage on the robot’s kicking capacitor, in volts.
     */
    Property<double> capacitor_voltage;

    /**
     * \brief The reading of the robot’s laser sensor.
     */
    Property<double> break_beam_reading;

    /**
     * \brief The temperature of the robot’s dribbler motor, in degrees
     * Celsius.
     */
    Property<double> dribbler_temperature;

    /**
     * \brief The speed of the robot’s dribbler motor, in revolutions
     * per minute.
     */
    Property<int> dribbler_speed;

    /**
     * \brief The temperature of the robot’s mainboard, in degrees
     * Celsius.
     */
    Property<double> board_temperature;

    /**
     * \brief The LPS reflectance values.
     */
    std::array<Property<double>, 4> lps_values;

    /**
     * \brief The link quality of the last received packet, from 0 to
     * 1.
     */
    Property<double> link_quality;

    /**
     * \brief The received signal strength of the last received packet,
     * in decibels.
     */
    Property<int> received_signal_strength;

    /**
     * \brief Whether or not the build ID information is valid.
     */
    Property<bool> build_ids_valid;

    /**
     * \brief The microcontroller firmware build ID.
     */
    Property<uint32_t> fw_build_id;

    /**
     * \brief The FPGA bitstream build ID.
     */
    Property<uint32_t> fpga_build_id;

    /**
     * \brief The current executing primitive.
     */
    Property<Primitive> primitive;

    /**
     * \brief Emitted when the autokick mechanism causes the robot to
     * kick.
     */
    sigc::signal<void> signal_autokick_fired;

    /**
     * \name Miscellaneous Functions
     * \{
     */

    /**
     * \brief Destroys a Robot.
     */
    virtual ~Robot();

    /**
     * \brief Returns the dongle controlling the robot.
     *
     * \return the dongle
     */
    virtual Drive::Dongle &dongle() = 0;

    /**
     * \brief Returns the dongle controlling the robot.
     *
     * \return the dongle
     */
    virtual const Drive::Dongle &dongle() const = 0;

    /**
     * \brief Sets the state of the capacitor charger.
     *
     * \param[in] state the state to set the charger to
     */
    virtual void set_charger_state(ChargerState state) = 0;

    /**
     * \}
     */

    /**
     * \name Movement Primitives
     *
     * The functions in this group can only be invoked if direct
     * control is inactive. Direct control is inactive by default or
     * after \ref direct_control is set to false.
     *
     * \{
     */

    /**
     * \brief Sends a low level primitive to the robot.
     *
     * \param[p] LLPrimitive to be sent to robot. See drive/primitives.h for
     * factory functions that return a LLPrimitive.
     */
    virtual void send_prim(Drive::LLPrimitive p) = 0;

    /**
     * \brief Sets whether the robot is limited to moving slowly.
     *
     * This function can be used to enforce rules about robot movement
     * speed due to certain play types.
     *
     * \param[in] slow \c true to move slowly, or \c false to move fast
     */
    virtual void move_slow(bool slow = true) = 0;

    /**
     * \}
     */

    /**
     * \name Direct Hardware Control
     *
     * The functions in this group can only be invoked if direct
     * control is active. Direct control is active after \ref
     * direct_control is set to \c true.
     *
     * \{
     */

    /**
     * \brief Sets the raw power levels sent to the robot’s wheels.
     *
     * The power levels are sent directly to the motors without any
     * control.
     *
     * \param[in] wheels the power levels sent to the wheels, in the
     * range ±255
     */
    virtual void direct_wheels(const int (&wheels)[4]) = 0;

    /**
     * \brief Sets a relative linear and angular velocity for the robot
     * to drive at.
     *
     * \param[in] vel the relative linear velocity at which to drive
     * \param[in] avel the angular velocity at which to drive
     */
    virtual void direct_velocity(Point vel, Angle avel) = 0;

    /**
     * \brief Controls the dribbler motor.
     *
     * \param[in] power the power level to use, with 0 meaning stop and
     * a maximum value given by \ref direct_dribbler_max
     */
    virtual void direct_dribbler(unsigned int rpm) = 0;

    /**
     * \brief Fires the chicker.
     *
     * \param[in] power the power, in metres per second speed (for a
     * kick) or metres distance (for a chip)
     * \param[in] chip \c true to fire the chipper or \c false to fire
     * the kicker
     */
    virtual void direct_chicker(double power, bool chip) = 0;

    /**
     * \brief Enables or disables automatic kicking when the ball
     * breaks the robot’s laser.
     *
     * If \p power is zero, automatic kicking will be disabled.
     *
     * \param[in] power the power, in metres per second speed (for a
     * kick) or metres distance (for a chip)
     * \param[in] chip \c true to fire the chipper, or \c false to fire
     * the kicker
     */
    virtual void direct_chicker_auto(double power, bool chip) = 0;

    /**
     * \}
     */

   protected:
    /**
     * \brief Constructs a new Robot.
     *
     * \param[in] index the pattern index of the robot
     * \param[in] break_beam_scale the rough maximum full-scale
     * deflection of the laser sensor
     * \param[in] kick_speed_max the maximum possible kick speed, in
     * m/s
     * \param[in] chip_distance_max the maximum possible chip distance,
     * in m
     * \param[in] direct_dribbler_max the maximum power level
     * understood by the \ref direct_dribbler function
     */
    explicit Robot(
        unsigned int index, double break_beam_scale, double kick_speed_max,
        double chip_distance_max, unsigned int direct_dribbler_max);

   private:
    Annunciator::Message low_battery_message, high_board_temperature_message;

    void handle_alive_changed();
    void handle_battery_voltage_changed();
    void handle_board_temperature_changed();
};
}

namespace std
{
template <>
struct hash<Drive::Primitive>
{
    std::size_t operator()(Drive::Primitive x) const
    {
        return static_cast<std::size_t>(x);
    }
};
}
