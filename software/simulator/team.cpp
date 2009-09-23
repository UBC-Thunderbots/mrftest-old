#include "simulator/team.h"
#include "util/xml.h"
#include "world/config.h"
#include <sstream>

simulator_team_data::simulator_team_data(xmlpp::Element *xml) : score(0), yellow(xml->get_attribute_value("colour") == "yellow"), current_playtype(playtype::halt), west_view(new simulator_team_view(west_players, score, west_other, yellow)), east_view(new simulator_team_view(east_players, score, east_other, yellow)) {
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

				// Create the objects.
				player_impl::ptr impl(player_impl::trivial());
				player::ptr wplr(new player(id, impl, false));
				player::ptr eplr(new player(id, impl, true));

				// Store the objects.
				ids.push_back(id);
				impls.push_back(impl);
				west_players.push_back(wplr);
				east_players.push_back(eplr);
			} else {
				xml->remove_child(node);
				config::dirty();
			}
		} else {
			xml->remove_child(node);
			config::dirty();
		}
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

