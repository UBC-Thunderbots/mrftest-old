#ifndef AI_ROBOT_CONTROLLER_ROBOT_CONTROLLER_H
#define AI_ROBOT_CONTROLLER_ROBOT_CONTROLLER_H

#include "ai/robot_controller/world.h"
#include "geom/point.h"
#include "util/noncopyable.h"
#include "util/registerable.h"
#include <memory>
#include <cairomm/context.h>
#include <cairomm/refptr.h>

namespace Gtk {
	class Widget;
}

namespace AI {
	class Player;

	namespace RC {
		class RobotControllerFactory;

		/**
		 * \brief Translates world-coordinate movement requests into robot wheel rotation speeds.
		 *
		 * Implementations of this class must also contain a public constructor with the same signature as the RobotController constructor itself.
		 */
		class RobotController : public NonCopyable {
			public:
				/**
				 * \brief The matrix which converts a 3-vector of (X, Y, T) coordinates into a 4-vector of wheel speeds.
				 */
				static const double WHEEL_MATRIX[4][3];

				/**
				 * \brief The pseudo-inverse of \ref WHEEL_MATRIX.
				 */
				static const double WHEEL_MATRIX_PINV[3][4];

				/**
				 * \brief Multiplies a robot-relative velocity tuple by the wheel matrix, producing a set of wheel rotation speeds.
				 *
				 * A robot controller implementation may call this function.
				 * It may also complete ignore it and compute wheel speeds in a different way.
				 *
				 * \param[in] vel the linear velocity to multiply, in metres per second.
				 *
				 * \param[in] avel the angular velocity to multiply, in radians per second.
				 *
				 * \param[out] wheel_speeds the wheel speeds,
				 * in quarters of a degree of motor shaft rotation per five milliseconds.
				 */
				static void convert_to_wheels(const Point &vel, Angle avel, int(&wheel_speeds)[4]);

				/**
				 * \brief Destroys the RobotController.
				 */
				virtual ~RobotController();

				/**
				 * \brief Reads the requested path from the Player using W::Player::path, then orders new wheel speeds using W::Player::drive.
				 */
				virtual void tick() = 0;

				/**
				 * \brief Provides an opportunity for the controller to draw an overlay on the visualizer.
				 *
				 * The default implementation does nothing.
				 * A subclass should override this function if it wishes to draw an overlay.
				 *
				 * \param[in] ctx the Cairo context onto which to draw.
				 */
				virtual void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx);

			protected:
				/**
				 * \brief The world in which the controller runs.
				 */
				AI::RC::W::World world;

				/**
				 * \brief The player to control.
				 */
				const AI::RC::W::Player player;

				/**
				 * \brief Constructs a new RobotController.
				 *
				 * \param[in] world the world in which the controller will run.
				 *
				 * \param[in] player the player to control.
				 */
				explicit RobotController(AI::RC::W::World world, AI::RC::W::Player player);
		};

		/**
		 * \brief A factory to construct \ref RobotController "RobotControllers".
		 */
		class RobotControllerFactory : public Registerable<RobotControllerFactory> {
			public:
				/**
				 * \brief Constructs a new RobotController.
				 *
				 * \param[in] world the world in which the controller will run.
				 *
				 * \param[in] plr the robot being controlled.
				 *
				 * \return the new controller.
				 */
				virtual std::unique_ptr<RobotController> create_controller(AI::RC::W::World world, AI::RC::W::Player plr) const = 0;

				/**
				 * \brief Returns the GTK widget for this RobotControllerFactory, which will be integrated into the AI's user interface.
				 *
				 * \return a GUI widget containing the controls for this RobotControllerFactory,
				 * or a null pointer if no GUI widgets are needed for this RobotControllerFactory.
				 *
				 * \note The default implementation returns a null pointer.
				 */
				virtual Gtk::Widget *ui_controls();

			protected:
				/**
				 * \brief Constructs a RobotControllerFactory.
				 *
				 * This is intended to be called from a subclass constructor.
				 * The subclass should be created by a global variable coming into scope at application startup.
				 *
				 * \param[in] name a human-readable name for the factory.
				 */
				explicit RobotControllerFactory(const char *name);
		};
	}
}

/**
 * \brief Registers a RobotController implementation.
 *
 * \param[in] cls the class of the controller to register.
 */
#define ROBOT_CONTROLLER_REGISTER(cls) \
	namespace { \
		class cls##ControllerFactory : public AI::RC::RobotControllerFactory { \
			public: \
				explicit cls##ControllerFactory(); \
				std::unique_ptr<RobotController> create_controller(World, Player) const; \
		}; \
	} \
	\
	cls##ControllerFactory::cls##ControllerFactory() : RobotControllerFactory(#cls) { \
	} \
	\
	std::unique_ptr<RobotController> cls##ControllerFactory::create_controller(World w, Player p) const { \
		std::unique_ptr<RobotController> ptr(new cls(w, p)); \
		return ptr; \
	} \
	\
	cls##ControllerFactory cls##ControllerFactory_instance;

#endif

