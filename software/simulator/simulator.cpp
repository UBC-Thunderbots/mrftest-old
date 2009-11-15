#include "simulator/field.h"
#include "simulator/simulator.h"
#include "util/xml.h"
#include "world/config.h"
#include "world/timestep.h"

simulator::simulator(xmlpp::Element *xml) : cur_playtype(playtype::halt), fld(new simulator_field), west_ball(new ball(ball_impl::trivial(), false)), east_ball(new ball(ball_impl::trivial(), true)), west_team(*this, false, xmlutil::strip(xmlutil::get_only_child(xml, "westteam")), true, west_ball, fld), east_team(*this, true, xmlutil::strip(xmlutil::get_only_child(xml, "eastteam")), false, east_ball, fld), xml(xml), ticker(1.0 / TIMESTEPS_PER_SECOND) {
	// Configure objects with each other as the opponents.
	west_team.set_other(east_team.west_view, east_team.east_view);
	east_team.set_other(west_team.west_view, west_team.east_view);

	// Set the appropriate engine.
	xmlpp::Element *xmlengines = xmlutil::strip(xmlutil::get_only_child(xml, "engines"));
	set_engine(xmlengines->get_attribute_value("active"));

	// Connect to the update tick.
	ticker.signal_expired().connect(sigc::mem_fun(*this, &simulator::tick));
}

void simulator::set_engine(const Glib::ustring &engine_name) {
	// Get the "engines" XML element.
	xmlpp::Element *xmlengines = xmlutil::get_only_child(xml, "engines");

	// Find the engine factory for this engine type.
	const simulator_engine_factory::map_type &factories = simulator_engine_factory::all();
	simulator_engine_factory::map_type::const_iterator factoryiter = factories.find(engine_name);
	if (factoryiter != factories.end()) {
		simulator_engine_factory *factory = factoryiter->second;
		xmlpp::Element *xmlparams = xmlutil::strip(xmlutil::get_only_child_keyed(xmlengines, "params", "engine", engine_name));
		engine = factory->create_engine(xmlparams);
	} else {
		engine.reset();
	}

	// Select the proper implementation of the ball.
	if (engine) {
		const ball_impl::ptr &impl = engine->get_ball();
		west_ball->set_impl(impl);
		east_ball->set_impl(impl);
	} else {
		west_ball->set_impl(ball_impl::trivial());
		east_ball->set_impl(ball_impl::trivial());
	}

	// Make the teams use the proper engine.
	west_team.set_engine(engine);
	east_team.set_engine(engine);

	// Save the choice of engine in the configuration.
	if (xmlengines->get_attribute_value("active") != engine_name) {
		xmlengines->set_attribute("active", engine_name);
		config::dirty();
	}
}

void simulator::tick() {
	west_team.tick_preengine();
	east_team.tick_preengine();
	if (engine)
		engine->tick();
	west_team.tick_postengine();
	west_team.tick_postengine();
	sig_updated.emit();
}

