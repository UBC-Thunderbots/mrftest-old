#include "ai/backend/physical/player.h"
#include <glibmm/ustring.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include "drive/dongle.h"
#include "drive/robot.h"
#include "geom/angle.h"
#include "util/algorithm.h"
#include "util/config.h"
#include "util/dprint.h"
#include "util/string.h"

using AI::BE::Physical::Player;

Player::Player(unsigned int pattern, Drive::Robot &bot)
    : AI::BE::Player(pattern),
      bot(bot),
      robot_dead_message(
          Glib::ustring::compose(u8"Bot %1 dead", pattern),
          Annunciator::Message::TriggerMode::LEVEL,
          Annunciator::Message::Severity::HIGH),
      autokick_fired_(false)
{
    bot.signal_autokick_fired.connect(
        sigc::mem_fun(this, &Player::on_autokick_fired));
}

Player::~Player()
{
    bot.send_prim(Drive::move_coast());
    bot.set_charger_state(Drive::Robot::ChargerState::DISCHARGE);
}

unsigned int Player::num_bar_graphs() const
{
    return 2;
}

double Player::bar_graph_value(unsigned int index) const
{
    switch (index)
    {
        case 0:
            return bot.alive ? clamp(
                                   (bot.battery_voltage - 13.0) / (16.5 - 13.0),
                                   0.0, 1.0)
                             : 0.0;

        case 1:
            return bot.alive ? clamp(bot.capacitor_voltage / 230.0, 0.0, 1.0)
                             : 0.0;

        default:
            throw std::logic_error("invalid bar graph index");
    }
}

Visualizable::Colour Player::bar_graph_colour(unsigned int index) const
{
    switch (index)
    {
        case 0:
        {
            double value = bar_graph_value(index);
            return Visualizable::Colour(1.0 - value, value, 0.0);
        }

        case 1:
            return Visualizable::Colour(
                bot.capacitor_charged ? 0.0 : 1.0,
                bot.capacitor_charged ? 1.0 : 0.0, 0.0);

        default:
            throw std::logic_error("invalid bar graph index");
    }
}

bool Player::has_ball() const
{
    return bot.ball_in_beam;
}

double Player::get_lps(unsigned int index) const
{
    assert(index < bot.lps_values.size());
    return bot.lps_values[index];
}

bool Player::chicker_ready() const
{
    return bot.alive && bot.capacitor_charged;
}

void Player::on_autokick_fired()
{
    autokick_fired_ = true;
}

void Player::tick(bool halt, bool stop)
{
    // Show a message if the robot is dead.
    robot_dead_message.active(!bot.alive);

    // Check for emergency conditions.
    if (!bot.alive ||
        bot.dongle().estop_state != Drive::Dongle::EStopState::RUN)
    {
        halt = true;
    }

    // Clear the autokick flag so it doesn't stick at true forever.
    autokick_fired_ = false;

    // Apply driving safety rules.
    if (halt)
    {
        bot.send_prim(Drive::move_brake());
    }
    else
    {
        bot.move_slow(stop);
    }

    // Kicker should always charge except in halt.
    bot.set_charger_state(
        halt ? Drive::Robot::ChargerState::FLOAT
             : Drive::Robot::ChargerState::CHARGE);
}
