#ifndef AI_NAVIGATOR_NAVIGATOR_H
#define AI_NAVIGATOR_NAVIGATOR_H

#include "ai/navigator/world.h"
#include "geom/point.h"
#include "util/byref.h"
#include "util/registerable.h"
#include <glibmm.h>
#include <map>
#include <utility>

namespace Gtk {
	class Widget;
}

namespace AI {
	namespace Nav {
		class NavigatorFactory;

		/**
		 * A Navigator path-plans a team on behalf of a strategy.
		 *
		 * To implement a Navigator, one must:
		 * <ul>
		 * <li>Subclass Navigator</li>
		 * <li>In the subclass, override all the pure virtual functions</li>
		 * <li>Subclass NavigatorFactory</li>
		 * <li>In the subclass, override all the pure virtual functions</li>
		 * <li>Create an instance of the NavigatorFactory in the form of a file-scope global variable</li>
		 * </ul>
		 */
		class Navigator : public ByRef {
			public:
				/**
				 * A pointer to a Navigator object.
				 */
				typedef RefPtr<Navigator> Ptr;

				/**
				 * Finds the NavigatorFactory that constructed this Navigator.
				 * Subclasses must override this function to return a reference to the global instance of their corresponding NavigatorFactory.
				 *
				 * \return a reference to the NavigatorFactory instance.
				 */
				virtual NavigatorFactory &factory() const = 0;

				/**
				 * Reads the requested destinations from the players using W::Player::destination and W::Player::flags,
				 * then orders paths to be followed with W::Player::path.
				 */
				virtual void tick() = 0;

				/**
				 * Returns the GTK widget for this Navigator, which will be integrated into the AI's user interface.
				 *
				 * \return a GUI widget containing the controls for this Navigator,
				 * or a null pointer if no GUI widgets are needed for this Navigator.
				 *
				 * \note The default implementation returns a null pointer.
				 */
				virtual Gtk::Widget *ui_controls();

			protected:
				/**
				 * The World in which to navigate.
				 */
				AI::Nav::W::World &world;

				/**
				 * Constructs a new Navigator.
				 *
				 * \param[in] world the World in which to navigate.
				 */
				Navigator(AI::Nav::W::World &world);
		};

		/**
		 * A NavigatorFactory is used to construct a particular type of Navigator.
		 * The factory permits the UI to discover all the available types of Navigator.
		 */
		class NavigatorFactory : public Registerable<NavigatorFactory> {
			public:
				/**
				 * Constructs a new instance of the Navigator corresponding to this NavigatorFactory.
				 *
				 * \param[in] world the World in which the new Navigator should live.
				 *
				 * \return the new Navigator.
				 */
				virtual Navigator::Ptr create_navigator(AI::Nav::W::World &world) const = 0;

			protected:
				/**
				 * Constructs a new NavigatorFactory.
				 * Subclasses should call this constructor from their own constructors.
				 *
				 * \param[in] name a human-readable name for this Navigator.
				 */
				NavigatorFactory(const char *name);
		};
	}
}

#endif

