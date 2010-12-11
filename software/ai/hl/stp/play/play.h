#ifndef AI_HL_STP_PLAY_H
#define AI_HL_STP_PLAY_H

#include "ai/hl/world.h"
#include "ai/hl/stp/tactic/tactic.h"

#include "util/byref.h"
#include "util/registerable.h"
#include <cstddef>
#include <sigc++/sigc++.h>

namespace AI {
	namespace HL {
		namespace STP {
			/**
			 * A play is a level in the STP paradigm.
			 */
			class Play : public ByRef, public sigc::trackable {
				public:
					typedef RefPtr<Play> Ptr;

					/**
					 * Computes the roles.
					 *
					 * \param [out] tactics a vector of tactics.
					 *
					 * \param [out] active the active tactic.
					 */
					virtual void execute(std::vector<Tactic::Ptr> &tactics, Tactic::Ptr &active) = 0;

					/**
					 * Indicates how likely tactics assignments are changed.
					 *
					 * \return the probability that role assignment is changed.
					 * 0 to indicate no change.
					 */
					virtual double change_probability() const = 0;

					/**
					 * Check if the play has resigned.
					 */
					bool has_resigned() const;

				protected:
					/**
					 * The World in which the Play lives.
					 */
					AI::HL::W::World &world;

					Play(AI::HL::W::World &world);

					~Play();

					/**
					 * A subclass can invoke this function if it determines that it no longer wishes to control the team.
					 */
					void resign();

				private:
					bool has_resigned_;
			};

			class PlayManager : public ByRef, public Registerable<PlayManager> {
				public:
					typedef RefPtr<PlayManager> Ptr;

					/**
					 * A scoring function to indicate how much it wants to be selected.
					 * This function is ALWAYS checked before running the current play.
					 *
					 * \param[in] world the World in which the new Strategy should live.
					 *
					 * \param[in] running true if the previous play is the same play as what this manager handles.
					 *
					 * \return probability that this function is chosen.
					 * Must return between 0 and 1.
					 * 0 means do not select this at all.
					 * 1 guarantees this function is chosen.
					 *
					 */
					virtual double score(AI::HL::W::World &world, bool running) const = 0;

					/**
					 * Constructs a new instance of the Play corresponding to this PlayManager.
					 *
					 * \param[in] world the World in which the new Play should live.
					 *
					 * \return the new Play.
					 */
					virtual Play::Ptr create_play(AI::HL::W::World &world) const = 0;

				protected:
					/**
					 * Constructs a new PlayManager.
					 * Subclasses should call this constructor from their own constructors.
					 *
					 * \param[in] name a human-readable name for this Strategy.
					 */
					PlayManager(const char *name);

					~PlayManager();
			};
		}
	}
}

#endif

