#include "sim/simulator.h"
#include "sim/team.h"
#include "util/config.h"
#include "util/xml.h"
#include <iostream>

simulator_team_data::simulator_team_data(simulator &sim, bool invert_playtype, xmlpp::Element *xml, bool yellow, ball::ptr ball, field::ptr field) : the_simulator(sim), invert_playtype(invert_playtype), score(0), yellow(yellow), controller_factory(0), west_view(new simulator_team_view(*this, west_players, west_other)), east_view(new simulator_team_view(*this, east_players, east_other)), xml(xml), the_ball(ball), the_field(field) {
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

	// Connect to the playtype change signal.
	sim.signal_playtype_changed().connect(sigc::mem_fun(this, &simulator_team_data::playtype_changed));
}

simulator_team_data::~simulator_team_data() {
	// Remove the robot_controllers from all the player_impls.
	// This breaks circular references and eliminates a potential memory leak.
	for (std::vector<player_impl::ptr>::iterator i = impls.begin(), iend = impls.end(); i != iend; ++i)
		(*i)->set_controller(robot_controller::ptr());
}

void simulator_team_data::set_engine(simulator_engine::ptr e) {
	// Remember how many objects have been created.
	unsigned int num_players = impls.size();

	// Delete old objects.
	while (impls.size())
		remove_player(impls.size() - 1);

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
	strategy_factory::map_type::const_iterator factoryiter = factories.find(name.collate_key());
	strategy::ptr strat;
	if (factoryiter != factories.end()) {
		strategy_factory *factory = factoryiter->second;
		xmlpp::Element *xmlparams = xmlutil::strip(xmlutil::get_only_child_keyed(xmlstrategies, "params", "strategy", name));
		strat = factory->create_strategy(xmlparams, the_ball, the_field, yellow ? west_view : east_view);
	}

	// Lock in the strategy.
	team_strategy = strat;

	// Save the choice of engine in the configuration.
	if (xmlstrategies->get_attribute_value("active") != name) {
		xmlstrategies->set_attribute("active", name);
		config::dirty();
	}
}

void simulator_team_data::set_controller_type(const Glib::ustring &name) {
	// Find the factory with this name.
	const robot_controller_factory::map_type &m = robot_controller_factory::all();
	robot_controller_factory::map_type::const_iterator i = m.find(name.collate_key());
	if (i != m.end()) {
		controller_factory = i->second;
	} else {
		controller_factory = 0;
	}

	// Assign new controllers to all the bots.
	for (unsigned int i = 0; i < impls.size(); i++) {
		if (controller_factory && engine) {
			impls[i]->set_controller(controller_factory->create_controller(impls[i], yellow, i));
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
	if (controller_factory && engine)
		impl->set_controller(controller_factory->create_controller(impl, yellow, impls.size()));

	// Insert the new data into the arrays.
	impls.push_back(impl);
	west_players.push_back(wplr);
	east_players.push_back(eplr);

	// Update the XML.
	xml->set_attribute("players", Glib::ustring::compose("%1", impls.size()));
	config::dirty();

	// Send notifications.
	west_view->signal_robot_added().emit();
	west_view->signal_player_added().emit();
	east_view->signal_robot_added().emit();
	east_view->signal_player_added().emit();
}

void simulator_team_data::remove_player(unsigned int index) {
	// Grab a reference to the players before clearing the arrays.
	player_impl::ptr impl = impls[index];
	player::ptr west_bot = west_players[index];
	player::ptr east_bot = east_players[index];

	// Detach the player's robot_controller (breaks circular references).
	impl->set_controller(robot_controller::ptr());

	// Delete the objects.
	impls.erase(impls.begin() + index);
	west_players.erase(west_players.begin() + index);
	east_players.erase(east_players.begin() + index);

	// Update the XML.
	xml->set_attribute("players", Glib::ustring::compose("%1", impls.size()));
	config::dirty();

	// Send notifications.
	west_view->signal_robot_removed().emit(index, west_bot);
	west_view->signal_player_removed().emit(index, west_bot);
	east_view->signal_robot_removed().emit(index, east_bot);
	east_view->signal_player_removed().emit(index, east_bot);

	// Remove the player from the engine.
	if (engine)
		engine->remove_player(impl);

	// Check for leaky references to the player objects.
	if (west_bot->refs() != 1 || east_bot != 1)
		std::cerr << "Leaky reference detected to player object.\n";
	west_bot.reset();
	east_bot.reset();

	// Check for a leaky reference to the player_impl.
	if (impl->refs() != 1)
		std::cerr << "Leaky reference detected to player_impl object.\n";
}

playtype::playtype simulator_team_data::current_playtype() const {
	if (invert_playtype)
		return playtype::invert[the_simulator.current_playtype()];
	else
		return the_simulator.current_playtype();
}

void simulator_team_data::playtype_changed(playtype::playtype) {
	west_view->signal_playtype_changed().emit(current_playtype());
	east_view->signal_playtype_changed().emit(current_playtype());
}

