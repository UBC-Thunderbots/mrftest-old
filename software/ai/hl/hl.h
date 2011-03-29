#ifndef AI_HL_HL_H
#define AI_HL_HL_H

#include "ai/hl/world.h"
#include "util/byref.h"
#include "util/registerable.h"
#include <gtkmm.h>

namespace AI {
	namespace HL {
		class HighLevelFactory;

		/**
		 * A complete AI structure.
		 */
		class HighLevel : public ByRef {
			public:
				/**
				 * A pointer to a HighLevel.
				 */
				typedef RefPtr<HighLevel> Ptr;

				/**
				 * Finds the HighLevelFactory that constructed this HighLevel.
				 * Subclasses must override this function to return a reference to the global instance of their corresponding HighLevelFactory.
				 *
				 * \return a reference to the HighLevelFactory instance.
				 */
				virtual HighLevelFactory &factory() const = 0;

				/**
				 * Runs one time tick on this high-level.
				 */
				virtual void tick() = 0;

				/**
				 * Returns the user interface controls for this high-level.
				 *
				 * \return the high-level's UI controls.
				 */
				virtual Gtk::Widget *ui_controls() = 0;

				/**
				 * Provides an opportunity for the AI to draw an overlay on the visualizer.
				 *
				 * \param[in] ctx the Cairo context onto which to draw.
				 */
				virtual void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) = 0;
		};

		/**
		 * A factory used to construct a particular type of HighLevel.
		 */
		class HighLevelFactory : public Registerable<HighLevelFactory> {
			public:
				/**
				 * Constructs the corresponding HighLevel.
				 *
				 * \param[in] world the world in which the HighLevel will operate.
				 *
				 * \return the new HighLevel.
				 */
				virtual HighLevel::Ptr create_high_level(AI::HL::W::World &world) const = 0;

			protected:
				/**
				 * Constructs a new HighLevelFactory.
				 *
				 * \param[in] name a human-readable name for this HighLevel.
				 */
				HighLevelFactory(const char *name) : Registerable<HighLevelFactory>(name) {
				}
		};
	}
}

#endif

