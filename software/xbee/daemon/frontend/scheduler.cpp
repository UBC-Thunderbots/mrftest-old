#include "util/algorithm.h"
#include "util/rwlock.h"
#include "util/time.h"
#include "xbee/daemon/frontend/backend.h"
#include "xbee/daemon/frontend/daemon.h"
#include "xbee/daemon/frontend/scheduler.h"
#include "xbee/shared/packettypes.h"
#include <algorithm>
#include <functional>
#include <cassert>
#include <ctime>

namespace {
	/**
	 * How many milliseconds to wait for a feedback packet or response packet
	 * before assuming it has been lost.
	 */
	const unsigned int TIMEOUT = 75;
}

XBeeScheduler::XBeeScheduler(XBeeDaemon &daemon) : daemon(daemon), sent_count(0), next_type(NEXT_QUEUED), last_feedback_index(0) {
	daemon.backend.signal_received.connect(sigc::mem_fun(this, &XBeeScheduler::on_receive));
	daemon.shm->run_data_interval.tv_sec = 1;
	daemon.shm->run_data_interval.tv_nsec = 0;
	timespec_now(last_rundata_timestamp);
}

void XBeeScheduler::queue(XBeeRequest::Ptr req) {
	pending.push(req);
	push();
}

void XBeeScheduler::push() {
	// If packets are outstanding, do nothing. We will be signalled when a
	// response arrives or timeout expires, at which point we can repush.
	if (sent_count > 0) {
		return;
	}

	// Compute whether there is any data available to send of each type.
	bool has_queued = !pending.empty();
	bool has_bulk = exists_if(daemon.run_data_index_reverse, daemon.run_data_index_reverse + XBeePacketTypes::MAX_DRIVE_ROBOTS, std::bind2nd(std::not_equal_to<uint64_t>(), UINT64_C(0)));

	// If there is no data at all to send, do nothing.
	if (!has_queued && !has_bulk) {
		return;
	}

	// If there's only one type of data available, we must send that.
	if (!has_queued) {
		next_type = NEXT_BULK;
	} else if (!has_bulk) {
		next_type = NEXT_QUEUED;
	}

	// Determine what to do next.
	if (next_type == NEXT_QUEUED) {
		// We should send some queued data. Dequeue a XBeeRequest.
		XBeeRequest::Ptr req = pending.front();
		pending.pop();

		// If the XBeeRequest will expect a response, then record it as outstanding
		// and register a timeout so we're sure delivery is successful at least
		// over the local USB, to the local radio. We will then do nothing
		// further.
		if (req->has_response()) {
			uint8_t frame = req->data()[1];
			assert(frame);
			assert(!sent[frame].data.is());
			sent[frame].data = req;
			sent[frame].timeout_connection = Glib::signal_timeout().connect(sigc::bind(sigc::mem_fun(this, &XBeeScheduler::on_request_timeout), frame), TIMEOUT);
			++sent_count;
		}

		// Actually send the data.
		iovec iov;
		iov.iov_base = const_cast<uint8_t *>(&req->data()[0]);
		iov.iov_len = req->data().size();
		daemon.backend.send(&iov, 1);

		// Next, it's a bulk packet's turn if there is one.
		next_type = NEXT_BULK;

		// If we will be getting a response, do nothing yet; we should wait for
		// this XBeeRequest to be complete before we move on and push more data. On
		// the other hand, if we will not be getting any response anyway, we
		// can't possibly know when the packet is "finished" and thus when we
		// can move on to the next packet... so we might as well do it right
		// now!
		if (!req->has_response()) {
			push();
		}
	} else {
		// We should send a bulk packet. Assemble the packet.
		struct __attribute__((packed)) BULK_PACKET {
			XBeePacketTypes::TRANSMIT16_HDR hdr;
			uint8_t pad;
			XBeePacketTypes::RUN_DATA data[XBeePacketTypes::MAX_DRIVE_ROBOTS];
			uint8_t pad2;
		} packet;
		unsigned int max_index = 0;
		for (unsigned int i = 0; i < XBeePacketTypes::MAX_DRIVE_ROBOTS; ++i) {
			if (daemon.run_data_index_reverse[i]) {
				max_index = i;
			}
		}
		bool eligible_for_feedback[XBeePacketTypes::MAX_DRIVE_ROBOTS];
		{
			RWLockScopedAcquire acq(&daemon.shm->lock, &pthread_rwlock_wrlock);
			timespec now;
			timespec_now(now);
			timespec run_data_interval;
			timespec_sub(now, last_rundata_timestamp, run_data_interval);
			daemon.shm->run_data_interval = run_data_interval;
			last_rundata_timestamp = now;
			timespec threshold;
			threshold.tv_sec = 0;
			threshold.tv_nsec = 500000000L;
			for (unsigned int i = 0; i <= max_index; ++i) {
				timespec diff;
				timespec_sub(now, daemon.shm->frames[i].timestamp, diff);
				bool timeout = timespec_cmp(diff, threshold) > 0;
				if (daemon.run_data_index_reverse[i] && (daemon.shm->frames[i].run_data.flags & XBeePacketTypes::RUN_FLAG_RUNNING)) {
					if (timeout) {
						packet.data[i].flags = XBeePacketTypes::RUN_FLAG_RUNNING;
						packet.data[i].dribbler_speed = 0;
						packet.data[i].chick_power = 0;
					} else {
						packet.data[i] = daemon.shm->frames[i].run_data;
					}
					eligible_for_feedback[i] = true;
				} else {
					packet.data[i].flags = XBeePacketTypes::RUN_FLAG_RUNNING;
					packet.data[i].dribbler_speed = 0;
					packet.data[i].chick_power = 0;
					eligible_for_feedback[i] = false;
				}
				if (!(packet.data[i].flags & (XBeePacketTypes::RUN_FLAG_DIRECT_DRIVE | XBeePacketTypes::RUN_FLAG_CONTROLLED_DRIVE))) {
					packet.data[i].drive1_speed = 0;
					packet.data[i].drive2_speed = 0;
					packet.data[i].drive3_speed = 0;
					packet.data[i].drive4_speed = 0;
				}
			}
		}
		packet.hdr.apiid = XBeePacketTypes::TRANSMIT16_APIID;
		packet.hdr.frame = 0;
		packet.hdr.address[0] = 0xFF;
		packet.hdr.address[1] = 0xFF;
		packet.hdr.options = XBeePacketTypes::TRANSMIT_OPTION_DISABLE_ACK;
		packet.pad = 0;
		packet.pad2 = 0;
		std::size_t length = sizeof(XBeePacketTypes::TRANSMIT16_HDR) + (max_index + 1) * sizeof(XBeePacketTypes::RUN_DATA) + 2;

		// Check if any robots are eligible to send feedback.
		bool any_eligible_for_feedback = exists(eligible_for_feedback, eligible_for_feedback + max_index + 1, true);
		if (any_eligible_for_feedback) {
			// Advance the feedback index to the next eligible robot.
			do {
				last_feedback_index = (last_feedback_index + 1) % (max_index + 1);
			} while (!eligible_for_feedback[last_feedback_index]);
			last_feedback_address = daemon.robots[daemon.run_data_index_reverse[last_feedback_index]]->address16();

			// Set the feedback flag in this robot's outbound data block.
			packet.data[last_feedback_index].flags |= XBeePacketTypes::RUN_FLAG_FEEDBACK;

			// Record the timestamp.
			timespec_now(last_feedback_timestamp);
		}

		// Actually send the data.
		iovec iov;
		iov.iov_base = &packet;
		iov.iov_len = length;
		daemon.backend.send(&iov, 1);

		// Record that there is a packet outstanding.
		++sent_count;

		// Start a timeout for receiving the feedback packet. If no robots were
		// asked to send feedback because none were eligible, we're in a
		// slightly strange situation where some robots are in the process of
		// being configured (else they wouldn't have run data index reverse
		// mappings at all), but have not yet been fully configured (else they
		// would have the run flag turned on and be eligible for feedback).
		// Because this will probably happen only occasionally and not for very
		// long (only until a robot is fully configured), and because this only
		// happens when NO robots are actually driving (else they would be
		// eligible for feedback), let's just handle this by the slightly hacky
		// solution of letting the feedback timeout expire and then pushing more
		// packets.
		feedback_timeout_connection.disconnect();
		feedback_timeout_connection = Glib::signal_timeout().connect(sigc::bind_return(sigc::mem_fun(this, &XBeeScheduler::on_feedback_timeout), false), TIMEOUT);

		// Next, it's a queued packet's turn if there is one.
		next_type = NEXT_QUEUED;
	}

	push();
}

