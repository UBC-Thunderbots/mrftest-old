#include "xbee/bot.h"
#include "xbee/util.h"



radio_bot::radio_bot(xbee &modem, uint64_t address) : modem(modem), address(address), latency_(0), tx_success_mask(0), feedback_counter(0) {
	fb_packet.flags = 0;
	out_packet_timer.stop();
}



radio_bot::~radio_bot() {
	stop();
}



void radio_bot::start() {
	stop();
	receive_connection = modem.signal_received().connect(sigc::mem_fun(*this, &radio_bot::on_receive));
	out_packet.txhdr.apiid = xbeepacket::TRANSMIT_APIID;
	out_packet.txhdr.frame = 0;
	xbeeutil::address_to_bytes(address, out_packet.txhdr.address);
	out_packet.txhdr.options = 0;
	out_packet.flags = xbeepacket::RUN_FLAG_RUNNING;
	out_packet.drive1_speed = 0;
	out_packet.drive2_speed = 0;
	out_packet.drive3_speed = 0;
	out_packet.drive4_speed = 0;
	out_packet.dribbler_speed = 0;
	send_packet();
}



void radio_bot::stop() {
	receive_connection.disconnect();
	timeout_connection.disconnect();
	out_packet_timer.stop();
}



void radio_bot::drive_scram() {
	drive_impl(0, 0, 0, 0, 0);
}



void radio_bot::drive_direct(int16_t m1, int16_t m2, int16_t m3, int16_t m4) {
	drive_impl(xbeepacket::RUN_FLAG_DIRECT_DRIVE, m1, m2, m3, m4);
}



void radio_bot::drive_controlled(int16_t m1, int16_t m2, int16_t m3, int16_t m4) {
	drive_impl(xbeepacket::RUN_FLAG_CONTROLLED_DRIVE, m1, m2, m3, m4);
}



void radio_bot::drive_impl(uint8_t flag, int16_t m1, int16_t m2, int16_t m3, int16_t m4) {
	out_packet.flags &= ~(xbeepacket::RUN_FLAG_DIRECT_DRIVE | xbeepacket::RUN_FLAG_CONTROLLED_DRIVE);
	out_packet.flags |= flag;
	out_packet.drive1_speed = m1;
	out_packet.drive2_speed = m2;
	out_packet.drive3_speed = m3;
	out_packet.drive4_speed = m4;
}



void radio_bot::dribble_scram() {
	out_packet.flags &= ~xbeepacket::RUN_FLAG_DRIBBLE;
	out_packet.dribbler_speed = 0;
}



void radio_bot::dribble(int16_t power) {
	out_packet.flags |= xbeepacket::RUN_FLAG_DRIBBLE;
	out_packet.dribbler_speed = power;
}



void radio_bot::send_packet() {
	if (++feedback_counter == 30) {
		feedback_counter = 0;
		out_packet.flags |= xbeepacket::RUN_FLAG_FEEDBACK;
	} else {
		out_packet.flags &= ~xbeepacket::RUN_FLAG_FEEDBACK;
	}
	out_packet.txhdr.frame = modem.alloc_frame();
	modem.send(&out_packet, sizeof(out_packet));
	out_packet_timer.reset();
	out_packet_timer.start();
	timeout_connection.disconnect();
	timeout_connection = Glib::signal_timeout().connect(sigc::bind_return(sigc::mem_fun(*this, &radio_bot::on_timeout), false), 100);
}



void radio_bot::on_receive(const void *buffer, std::size_t length) {
	// Must be long enough to contain API ID.
	if (length < 1) {
		return;
	}

	// Dispatch by API ID.
	uint8_t apiid = *static_cast<const uint8_t *>(buffer);
	if (apiid == xbeepacket::TRANSMIT_STATUS_APIID) {
		// Check for correct length.
		if (length != sizeof(xbeepacket::TRANSMIT_STATUS)) {
			return;
		}
		const xbeepacket::TRANSMIT_STATUS &in_packet = *static_cast<const xbeepacket::TRANSMIT_STATUS *>(buffer);

		// Check that it's in response to our own packet and not somebody else's.
		if (in_packet.frame != out_packet.txhdr.frame) {
			return;
		}

		// Stop the latency estimation timer.
		out_packet_timer.stop();

		// Check status.
		if (in_packet.status == xbeepacket::TRANSMIT_STATUS_SUCCESS) {
			// Accumulate a latency estimate.
			latency_ = latency_ * 0.9 + out_packet_timer.elapsed() * 0.1;

			// Accumulate a successful transmission.
			tx_success_mask = (tx_success_mask << 1) | 1;
		} else if (in_packet.status == xbeepacket::TRANSMIT_STATUS_NO_ACK) {
			// Accumulate an unsuccessful transmission.
			tx_success_mask <<= 1;
		} else if (in_packet.status == xbeepacket::TRANSMIT_STATUS_NO_CCA) {
			// Accumulate an unsuccessful transmission.
			tx_success_mask <<= 1;
		}

		// Fire the update notification signal.
		signal_updated().emit();

		// Send the next packet.
		send_packet();
	} else if (apiid == xbeepacket::RECEIVE_APIID) {
		// Check for correct length.
		if (length != sizeof(xbeepacket::FEEDBACK_DATA)) {
			return;
		}
		const xbeepacket::FEEDBACK_DATA &in_packet = *static_cast<const xbeepacket::FEEDBACK_DATA *>(buffer);

		// Check that it's from our robot and not another one.
		if (xbeeutil::address_from_bytes(in_packet.rxhdr.address) != address) {
			return;
		}

		// Check that it's indicating normal run mode.
		if (!(in_packet.flags & xbeepacket::FEEDBACK_FLAG_RUNNING)) {
			return;
		}

		// Stash it.
		fb_packet = in_packet;

		// Fire the update notification signal.
		signal_updated().emit();
	}
}



void radio_bot::on_timeout() {
	tx_success_mask <<= 1;
	send_packet();
}

