#ifndef AI_BACKEND_SIMULATOR_FIELD_H
#define AI_BACKEND_SIMULATOR_FIELD_H

#include "ai/backend/backend.h"

namespace AI {
	namespace BE {
		namespace Simulator {
			class Field : public AI::BE::Field {
				public:
					/**
					 * Constructs a new Field.
					 */
					Field() {
					}

					/**
					 * Destroys a Field.
					 */
					~Field() {
					}

					bool valid() const { return true; }
					double length() const { return 6.05; }
					double total_length() const { return 7.40; }
					double width() const { return 4.05; }
					double total_width() const { return 5.40; }
					double goal_width() const { return 0.70; }
					double centre_circle_radius() const { return 0.50; }
					double defense_area_radius() const { return 0.50; }
					double defense_area_stretch() const { return 0.35; }
			};
		}
	}
}

#endif

