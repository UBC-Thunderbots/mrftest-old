#ifndef UTIL_CONFIG_H
#define UTIL_CONFIG_H

#include "util/noncopyable.h"
#include <glibmm.h>
#include <map>
#include <stdint.h>
#include <vector>

namespace xmlpp {
	class Element;
}

/**
 * Provides access to the configuration file.
 */
class Config : public NonCopyable {
	public:
		/**
		 * The configuration of a single robot.
		 */
		struct RobotInfo {
			/**
			 * The robot's XBee address.
			 */
			uint64_t address;

			/**
			 * The index of the robot's lid pattern in the SSL-Vision pattern file.
			 */
			unsigned int pattern;

			/**
			 * Constructs a new RobotInfo.
			 *
			 * \param[in] address the robot's XBee address.
			 *
			 * \param[in] pattern the index in the SSL-Vision pattern file of the robot's lid pattern.
			 */
			RobotInfo(uint64_t address, unsigned int pattern) : address(address), pattern(pattern) {
			}
		};

		/**
		 * A collection of RobotInfo objects.
		 */
		class RobotSet : public NonCopyable {
			public:
				/**
				 * Gets the size of the list.
				 *
				 * \return the number of robots in the list.
				 */
				unsigned int size() const {
					return robots.size();
				}

				/**
				 * Fetches a robot from the list.
				 *
				 * \param[in] index the position of the robot in the list.
				 *
				 * \return the robot at that position.
				 */
				const RobotInfo &operator[](unsigned int index) const {
					return robots[index];
				}

				/**
				 * Finds a robot by 64-bit address.
				 *
				 * \param[in] address the XBee address to look up.
				 *
				 * \return the robot's information structure.
				 */
				const RobotInfo &find(uint64_t address) const;

				/**
				 * Checks whether or not there is a robot with a 64-bit address.
				 *
				 * \param[in] address the address to check.
				 *
				 * \return \c true if some robot in the collection has the address \p address, or \c false if not.
				 */
				bool contains_address(uint64_t address) const;

				/**
				 * Checks whether or not there is a robot with a lid pattern.
				 *
				 * \param[in] pattern the pattern index to look up.
				 *
				 * \return \c true if a robot in the ocllection has the given colour and pattern index, or \c false if not.
				 */
				bool contains_pattern(unsigned int pattern) const;

				/**
				 * Adds a new robot.
				 *
				 * \param[in] address the robot's XBee address.
				 *
				 * \param[in] pattern the index of the robot's lid pattern.
				 */
				void add(uint64_t address, unsigned int pattern);

				/**
				 * Emitted when a robot is added.
				 * Parameter is the index of the robot.
				 */
				mutable sigc::signal<void, unsigned int> signal_robot_added;

				/**
				 * Deletes a robot.
				 *
				 * \param[in] address the XBee address of the robot to delete.
				 */
				void remove(uint64_t address);

				/**
				 * Emitted when a robot is deleted.
				 * Parameter is the index of the robot.
				 */
				mutable sigc::signal<void, unsigned int> signal_robot_removed;

				/**
				 * Replaces an existing robot with new data.
				 *
				 * \param[in] old_address the XBee address of the robot to replace.
				 *
				 * \param[in] address the new XBee address to store.
				 *
				 * \param[in] pattern the new lid pattern index.
				 */
				void replace(uint64_t old_address, uint64_t address, unsigned int pattern);

				/**
				 * Emitted when a robot is replaced.
				 * Parameter is the index of the robot.
				 */
				mutable sigc::signal<void, unsigned int> signal_robot_replaced;

				/**
				 * Sorts the robots in the collection by their 64-bit address.
				 */
				void sort_by_address();

				/**
				 * Sorts the robots in the collection by their lid pattern.
				 */
				void sort_by_lid();

				/**
				 * Emitted when the collection is sorted.
				 */
				mutable sigc::signal<void> signal_sorted;

			private:
				std::vector<RobotInfo> robots;

				void load(const xmlpp::Element *players);
				void save(xmlpp::Element *players) const;

				friend class Config;
		};

		/**
		 * Loads the configuration file.
		 */
		Config();

		/**
		 * Saves the configuration file.
		 */
		void save() const;

		/**
		 * Gets the set of configured robots.
		 *
		 * \return the set of configured robots.
		 */
		const RobotSet &robots() const {
			return robots_;
		}

		/**
		 * Gets the set of configured robots.
		 *
		 * \return the set of configured robots.
		 */
		RobotSet &robots() {
			return robots_;
		}

		/**
		 * Gets the radio channel.
		 *
		 * \return the radio channel.
		 */
		unsigned int channel() const {
			return channel_;
		}

		/**
		 * Sets the radio channel.
		 *
		 * \param[in] chan the new channel, which must be between \c 0x0B and \c 0x1A.
		 */
		void channel(unsigned int chan);

		/**
		 * The collection of tunable booleans, by name.
		 */
		std::map<Glib::ustring, bool> bool_params;

		/**
		 * The collection of tunable integers, by name.
		 */
		std::map<Glib::ustring, int> int_params;

		/**
		 * The collection of tunable doubles, by name.
		 */
		std::map<Glib::ustring, double> double_params;

	private:
		RobotSet robots_;
		unsigned int channel_;
};

#endif

