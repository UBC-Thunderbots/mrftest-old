#include "simulator/team.h"
#include "util/xml.h"
#include "world/config.h"
#include <sstream>
#include <algorithm>
#include <stdexcept>

simulator_team_data::simulator_team_data(xmlpp::Element *xml, bool yellow) : score(0), yellow(yellow), current_playtype(playtype::halt), west_view(new simulator_team_view(west_players, score, west_other, yellow)), east_view(new simulator_team_view(east_players, score, east_other, yellow)) {
	// Iterate the child nodes.
	const xmlpp::Node::NodeList &players = xml->get_children();
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
				xml->remove_child(node);
				config::dirty();
			}
		} else {
			xml->remove_child(node);
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

void simulator_team_data::set_engine(const simulator_engine::ptr &e) {
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

void simulator_team_data::add_player(unsigned int id) {
	// Find where to insert the new player.
	unsigned int pos = std::lower_bound(ids.begin(), ids.end(), id) - ids.begin();

	// Create the new objects.
	player_impl::ptr impl(engine ? engine->add_player() : player_impl::trivial());
	player::ptr wplr(new player(id, impl, false));
	player::ptr eplr(new player(id, impl, true));

	// Insert the new data into the arrays.
	ids.insert(ids.begin() + pos, id);
	impls.insert(impls.begin() + pos, impl);
	west_players.insert(west_players.begin() + pos, wplr);
	east_players.insert(east_players.begin() + pos, eplr);
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
}

