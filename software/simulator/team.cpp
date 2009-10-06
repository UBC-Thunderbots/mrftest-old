#include "simulator/team.h"
#include "util/xml.h"
#include "world/config.h"
#include <sstream>
#include <algorithm>
#include <stdexcept>

simulator_team_data::simulator_team_data(xmlpp::Element *xml, bool yellow) : score(0), yellow(yellow), current_playtype(playtype::halt), controller_factory(0), west_view(new simulator_team_view(west_players, score, west_other, yellow)), east_view(new simulator_team_view(east_players, score, east_other, yellow)), xml(xml) {
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

void simulator_team_data::set_strategy(const Glib::ustring &name, ball::ptr ball, field::ptr field) {
	// Get the "engines" XML element.
	xmlpp::Element *xmlstrategies = xmlutil::get_only_child(xml, "strategies");

	// Find the strategy factory for this strategy type.
	const strategy_factory::map_type &factories = strategy_factory::all();
	strategy_factory::map_type::const_iterator factoryiter = factories.find(name);
	strategy::ptr strat;
	if (factoryiter != factories.end()) {
		strategy_factory *factory = factoryiter->second;
		xmlpp::Element *xmlparams = xmlutil::strip(xmlutil::get_only_child_keyed(xmlstrategies, "params", "strategy", name));
		strat = factory->create_strategy(xmlparams, ball, field, yellow ? west_view : east_view);
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

void simulator_team_data::set_controller_type(robot_controller_factory *cf) {
	controller_factory = cf;

	for (unsigned int i = 0; i < west_players.size(); i++) {
		if (controller_factory) {
			impls[i]->set_controller(controller_factory->create_controller());
		} else {
			impls[i]->set_controller(robot_controller::ptr());
		}
	}
}

void simulator_team_data::add_player() {
	// Create the new objects.
	player_impl::ptr impl(engine ? engine->add_player() : player_impl::trivial());
	player::ptr wplr(new player(impl, false));
	player::ptr eplr(new player(impl, true));

	// Set the robot controller.
	if (controller_factory) {
		impl->set_controller(controller_factory->create_controller());
	}

	// Insert the new data into the arrays.
	impls.push_back(impl);
	west_players.push_back(wplr);
	east_players.push_back(eplr);

	// Update the XML.
	xml->set_attribute("players", Glib::ustring::compose("%1", impls.size()));
	config::dirty();
}

void simulator_team_data::remove_player(unsigned int index) {
	// Delete the objects.
	impls.erase(impls.begin() + index);
	west_players.erase(west_players.begin() + index);
	east_players.erase(east_players.begin() + index);

	// Update the XML.
	xml->set_attribute("players", Glib::ustring::compose("%1", impls.size()));
	config::dirty();
}

