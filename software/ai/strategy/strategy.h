#ifndef AI_STRATEGY_STRATEGY_H
#define AI_STRATEGY_STRATEGY_H

#include "ai/world/world.h"
#include "util/byref.h"
#include "util/registerable.h"
#include <cstddef>
#include <sigc++/sigc++.h>

class StrategyFactory;

/**
 * A Strategy manages the operation of the entire team during a segment of time.
 * Each Strategy is suitable to be created and assigned to the team during some
 * specific play type or play types; the Coach makes the decision about when to
 * assign a Strategy to the team. The Strategy then runs the team until deciding
 * that it is no longer suitable (perhaps because the play type changed to one
 * with which it is unfamiliar) or until the Coach decides to remove the
 * Strategy from play.
 *
 * To implement a Strategy, one must:
 * <ul>
 * <li>Subclass Strategy</li>
 * <li>In the subclass, override the virtual functions corresponding to the play
 * types one is interested in handling</li>
 * <li>Subclass StrategyFactory</li>
 * <li>In the subclass, override all the pure virtual functions</li>
 * <li>Create an instance of the StrategyFactory in the form of a file-scope
 * global variable</li>
 * </ul>
 */
class Strategy : public ByRef, public sigc::trackable {
	public:
		/**
		 * A pointer to a Strategy.
		 */
		typedef RefPtr<Strategy> Ptr;

		/**
		 * Finds the StrategyFactory that constructed this Strategy. Subclasses
		 * must override this function to return a reference to the global
		 * instance of their corresponding StrategyFactory.
		 *
		 * \return a reference to the StrategyFactory instance.
		 */
		virtual StrategyFactory &get_factory() const = 0;

		/**
		 * Invoked once per time tick when the game is in PlayType::HALT.
		 * Subclasses may override this function to provide their own logic.
		 * Most subclasses will have no reason to override this function.
		 */
		virtual void halt();

		/**
		 * Invoked once per time tick when the game is in PlayType::STOP.
		 * Subclasses may override this function to provide their own logic.
		 * Most subclasses should not override this function. The default
		 * implementation causes the strategy to resign control, which is
		 * appropriate for a stoppage in play as it allows the Coach to choose
		 * a new Strategy when a special play is issued.
		 */
		virtual void stop();

		/**
		 * Invoked once per time tick when the game is in PlayType::PLAY.
		 * Subclasses may override this function to provide their own logic.
		 */
		virtual void play();

		/**
		 * Invoked once per time tick when the game is in
		 * PlayType::PREPARE_KICKOFF_FRIENDLY. Subclasses may override this
		 * function to provide their own logic.
		 */
		virtual void prepare_kickoff_friendly();

		/**
		 * Invoked once per time tick when the game is in
		 * PlayType::EXECUTE_KICKOFF_FRIENDLY. Subclasses may override this
		 * function to provide their own logic.
		 */
		virtual void execute_kickoff_friendly();

		/**
		 * Invoked once per time tick when the game is in
		 * PlayType::PREPARE_KICKOFF_ENEMY. Subclasses may override this
		 * function to provide their own logic.
		 */
		virtual void prepare_kickoff_enemy();

		/**
		 * Invoked once per time tick when the game is in
		 * PlayType::EXECUTE_KICKOFF_ENEMY. Subclasses may override this
		 * function to provide their own logic.
		 */
		virtual void execute_kickoff_enemy();

		/**
		 * Invoked once per time tick when the game is in
		 * PlayType::PREPARE_PENALTY_FRIENDLY. Subclasses may override this
		 * function to provide their own logic.
		 */
		virtual void prepare_penalty_friendly();

		/**
		 * Invoked once per time tick when the game is in
		 * PlayType::EXECUTE_PENALTY_FRIENDLY. Subclasses may override this
		 * function to provide their own logic.
		 */
		virtual void execute_penalty_friendly();

