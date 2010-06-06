#ifndef UTIL_CONFIG_H
#define UTIL_CONFIG_H

#include <istream>
#include <ostream>
#include <vector>
#include <glibmm.h>
#include <stdint.h>
#include "util/noncopyable.h"

//
// Provides access to the configuration file.
//
class config : public noncopyable {
	public:
		//
		// The configuration of a single robot.
		//
		struct robot_info {
			uint64_t address;
			bool yellow;
			unsigned int pattern_index;
			Glib::ustring name;

			robot_info() {
			}

			robot_info(uint64_t address, bool yellow, unsigned int pattern_index, const Glib::ustring &name) : address(address), yellow(yellow), pattern_index(pattern_index), name(name) {
			}
		};

		//
		// A collection of robot_info objects.
		//
		class robot_set : public noncopyable {
			public:
				//
				// Returns the number of robots in the list.
				//
				unsigned int size() const {
					return robots.size();
				}

				//
				// Gets a robot by its position in the list.
				//
				const robot_info &operator[](unsigned int index) const {
					return robots[index];
				}

				//
				// Gets a robot by its 64-bit address.
				//
				const robot_info &find(uint64_t address) const;

				//
				// Checks whether a particular address is already in the list.
				//
				bool contains_address(uint64_t address) const;

				//
				// Checks whether a particular vision pattern is already in the
				// list.
				//
				bool contains_pattern(bool yellow, unsigned int pattern_index) const;

				/**
				 * \param[in] name the name to look for.
				 *
				 * \return \c true if \p name is already used by a robot in this
				 * collection, or \c false if not.
				 */
				bool contains_name(const Glib::ustring &name) const;

				//
				// Adds a new robot.
				//
				void add(uint64_t address, bool yellow, unsigned int pattern_index, const Glib::ustring &name);

				//
				// Emitted when a robot is added. Parameter is the index of the
				// robot.
				//
				mutable sigc::signal<void, unsigned int> signal_robot_added;

				//
				// Deletes a robot.
				//
				void remove(uint64_t address);

				//
				// Emitted when a robot is deleted. Parameter is the index of
				// the robot.
				//
				mutable sigc::signal<void, unsigned int> signal_robot_removed;

			private:
				std::vector<robot_info> robots;

				void save(std::ostream &ofs) const;
				void load_v1(std::istream &ifs);

				friend class config;
		};

		//
		// Loads the configuration file.
		//
		config();

		//
		// Saves the configuration file.
		//
		void save() const;

		//
		// Returns the set of configured robots.
		//
		const robot_set &robots() const {
			return robots_;
		}

		//
		// Returns the set of configured robots.
		//
		robot_set &robots() {
			return robots_;
		}

	private:
		robot_set robots_;

		void load_v1(std::istream &ifs);
};

#endif

