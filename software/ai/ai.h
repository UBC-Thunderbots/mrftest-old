#ifndef AI_AI_H
#define AI_AI_H

#include <glibmm/ustring.h>
#include <sigc++/signal.h>
#include "ai/backend/backend.h"
#include "ai/hl/hl.h"
#include "ai/navigator/navigator.h"
#include "util/noncopyable.h"
#include "util/property.h"

namespace AI
{
/**
 * A complete %AI.
 */
class AIPackage final : public NonCopyable
{
   public:
    /**
     * The Backend against which the AI is running.
     */
    AI::BE::Backend &backend;

    /**
     * The HighLevel in use.
     */
    Property<std::unique_ptr<AI::HL::HighLevel>> high_level;

    /**
     * The Navigator navigating the robots.
     */
    Property<AI::Nav::Navigator::Ptr> navigator;

    /**
     * \brief Fired whenever the AI notes change.
     */
    mutable sigc::signal<void, Glib::ustring> signal_ai_notes_changed;

    /**
     * \brief Whether or not the overlay mechanism should render the high-level
     * overlay.
     */
    bool show_hl_overlay;

    /**
     * \brief Whether or not the overlay mechanism should render the navigator
     * overlay.
     */
    bool show_nav_overlay;

    /**
     * Constructs a new AIPackage.
     *
     * \param[in] backend the Backend against which to run.
     */
    explicit AIPackage(AI::BE::Backend &backend);

   private:
    void tick();
    void init_ai_notes();
    void ai_notes_changed();
    void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const;
    void save_setup() const;
};
}

#endif