		/**
		 * Invoked once per time tick when the game is in
		 * PlayType::PREPARE_PENALTY_ENEMY. Subclasses may override this
		 * function to provide their own logic.
		 */
		virtual void prepare_penalty_enemy();

		/**
		 * Invoked once per time tick when the game is in
		 * PlayType::EXECUTE_PENALTY_ENEMY. Subclasses may override this
		 * function to provide their own logic.
		 */
		virtual void execute_penalty_enemy();

		/**
		 * Invoked once per time tick when the game is in
		 * PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY. Subclasses may override
		 * this function to provide their own logic.
		 */
		virtual void execute_direct_free_kick_friendly();

		/**
		 * Invoked once per time tick when the game is in
		 * PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY. Subclasses may
		 * override this function to provide their own logic.
		 */
		virtual void execute_indirect_free_kick_friendly();

		/**
		 * Invoked once per time tick when the game is in
		 * PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY. Subclasses may override
		 * this function to provide their own logic.
		 */
		virtual void execute_direct_free_kick_enemy();

		/**
		 * Invoked once per time tick when the game is in
		 * PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY. Subclasses may override
		 * this function to provide their own logic.
		 */
		virtual void execute_indirect_free_kick_enemy();

		/**
		 * Checks if the Strategy has resigned.
		 *
		 * \return \c true if this Strategy has resigned control over the team,
		 * or \c false if not.
		 */
		bool has_resigned() const;

		/**
		 * Dispatches a time tick to the appropriate handler function. This
		 * function is intended to be called by the framework and should not be
		 * touched by the AI.
		 */
		void tick();

	protected:
		/**
		 * The World in which the Strategy lives.
		 */
		const World::Ptr world;

		/**
		 * Constructs a new Strategy. Subclasses should call this constructor
		 * from their own constructors.
		 *
		 * \param[in] world the World in which the Strategy lives.
		 */
		Strategy(const World::Ptr &world);

		/**
		 * Destroys a Strategy.
		 */
		~Strategy();

		/**
		 * A subclass can invoke this function if it determines that it no
		 * longer wishes to control the team. The Coach will look for another
		 * Strategy to assign.
		 */
		void resign();

	private:
		bool has_resigned_;
};

/**
 * A StrategyFactory is used to construct a particular type of Strategy. The
 * factory permits the Coach to discover all the available types of Strategy and
 * to interrogate each StrategyFactory about what play types its corresponding
 * Strategy is suitable to play.
 */
class StrategyFactory : public Registerable<StrategyFactory> {
	public:
		/**
		 * A pointer to the first play type in an array of play types the
		 * corresponding Strategy is willing to handle.
		 */
		const PlayType::PlayType * const handled_play_types;

		/**
		 * The number of elements in the \ref handled_play_types array.
		 */
		const std::size_t handled_play_types_size;

		/**
		 * Constructs a new instance of the Strategy corresponding to this
		 * StrategyFactory.
		 *
		 * \param[in] world the World in which the new Strategy should live.
		 *
		 * \return the new Strategy.
		 */
		virtual Strategy::Ptr create_strategy(const World::Ptr &world) const = 0;

	protected:
		/**
		 * Constructs a new StrategyFactory. Subclasses should call this
		 * constructor from their own constructors.
		 *
		 * \param[in] name a human-readable name for this Strategy.
		 *
		 * \param[in] handled_play_types a pointer to the first play type in an
		 * array of play types the corresponding Strategy is willing to handle;
		 * the array must be allocated in static memory and remain alive
		 * forever.
		 *
		 * \param[in] handled_play_types_size the number of elements in the \p
		 * handled_play_types array.
		 */
		StrategyFactory(const Glib::ustring &name, const PlayType::PlayType *handled_play_types, std::size_t handled_play_types_size);

		/**
		 * Destroys a StrategyFactory.
		 */
		~StrategyFactory();
};

#endif

