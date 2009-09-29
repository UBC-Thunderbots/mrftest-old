#include "simulator/team.h"
#include "util/xml.h"
#include "world/config.h"
#include <sstream>
#include <algorithm>
#include <stdexcept>

simulator_team_data::simulator_team_data(xmlpp::Element *xml, bool yellow) : score(0), yellow(yellow), current_playtype(playtype::halt), controller_factory(0), west_view(new simulator_team_view(west_players, score, west_other, yellow)), east_view(new simulator_team_view(east_players, score, east_other, yellow)), xml(xml) {
	// Get the "players" element.
	xmlpp::Element *xmlplayers = xmlutil::get_only_child(xml, "players");

	// Iterate the child nodes.
	const xmlpp::Node::NodeList &players = xmlplayers->get_children();
	for (xmlpp::Node::NodeList::const_iterator i = players.begin(), iend = players.end(); i != iend; ++i) {
		xmlpp::Node *node = *i;

		// It should be an element of type "player".
		xmlpp::Element *elem = dynamic_cast<xmlpp::Element *>(node);
		if (elem && elem->get_name() == "player") {
			// It should have an "id" attribute.
			const Glib::ustring &id_string = elem->get_attribute_value("id");
			if (!id_string.empty()) {
				// Convert the ID to an integer.
				std::istringstream iss(id_string);
				unsigned int id;
				iss >> id;

				// Store the ID.
				ids.push_back(id);
			} else {
				xmlplayers->remove_child(node);
				config::dirty();
			}
		} else {
			xmlplayers->remove_child(node);
			config::dirty();
		}
	}

	// Sort the ID numbers.
	std::sort(ids.begin(), ids.end());

	// Create the other objects for the players.
	for (unsigned int i = 0; i < ids.size(); i++) {
		// Create the objects.
		player_impl::ptr impl(player_impl::trivial());
		player::ptr wplr(new player(ids[i], impl, false));
		player::ptr eplr(new player(ids[i], impl, true));

		// Store the objects.
		impls.push_back(player_impl::trivial());
		west_players.push_back(wplr);
		east_players.push_back(eplr);
	}
}

void simulator_team_data::set_engine(simulator_engine::ptr e) {
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
	for (unsigned int i = 0; i < ids.size(); i++) {
		unsigned int id = ids[i];
		player_impl::ptr p;
		if (engine) {
			p = engine->add_player();
		} else {
			p = player_impl::trivial();
		}
		player::ptr w(new player(id, p, false));
		player::ptr e(new player(id, p, true));
		impls.push_back(p);
		west_players.push_back(w);
		east_players.push_back(e);
	}
}

void simulator_team_data::set_strategy(const Glib::ustring &name) {
	// Get the "engines" XML element.
	xmlpp::Element *xmlstrategies = xmlutil::get_only_child(xml, "strategies");

	// Find the strategy factory for this strategy type.
	const strategy_factory::map_type &factories = strategy_factory::all();
	strategy_factory::map_type::const_iterator factoryiter = factories.find(name);
	strategy::ptr strat;
	if (factoryiter != factories.end()) {
		strategy_factory *factory = factoryiter->second;
		xmlpp::Element *xmlparams = xmlutil::strip(xmlutil::get_only_child_keyed(xmlstrategies, "params", "strategy", name));
		strat = factory->create_strategy(xmlparams);
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
			west_players[i]->set_controller(controller_factory->create_controller());
			east_players[i]->set_controller(controller_factory->create_controller());
		} else {
			west_players[i]->set_controller(robot_controller::ptr());
			east_players[i]->set_controller(robot_controller::ptr());
		}
	}
}

void simulator_team_data::add_player(unsigned int id) {
	// Find where to insert the new player.
	unsigned int pos = std::lower_bound(ids.begin(), ids.end(), id) - ids.begin();

	// Create the new objects.
	player_impl::ptr impl(engine ? engine->add_player() : player_impl::trivial());
	player::ptr wplr(new player(id, impl, false));
	player::ptr eplr(new player(id, impl, true));

	// Set the robot controller.
	if (controller_factory) {
		wplr->set_controller(controller_factory->create_controller());
		eplr->set_controller(controller_factory->create_controller());
	}

	// Insert the new data into the arrays.
	ids.insert(ids.begin() + pos, id);
	impls.insert(impls.begin() + pos, impl);
	west_players.insert(west_players.begin() + pos, wplr);
	east_players.insert(east_players.begin() + pos, eplr);

	// Add the player to the XML.
	xmlpp::Element *xmlplayers = xmlutil::get_only_child(xml, "players");
	xmlpp::Element *xmlplayer = xmlplayers->add_child("player");
	xmlplayer->set_attribute("id", Glib::ustring::compose("%1", id));
	config::dirty();
}

void simulator_team_data::remove_player(unsigned int id) {
	// Find the object.
	unsigned int pos = std::lower_bound(ids.begin(), ids.end(), id) - ids.begin();
	if (pos == ids.size() || ids[pos] != id) throw std::domain_error("No such element.");

	// Delete the objects.
	ids.erase(ids.begin() + pos);
	impls.erase(impls.begin() + pos);
	west_players.erase(west_players.begin() + pos);
	east_players.erase(east_players.begin() + pos);

	// Delete the XML.
	xmlpp::Element *xmlplayers = xmlutil::get_only_child(xml, "players");
	xmlpp::Element *xmlplayer = xmlutil::get_only_child_keyed(xmlplayers, "player", "id", Glib::ustring::compose("%1", id));
	xmlplayers->remove_child(xmlplayer);
	config::dirty();
}

