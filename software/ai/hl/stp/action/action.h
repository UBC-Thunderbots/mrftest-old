#pragma once

#include "ai/hl/stp/world.h"
#include "ai/hl/stp/tactic/tactic.h"

namespace Actions = AI::BE::Primitives;
using caller_t = AI::HL::STP::Tactic::Tactic::caller_t;
using Primitives::Primitive;

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				inline void yield(caller_t& ca) {
					ca();
				}

				inline void wait(caller_t& ca, const Primitive& prim) {
					while (!prim.done()) {
						yield(ca);
					}
				}

                inline Point local_dest(Player player, Point dest) {
                    Point pos_diff = dest - player.position();
                    Point robot_local_dest = pos_diff.rotate(-player.orientation());
                    return robot_local_dest;
                }

                class Action : public NonCopyable {
                    public:
                        using Ptr = std::unique_ptr<Action>;

                        Action() = delete;

                        /**
                         * \brief Constructs a primitive with the given arguments.
                         */
                        Action(AI::Common::Player player);

                        /**
                         * \brief Destroys the Action, removing it from the queue.
                         */
                        virtual ~Action();

                        /**
                         * \brief Checks if the primitive is done.
                         */
                        bool done() const;
                        void done(bool value);

                        /**
                         * \brief Checks if the primitive is being processed.
                         *
                         * The Navigator must set this flag.
                         */
                        bool active() const;
                        void active(bool value);

                        const ActionDescriptor& desc() const;


                        /**
                         * \brief Gets the error thrown by this primitive, if it failed.
                         * If this primitive has not failed, error() returns a nullptr.
                         */
                        std::exception_ptr error() const;

                    private:
                        std::unique_ptr<std::exception> error_;
                        AI::Common::Player player_;
                        bool done_;
                        bool active_;
                        bool moved_;
                };

                inline bool Action::done() const {
                    return done_;
                }

                inline void Action::done(bool value) {
                    done_ = value;
                }

                inline bool Action::active() const {
                    return active_;
                }

                inline void Action::active(bool value) {
                    active_ = value;
                }

                inline std::exception_ptr Action::error() const {
                    return std::make_exception_ptr(*error_);
                }
			}
		}
	}
}
