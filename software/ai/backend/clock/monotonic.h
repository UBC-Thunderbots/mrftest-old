#ifndef AI_BACKEND_CLOCK_MONOTONIC_H
#define AI_BACKEND_CLOCK_MONOTONIC_H

#include <glibmm/main.h>
#include <sigc++/trackable.h>
#include "ai/backend/clock/clock.h"
#include "util/annunciator.h"
#include "util/fd.h"

namespace AI
{
namespace BE
{
namespace Clock
{
/**
 * \brief A clock source backed by the system monotonic clock.
 */
class Monotonic final : public AI::BE::Clock::Clock, public sigc::trackable
{
   public:
    /**
     * \brief Constructs a new Monotonic.
     */
    explicit Monotonic();

   private:
    const FileDescriptor tfd;
    Annunciator::Message overflow_message;
    bool on_readable(Glib::IOCondition);
};
}
}
}

#endif
