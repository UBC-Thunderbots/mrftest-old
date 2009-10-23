#include "simulator/team.h"
#include "util/xml.h"
#include "world/config.h"
#include <sstream>
#include <algorithm>
#include <stdexcept>

simulator_team_data::simulator_team_data(xmlpp::Element *xml, bool yellow, ball::ptr ball, field::ptr field) : score(0), yellow(yellow), current_playtype(playtype::halt), controller_factory(0), west_view(new simulator_team_view(west_players, score, west_other, this->yellow)), east_view(new simulator_team_view(east_players, score, east_other, this->yellow)), xml(xml), the_ball(ball), the_field(field) {
	// Get the "players" attribute.
	const Glib::ustring &players_string = xml->get_attribute_value("players");
	unsigned int players = 0;
	if (!players_string.empty()) {
		std::istringstream iss(players_string);
		iss >> players;
	}

	// Create the objects for the players.
	for (unsigned int i = 0; i < players; i++)
		add_player();

	// Get the "strategies" element.
	xmlpp::Element *xmlstrategies = xmlutil::get_only_child(xml, "strategies");

	// Get the "active" attribute.
	const Glib::ustring &strat_name = xmlstrategies->get_attribute_value("active");

	// Set this strategy.
	set_strategy(strat_name);

	// Get the "controller" attribute.
	const Glib::ustring &controller_name = xml->get_attribute_value("controller");

	// Set this controller.
	set_controller_type(controller_name);
}

void simulator_team_data::set_engine(simulator_engine::ptr e) {
	// Remember how many objects have been created.
	unsigned int num_players = impls.size();

	// Delete old objects.
	if (engine)
		for (unsigned int i = 0; i < impls.size(); i++)
			engine->remove_player(impls[i]);
	impls.clear();
	west_players.clear();
	east_players.clear();

	// Set new engine.
	engine = e;

	// Create new objects.
	for (unsigned int i = 0; i < num_players; i++)
		add_player();
}

void simulator_team_data::set_strategy(const Glib::ustring &name) {
	// Get the "strategies" XML element.
	xmlpp::Element *xmlstrategies = xmlutil::get_only_child(xml, "strategies");

	// Find the strategy factory for this strategy type.
	const strategy_factory::map_type &factories = strategy_factory::all();
	strategy_factory::map_type::const_iterator factoryiter = factories.find(name);
	strategy::ptr strat;
	if (factoryiter != factories.end()) {
		strategy_factory *factory = factoryiter->second;
		xmlpp::Element *xmlparams = xmlutil::strip(xmlutil::get_only_child_keyed(xmlstrategies, "params", "strategy", name));
		strat = factory->create_strategy(xmlparams, the_ball, the_field, yellow ? west_view : east_view);
	}

	// Lock in the strategy.
	team_strategy = strat;
	if (team_strategy)
		team_strategy->set_playtype(current_playtype);

	// Save the choice of engine in the configuration.
	if (xmlstrategies->get_attribute_value("active") != name) {
		xmlstrategies->set_attribute("active", name);
		config::dirty();
	}
}

void simulator_team_data::set_controller_type(const Glib::ustring &name) {
	// Find the factory with this name.
	const robot_controller_factory::map_type &m = robot_controller_factory::all();
	robot_controller_factory::map_type::const_iterator i = m.find(name);
	if (i != m.end()) {
		controller_factory = i->second;
	} else {
		controller_factory = 0;
	}

	// Assign new controllers to all the bots.
	for (unsigned int i = 0; i < impls.size(); i++) {
		if (controller_factory) {
			const Glib::ustring &name = Glib::ustring::compose("%1 %2", yellow ? "Yellow" : "Blue", i);
			impls[i]->set_controller(controller_factory->create_controller(name));
		} else {
			impls[i]->set_controller(robot_controller::ptr());
		}
	}

	// Save the choice of controller in the configuration.
	if (xml->get_attribute_value("controller") != name) {
		xml->set_attribute("controller", name);
		config::dirty();
	}
}

void simulator_team_data::add_player() {
	// Create the new objects.
	player_impl::ptr impl(engine ? engine->add_player() : player_impl::trivial());
	player::ptr wplr(new player(impl, false));
	player::ptr eplr(new player(impl, true));

	// Set the robot controller.
	if (controller_factory) {
		const Glib::ustring &name = Glib::ustring::compose("%1 %2", yellow ? "Yellow" : "Blue", impls.size());
		impl->set_controller(controller_factory->create_controller(name));
	}

	// Insert the new data into the arrays.
	impls.push_back(impl);
	west_players.push_back(wplr);
	east_players.push_back(eplr);

	// Update the XML.
	xml->set_attribute("players", Glib::ustring::compose("%1", impls.size()));
	config::dirty();

	// Send notifications.
	west_view->signal_robot_added().emit();
	east_view->signal_robot_added().emit();
}

void simulator_team_data::remove_player(unsigned int index) {
	// Grab a reference to the players before clearing the arrays.
	robot::ptr west_bot = west_players[index];
	robot::ptr east_bot = east_players[index];

	// Delete the objects.
	impls.erase(impls.begin() + index);
	west_players.erase(west_players.begin() + index);
	east_players.erase(east_players.begin() + index);

	// Update the XML.
	xml->set_attribute("players", Glib::ustring::compose("%1", impls.size()));
	config::dirty();

	// Send notifications.
	west_view->signal_robot_removed().emit(index, west_bot);
	east_view->signal_robot_removed().emit(index, east_bot);
}