bool XBeeScheduler::on_request_timeout(uint8_t frame) {
	// Resend the XBeeRequest.
	XBeeRequest::Ptr req = sent[frame].data;
	iovec iov;
	iov.iov_base = const_cast<uint8_t *>(&req->data()[0]);
	iov.iov_len = req->data().size();
	daemon.backend.send(&iov, 1);

	// Keep the timeout signal connected.
	return true;
}

void XBeeScheduler::on_feedback_timeout() {
	// Whatever happens, give up on waiting for this XBeeRequest.
	--sent_count;

	// See if we can report the timeout to someone who cares.
	uint64_t address64 = daemon.run_data_index_reverse[last_feedback_index];
	if (address64) {
		// OK, there's still a robot at this run data index.
		XBeeRobot::Ptr bot(daemon.robots[address64]);
		if (bot->address16() == last_feedback_address) {
			// OK, the bot at this run data index has the same 16-bit address as
			// the one that was there when we sent the original bulk packet. We
			// can be reasonably certain it's the same robot.
			bot->on_feedback_timeout();
		}
	}

	// Send some more packets, in any case.
	push();
}

void XBeeScheduler::on_receive(const std::vector<uint8_t> &data) {
	if (data[0] == XBeePacketTypes::AT_RESPONSE_APIID || data[0] == XBeePacketTypes::REMOTE_AT_RESPONSE_APIID || data[0] == XBeePacketTypes::TRANSMIT_STATUS_APIID) {
		// We're receiving an XBee-layer response to a queued XBeeRequest. Use the
		// frame number to dispatch the XBeeRequest to the proper handlers.
		uint8_t frame = data[1];
		if (sent[frame].data.is()) {
			// This matches a sent frame.
			assert(sent[frame].data->has_response());
			sent[frame].data->signal_complete().emit(&data[0], data.size());
			sent[frame].data.reset();
			sent[frame].timeout_connection.disconnect();
			--sent_count;

			// Send some more packets.
			push();
		} else {
			// This does not match a sent frame. It could be some old crud from
			// a previous run accumulating in the serial buffer. Just ignore it.
		}
	} else if (data[0] == XBeePacketTypes::RECEIVE16_APIID) {
		// We're receiving a feedback packet from a robot.
		const struct __attribute__((packed)) FEEDBACK_PACKET {
			XBeePacketTypes::RECEIVE16_HDR hdr;
			XBeePacketTypes::FEEDBACK_DATA data;
		} &packet = *reinterpret_cast<const FEEDBACK_PACKET *>(&data[0]);

		// Check for proper length.
		if (data.size() != sizeof(packet)) {
			return;
		}

		// Check for the source address being the address of the robot most
		// recently asked to provide feedback.
		uint16_t address16 = (packet.hdr.address[0] << 8) | packet.hdr.address[1];
		if (address16 != last_feedback_address) {
			return;
		}

		// Check flags.
		if (!(packet.data.flags & XBeePacketTypes::FEEDBACK_FLAG_RUNNING)) {
			return;
		}

		// Looks like it's a genuine feedback packet. Assuming there's still a
		// robot hanging off this run data index, pass the feedback to it.
		uint64_t address64 = daemon.run_data_index_reverse[last_feedback_index];
		if (address64) {
			XBeeRobot::Ptr bot(daemon.robots[address64]);
			if (bot->address16() == last_feedback_address) {
				timespec now;
				timespec_now(now);
				timespec diff;
				timespec_sub(now, last_feedback_timestamp, diff);
				bot->on_feedback(packet.hdr.rssi, packet.data, diff);
			}
		}

		// We should now disconnect the timeout and send more packets.
		--sent_count;
		feedback_timeout_connection.disconnect();
		push();
	}
}

