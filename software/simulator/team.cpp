#include "simulator/team.h"
#include "simulator/simulator.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <limits>

Simulator::Team::Team(Simulator &sim, const Team *other, bool invert) : sim(sim), other(other), invert(invert), ready_(true) {
}

Simulator::Team::~Team() {
}

bool Simulator::Team::has_connection() const {
	return connection.is();
}

void Simulator::Team::add_connection(Connection::Ptr conn) {
	assert(!connection.is());
	assert(conn.is());
	conn->signal_closed().connect(sigc::mem_fun(this, &Team::close_connection));
	conn->signal_packet().connect(sigc::mem_fun(this, &Team::on_packet));
	connection = conn;
	ready_ = true;
	Glib::signal_idle().connect_once(sigc::mem_fun(this, &Team::send_speed_mode));
	Glib::signal_idle().connect_once(sigc::mem_fun(this, &Team::send_play_type));
	Glib::signal_idle().connect_once(signal_ready().make_slot());
}

bool Simulator::Team::ready() const {
	return ready_;
}

sigc::signal<void> &Simulator::Team::signal_ready() const {
	return signal_ready_;
}

void Simulator::Team::send_tick(const timespec &ts) {
	// Actually remove any robots we were asked to remove in the past time period.
	for (std::size_t i = 0; i < to_remove.size(); ++i) {
		bool found = false;
		for (std::size_t j = 0; j < players.size() && !found; ++j) {
			if (to_remove[i] == players[j].pattern) {
				Player::Ptr plr = players[j].player;
				players.erase(players.begin() + j);
				sim.engine->remove_player(plr);
				found = true;
			}
		}
		if (!found) {
			std::cout << "AI asked to remove nonexistent robot pattern " << to_remove[i] << '\n';
			close_connection();
			return;
		}
	}
	to_remove.clear();

	// Actually add any robots we were asked to add in the past time period.
	for (std::size_t i = 0; i < to_add.size(); ++i) {
		for (std::size_t j = 0; j < players.size(); ++j) {
			if (to_add[i] == players[j].pattern) {
				std::cout << "AI asked to add robot pattern " << to_add[i] << " twice\n";
				close_connection();
				return;
			}
		}
		PlayerInfo pi = { player: sim.engine->add_player(), pattern: to_add[i], };
		players.push_back(pi);
	}
	to_add.clear();

	if (connection.is()) {
		// Send the state of the world over the socket.
		Proto::S2APacket packet;
		std::memset(&packet, 0, sizeof(packet));
		packet.type = Proto::S2A_PACKET_TICK;
		sim.encode_ball_state(packet.world_state.ball, invert);
		for (std::size_t i = 0; i < G_N_ELEMENTS(packet.world_state.friendly); ++i) {
			if (i < players.size()) {
				packet.world_state.friendly[i].robot_info.pattern = players[i].pattern;
				packet.world_state.friendly[i].robot_info.x = players[i].player->position().x * (invert ? -1.0 : 1.0);
				packet.world_state.friendly[i].robot_info.y = players[i].player->position().y * (invert ? -1.0 : 1.0);
				packet.world_state.friendly[i].robot_info.orientation = players[i].player->orientation() + (invert ? M_PI : 0.0);
				packet.world_state.friendly[i].has_ball = players[i].player->has_ball();
			} else {
				packet.world_state.friendly[i].robot_info.pattern = std::numeric_limits<unsigned int>::max();
			}
		}
		for (std::size_t i = 0; i < G_N_ELEMENTS(packet.world_state.enemy); ++i) {
			if (i < other->players.size()) {
				packet.world_state.enemy[i].pattern = other->players[i].pattern;
				packet.world_state.enemy[i].x = other->players[i].player->position().x * (invert ? -1.0 : 1.0);
				packet.world_state.enemy[i].y = other->players[i].player->position().y * (invert ? -1.0 : 1.0);
				packet.world_state.enemy[i].orientation = other->players[i].player->orientation() + (invert ? M_PI : 0.0);
			} else {
				packet.world_state.enemy[i].pattern = std::numeric_limits<unsigned int>::max();
			}
		}
		packet.world_state.stamp = ts;
#warning implement scores
		packet.world_state.friendly_score = 0;
		packet.world_state.enemy_score = 0;
		connection->send(packet);

		// Having sent the tick packet, we are no longer ready and must wait for an A2S_PACKET_PLAYERS.
		ready_ = false;
	}
}

