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
				AI::RC::W::World &world;

				/**
				 * \brief The player to control.
				 */
				const AI::RC::W::Player::Ptr player;

				/**
				 * \brief Constructs a new RobotController.
				 *
				 * \param[in] world the world in which the controller will run.
				 *
				 * \param[in] player the player to control.
				 */
				RobotController(AI::RC::W::World &world, AI::RC::W::Player::Ptr player);
		};

		/**
		 * \brief A compatibility layer for using old controllers that accept only a point as input.
		 *
		 * Not intended for new code.
		 */
		class OldRobotController2 : public RobotController {
			public:
				/**
				 * \brief Tells the robot controlled by this controller to move to the specified target location and orientation.
				 *
				 * It is expected that this function will update internal state.
				 * This function will be called exactly once per timer tick, except for those ticks in which clear() is called instead.
				 *
				 * \param[in] new_position the position to move to, in team coordinates measured in metres.
				 *
				 * \param[in] new_orientation the orientation to rotate to in team coordinates measured in radians.
				 *
				 * \param[out] wheel_speeds the speeds of the four wheels to send to the robot,
				 * in quarters of a degree of motor shaft rotation per five milliseconds.
				 */
				virtual void move(const Point &new_position, Angle new_orientation, int(&wheel_speeds)[4]) = 0;

				/**
				 * \brief Tells the controller to clear its internal state because the robot under control is scrammed.
				 *
				 * The controller should clear any integrators and similar structures in order to prevent unexpected jumps when driving resumes.
				 */
				virtual void clear() = 0;

			protected:
				/**
				 * \brief Constructs a new OldRobotController2.
				 *
				 * \param[in] world the world in which the controller will run.
				 *
				 * \param[in] player the player to control.
				 */
				OldRobotController2(AI::RC::W::World &world, AI::RC::W::Player::Ptr player);

			private:
				void tick();
		};

		/**
		 * \brief A compatibility layer for using very old controllers that produce output as linear and angular velocities rather than calculating wheel speeds directly.
		 *
		 * Not intended for new code.
		 */
		class OldRobotController : public OldRobotController2 {
			public:
				/**
				 * \brief Tells the robot controlled by this controller to move to the specified target location and orientation.
				 *
				 * It is expected that this function will update internal state.
				 * This function will be called exactly once per timer tick, except for those ticks in which clear() is called instead.
				 *
				 * \param[in] new_position the position to move to, in team coordinates measured in metres.
				 *
				 * \param[in] new_orientation the orientation to rotate to in team coordinates measured in radians.
				 *
				 * \param[out] linear_velocity the linear velocity to move at, in robot-relative coordinates
				 * (defined as the positive X axis being forward and the positive Y axis being left).
				 *
				 * \param[out] angular_velocity the angular velocity to rotate at, with positive being counter-clockwise.
				 */
				virtual void move(const Point &new_position, Angle new_orientation, Point &linear_velocity, Angle &angular_velocity) = 0;

			protected:
				/**
				 * \brief Constructs a new OldRobotController.
				 *
				 * \param[in] world the world in which the controller will run.
				 *
				 * \param[in] player the player to control.
				 */
				OldRobotController(AI::RC::W::World &world, AI::RC::W::Player::Ptr player);

			private:
				void move(const Point &new_position, Angle new_orientation, int(&wheel_speeds)[4]);
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
				virtual std::unique_ptr<RobotController> create_controller(AI::RC::W::World &world, AI::RC::W::Player::Ptr plr) const = 0;

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
				RobotControllerFactory(const char *name);
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
		class cls##ControllerFactory : public RobotControllerFactory { \
			public: \
				cls##ControllerFactory(); \
				std::unique_ptr<RobotController> create_controller(World &, Player::Ptr) const; \
		}; \
	} \
	\
	cls##ControllerFactory::cls##ControllerFactory() : RobotControllerFactory(#cls) { \
	} \
	\
	std::unique_ptr<RobotController> cls##ControllerFactory::create_controller(World &w, Player::Ptr p) const { \
		std::unique_ptr<RobotController> ptr(new cls(w, p)); \
		return ptr; \
	} \
	\
	cls##ControllerFactory cls##ControllerFactory_instance;

#endif

