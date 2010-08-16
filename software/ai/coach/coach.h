#ifndef AI_COACH_COACH_H
#define AI_COACH_COACH_H

#include "ai/strategy/strategy.h"
#include "util/byref.h"
#include "util/registerable.h"
#include <vector>
#include <sigc++/sigc++.h>

namespace Gtk {
	class Widget;
}

class CoachFactory;

/**
 * A Coach is responsible for choosing \ref Strategy "Strategies" to run. The
 * Coach must choose a Strategy to run whenever no Strategy is currently
 * assigned (such as at the start of a game and after a Strategy resigns). The
 * Coach may also choose to forcefully destroy a Strategy if it determines the
 * Strategy is not playing a good game.
 *
 * To implement a Coach, one must:
 * <ul>
 * <li>Subclass Coach</li>
 * <li>In the subclass, override all the pure virtual functions</li>
 * <li>Subclass CoachFactory</li>
 * <li>In the subclass, override all the pure virtual functions</li>
 * <li>Create an instance of the CoachFactory in the form of a file-scope global
 * variable</li>
 * </ul>
 */
class Coach : public ByRef {
	public:
		/**
		 * A pointer to a Coach.
		 */
		typedef RefPtr<Coach> Ptr;

		/**
		 * Emitted when the Strategy changes.
		 */
		mutable sigc::signal<void, Strategy::Ptr> signal_strategy_changed;

		/**
		 * Finds the CoachFactory that constructed this Coach. Subclasses must
		 * override this function to return a reference to the global instance
		 * of their corresponding CoachFactory.
		 *
		 * \return a reference to the CoachFactory instance.
		 */
		virtual CoachFactory &get_factory() const = 0;

		/**
		 * Invoked once per time tick. Subclasses must override this function to
		 * provide their own logic.
		 *
		 * An override of this function should:
		 * <ol>
		 * <li>Check, if a Strategy currently exists, whether the Strategy has
		 * resigned (with Strategy::has_resigned()) and, if so, call
		 * clear_strategy()</li>
		 * <li>Check, if a Strategy currently exists, whether the Strategy is
		 * doing a poor job of playing the game and a different Strategy should
		 * be chosen and, if so, call clear_strategy()</li>
		 * <li>Check if there is no Strategy and, if so, select and instantiate
		 * a new Strategy appropriate to the current play type</li>
		 * <li>If there is a Strategy, call its Strategy::tick() function</li>
		 * </ol>
		 */
		virtual void tick() = 0;

		/**
		 * Returns the GTK widget for this Coach, which will be integrated into
		 * the AI's user interface.
		 *
		 * \return a GUI widget containing the controls for this Coach, or a
		 * null pointer if no GUI widgets are needed for this Coach.
		 */
		virtual Gtk::Widget *get_ui_controls() = 0;

		/**
		 * Returns the Strategy currently selected by the Coach.
		 *
		 * \return the currently-selected Strategy.
		 */
		Strategy::Ptr get_strategy() const;

		/**
		 * Finds all the \ref Strategy "Strategies" that are suitable for use
		 * with a particular play type.
		 *
		 * \param[in] pt the play type to look for
		 *
		 * \return all the StrategyFactory instances whose corresponding
		 * Strategy implementations are suitable for assignment to handle play
		 * type \p pt.
		 */
		const std::vector<StrategyFactory *> &get_strategies_by_play_type(PlayType::PlayType pt);

	protected:
		/**
		 * The World in which the Coach lives.
		 */
		const World::Ptr world;

		/**
		 * Creates a new Coach. Subclasses should call this constructor from
		 * their own constructors.
		 *
		 * \param[in] world the World in which the Coach lives.
		 */
		Coach(const World::Ptr &world);

		/**
		 * Destroys a Coach.
		 */
		~Coach();

		/**
		 * Removes the current Strategy, if any.
		 */
		void clear_strategy();

		/**
		 * Sets the current Strategy. Any existing Strategy will be replaced.
		 *
		 * \param[in] strat the new Strategy to install.
		 */
		void set_strategy(const Strategy::Ptr &strat);

		/**
		 * Sets the current Strategy. Any existing Strategy will be replaced.
		 *
		 * \param[in] fact the StrategyFactory that should be used to construct
		 * a Strategy to install.
		 */
		void set_strategy(const StrategyFactory *fact);

	private:
		Strategy::Ptr strategy;
};

/**
 * A CoachFactory is used to construct a particular type of Coach.
 */
class CoachFactory : public Registerable<CoachFactory> {
	public:
		/**
		 * Constructs a new instance of the Coach corresponding to this
		 * CoachFactory.
		 *
		 * \param[in] world the World in which the new Coach should live.
		 *
		 * \return the new Coach.
		 */
		virtual Coach::Ptr create_coach(const World::Ptr &world) const = 0;

	protected:
		/**
		 * Constructs a new CoachFactory. Subclasses should call this
		 * constructor from their own constructors.
		 *
		 * \param[in] name a human-readable name for this Coach.
		 */
		CoachFactory(const Glib::ustring &name);

		/**
		 * Destroys a CoachFactory.
		 */
		~CoachFactory();
};

#endif

