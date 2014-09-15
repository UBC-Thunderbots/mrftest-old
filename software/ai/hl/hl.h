#ifndef AI_HL_HL_H
#define AI_HL_HL_H

#include "ai/hl/world.h"
#include "util/noncopyable.h"
#include "util/property.h"
#include "util/registerable.h"
#include <cairomm/context.h>
#include <cairomm/refptr.h>
#include <memory>

namespace Gtk {
	//forward declare the widget pointer
	class Widget;
}

namespace AI {
	namespace HL {
		class HighLevelFactory;

		/**
		 * \brief A complete AI structure.
		 */
		class HighLevel : public NonCopyable {
			public:
				/**
				 * \brief Textual information about the current high level.
				 */
				Property<Glib::ustring> ai_notes;

				/**
				 * \brief Destroys the HighLevel.
				 */
				virtual ~HighLevel();

				/**
				 * \brief Finds the HighLevelFactory that constructed this HighLevel.
				 *
				 * Subclasses must override this function to return a reference to the global instance of their corresponding HighLevelFactory.
				 *
				 * \return a reference to the HighLevelFactory instance.
				 */
				virtual HighLevelFactory &factory() const = 0;

				/**
				 * \brief Runs one time tick on this high-level.
				 */
				virtual void tick() = 0;

				/**
				 * \brief Returns the user interface controls for this high-level.
				 *
				 * \return the high-level's UI controls.
				 */
				virtual Gtk::Widget *ui_controls() = 0;

				/**
				 * \brief Provides an opportunity for the high level to draw an overlay on the visualizer.
				 *
				 * The default implementation does nothing.
				 * A subclass should override this function if it wishes to draw an overlay.
				 *
				 * \param[in] ctx the Cairo context onto which to draw.
				 */
				virtual void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx);

			protected:
				/**
				 * \brief Constructs a new high level.
				 */
				explicit HighLevel();
		};

		/**
		 * \brief A factory used to construct a particular type of HighLevel.
		 */
		class HighLevelFactory : public Registerable<HighLevelFactory> {
			public:
				/**
				 * \brief Constructs the corresponding HighLevel.
				 *
				 * \param[in] world the world in which the HighLevel will operate.
				 *
				 * \return the new HighLevel.
				 */
				virtual std::unique_ptr<HighLevel> create_high_level(AI::HL::W::World world) const = 0;

			protected:
				/**
				 * \brief Constructs a new HighLevelFactory.
				 *
				 * \param[in] name a human-readable name for this HighLevel.
				 */
				explicit HighLevelFactory(const char *name) : Registerable<HighLevelFactory>(name) {
				}
		};
	}
}

#define HIGH_LEVEL_REGISTER(cls) \
	namespace { \
		class cls##HighLevelFactory : public AI::HL::HighLevelFactory { \
			public: \
				cls##HighLevelFactory(); \
				std::unique_ptr<HighLevel> create_high_level(AI::HL::W::World) const; \
		}; \
	} \
	\
	cls##HighLevelFactory::cls##HighLevelFactory() : HighLevelFactory(#cls) { \
	} \
	\
	std::unique_ptr<HighLevel> cls##HighLevelFactory::create_high_level(AI::HL::W::World world) const { \
		std::unique_ptr<HighLevel> p(new cls(world)); \
		return p; \
	} \
	\
	cls##HighLevelFactory cls##HighLevelFactory_instance; \
	\
	HighLevelFactory &cls::factory() const { \
		return cls##HighLevelFactory_instance; \
	}

#endif