void Simulator::Team::send_speed_mode() {
	if (connection.is()) {
		Proto::S2APacket packet;
		std::memset(&packet, 0, sizeof(packet));
		packet.type = Proto::S2A_PACKET_SPEED_MODE;
		packet.speed_mode = sim.speed_mode();
		connection->send(packet);
	}
}

void Simulator::Team::send_play_type() {
	if (connection.is()) {
		Proto::S2APacket packet;
		std::memset(&packet, 0, sizeof(packet));
		packet.type = Proto::S2A_PACKET_PLAY_TYPE;
		packet.playtype = invert ? AI::Common::PlayType::INVERT[sim.play_type()] : sim.play_type();
		connection->send(packet);
	}
}

void Simulator::Team::close_connection() {
	// Drop the socket.
	connection.reset();

	// If no AI is connected, we should not prevent the other team from making forward progress with simulation.
	ready_ = true;
	Glib::signal_idle().connect_once(signal_ready().make_slot());

	// Stop all our robots from kicking, chipping, or moving.
	for (std::size_t i = 0; i < players.size(); ++i) {
		players[i].player->orders.kick = false;
		players[i].player->orders.chip = false;
		players[i].player->orders.chick_power = 0.0;
		std::fill(&players[i].player->orders.wheel_speeds[0], &players[i].player->orders.wheel_speeds[4], 0);
	}
}

void Simulator::Team::on_packet(const Proto::A2SPacket &packet) {
	switch (packet.type) {
		case Proto::A2S_PACKET_PLAYERS:
			// This should only be received if we're waiting for it after an S2A_PACKET_TICK.
			if (ready_) {
				std::cout << "AI out of phase, sent orders twice\n";
				close_connection();
				return;
			}

			// Stash the robots' orders into their Player objects for the engine to examine.
			for (std::size_t i = 0; i < G_N_ELEMENTS(packet.players); ++i) {
				if (packet.players[i].pattern != std::numeric_limits<unsigned int>::max()) {
					bool found = false;
					for (std::size_t j = 0; j < players.size() && !found; ++j) {
						if (players[j].pattern == packet.players[i].pattern) {
							found = true;
							players[j].player->orders = packet.players[i];
						}
					}
					if (!found) {
						std::cout << "AI sent orders to nonexistent robot pattern " << packet.players[i].pattern << '\n';
						close_connection();
						return;
					}
				}
			}

			// Mark this team as ready for another tick.
			ready_ = true;
			signal_ready().emit();
			return;

		case Proto::A2S_PACKET_ADD_PLAYER:
			if (packet.pattern >= Proto::MAX_PLAYERS_PER_TEAM) {
				std::cout << "AI asked to add invalid robot pattern " << packet.pattern << '\n';
				close_connection();
				return;
			}
			to_add.push_back(packet.pattern);
			return;

		case Proto::A2S_PACKET_REMOVE_PLAYER:
			if (packet.pattern >= Proto::MAX_PLAYERS_PER_TEAM) {
				std::cout << "AI asked to remove invalid robot pattern " << packet.pattern << '\n';
				close_connection();
				return;
			}
			to_remove.push_back(packet.pattern);
			return;

		case Proto::A2S_PACKET_SET_SPEED:
			sim.speed_mode(packet.speed_mode);
			return;

		case Proto::A2S_PACKET_PLAY_TYPE:
			if (packet.playtype < AI::Common::PlayType::COUNT) {
				sim.set_play_type(invert ? AI::Common::PlayType::INVERT[packet.playtype] : packet.playtype);
			} else {
				std::cout << "AI asked to set invalid play type\n";
				close_connection();
			}
			return;

		case Proto::A2S_PACKET_DRAG_BALL:
		{
			Point p(packet.drag.x, packet.drag.y);
			sim.engine->get_ball()->position(invert ? -p : p);
			return;
		}

		case Proto::A2S_PACKET_DRAG_PLAYER:
		{
			bool found = false;
			for (std::size_t i = 0; i < players.size(); ++i) {
				if (players[i].pattern == packet.drag.pattern) {
					Point p(packet.drag.x, packet.drag.y);
					players[i].player->position(invert ? -p : p);
					found = true;
				}
			}
			if (!found) {
				std::cout << "AI dragged nonexistent robot pattern " << packet.drag.pattern << '\n';
				close_connection();
				return;
			}
			return;
		}
	}

	std::cout << "AI sent bad packet type\n";
	close_connection();
}

