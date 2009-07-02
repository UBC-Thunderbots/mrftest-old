#include "datapool/Config.h"
#include "Log/Log.h"
#include "XBee/XBeeBot.h"
#include "XBee/XBeeModem.h"

#include <string>
#include <sstream>
#include <cstring>
#include <cassert>
#include <stdint.h>

#include <sigc++/sigc++.h>
#include <glibmm.h>
#include <gtkmm.h>

// How long to wait for the data back from the remote node before assuming failure.
#define TIMEOUT 300

// How many errors to ignore before flagging.
#define MAX_ERRORS 5

// How long to send packets with REBOOT flag set before clearing.
#define REBOOT_TIME 500

// How long to send packets with KICK field set before clearing.
#define KICK_TIME 150

// Coefficient to convert green battery ADC readings to volts.
#define GREEN_BATTERY_CONVERSION_FACTOR 0.01

// Coefficient to convert motor battery ADC readings to volts.
#define MOTOR_BATTERY_CONVERSION_FACTOR 0.01

// Flags in the transmitted packet.
#define TXFLAG_RUN    0
#define TXFLAG_REBOOT 1
#define TXFLAG_REPORT 2

// Flags in the received packet.
#define RXFLAG_HAS_GYRO 0

// The PAN ID.
#define PAN_ID 0x7495

namespace {
	//
	// The next frame number to use.
	//
	uint8_t nextFrameNum = 1;

	//
	// Gets a frame number.
	//
	uint8_t generateFrameNumber() {
		uint8_t ans = nextFrameNum++;
		if (!nextFrameNum) nextFrameNum = 1;
		return ans;
	}

	//
	// The format of an outbound packet from host to robot.
	//
	struct TXData {
		int8_t vx;
		int8_t vy;
		int8_t vt;
		uint8_t dribble;
		uint8_t kick;
		uint8_t flags;
	} __attribute__((__packed__));

	//
	// The format of an inbound packet from robot to host.
	//
	struct RXData {
		uint8_t vGreen[2];
		uint8_t vMotor[2];
		uint8_t firmware[2];
		uint8_t flags;
	} __attribute__((__packed__));

	//
	// Clamps a value between a minimum and maximum bound.
	//
	template<typename T>
	T clamp(T v, T lower, T upper) {
		if (v < lower)
			return lower;
		else if (v > upper)
			return upper;
		else
			return v;
	}

	//
	// Encapsulates the job of sending an AT command to the modem and getting a response back.
	//
	class ATCommand : public virtual sigc::trackable {
	public:
		//
		// Constructs the ATCommand object.
		//
		ATCommand(const std::string &command, uint32_t parameter, XBeeModem &modem) : command(command), parameter(parameter), framenum(generateFrameNumber()), done(false), modem(modem) {
			assert(command.size() == 2);
		}

		//
		// Executes the command. Returns once it has successfully completed.
		//
		void execute() {
			modem.connect_packet_received(XBeeModem::PACKET_AT_COMMAND_RESPONSE, sigc::mem_fun(*this, &ATCommand::onPacketReceived));
			timeoutConnection = Glib::signal_timeout().connect(sigc::mem_fun(*this, &ATCommand::onTimeout), TIMEOUT);
			sendPacket();
			while (!done)
				Gtk::Main::run();
		}

	private:
		const std::string command;
		const uint32_t parameter;
		const uint8_t framenum;
		bool done;
		XBeeModem &modem;
		sigc::connection timeoutConnection;

		void sendPacket() {
			struct packet {
				uint8_t framenum;
				uint8_t command[2];
				uint8_t parameter[4];
			} __attribute__((__packed__));
			packet pkt;
			pkt.framenum = framenum;
			pkt.command[0] = command[0];
			pkt.command[1] = command[1];
			for (unsigned int i = 0; i < 4; i++)
				pkt.parameter[3 - i] = parameter >> (i * 8);
			modem.send(XBeeModem::PACKET_AT_COMMAND, &pkt, sizeof(pkt));
		}

		void onPacketReceived(const void *data, std::size_t len) {
			struct packet {
				uint8_t framenum;
				uint8_t command[2];
				uint8_t status;
			} __attribute__((__packed__));
			if (len != sizeof(packet))
				return;
			const packet *p = reinterpret_cast<const packet *>(data);
			if (p->framenum == framenum && p->command[0] == command[0] && p->command[1] == command[1]) {
				if (p->status == 0) {
					done = true;
					timeoutConnection.disconnect();
					Gtk::Main::quit();
				} else {
					Log::log(Log::LEVEL_ERROR, "XBee") << "AT command AT" << command << " with parameter " << parameter << " rejected with status " << static_cast<unsigned int>(p->status) << '\n';
					std::exit(1);
				}
			}
		}

		bool onTimeout() {
			sendPacket();
			return true;
		}
	};
}

Glib::PropertyProxy<XBeeBot::STATUS> XBeeBot::property_commStatus() {
	return prop_commStatus.get_proxy();
}

Glib::PropertyProxy<double> XBeeBot::property_greenVoltage() {
	return prop_greenVoltage.get_proxy();
}

Glib::PropertyProxy<double> XBeeBot::property_motorVoltage() {
	return prop_motorVoltage.get_proxy();
}

Glib::PropertyProxy<unsigned int> XBeeBot::property_firmwareVersion() {
	return prop_firmwareVersion.get_proxy();
}

Glib::PropertyProxy<bool> XBeeBot::property_hasGyro() {
	return prop_hasGyro.get_proxy();
}

void XBeeBot::vx(double vx) {
	vx_ = clamp(vx, -1.0, 1.0) * 127;
}

void XBeeBot::vy(double vy) {
	vy_ = clamp(vy, -1.0, 1.0) * 127;
}

void XBeeBot::vt(double vt) {
	vt_ = clamp(vt, -1.0, 1.0) * 127;
}

void XBeeBot::dribbler(double d) {
	dribbler_ = clamp(d, 0.0, 1.0) * 255;
}

void XBeeBot::kick(double strength) {
	if (!kick_) {
		kick_ = clamp(strength, 0.0, 1.0) * 255;
		Glib::signal_timeout().connect(sigc::mem_fun(*this, &XBeeBot::clearKick), KICK_TIME);
	}
}

void XBeeBot::reboot() {
	if (!reboot_) {
		kick_ = 0;
		reboot_ = true;
		Glib::signal_timeout().connect(sigc::mem_fun(*this, &XBeeBot::clearReboot), REBOOT_TIME);
	}
}

XBeeBot::XBeeBot(uint64_t address, XBeeModem &modem, XBeeBotSet &botSet) : Glib::ObjectBase(typeid(XBeeBot)), address(address), modem(modem), botSet(botSet), prop_commStatus(*this, "commStatus", STATUS_NO_ACK), prop_greenVoltage(*this, "greenVoltage", 0), prop_motorVoltage(*this, "motorVoltage", 0), prop_firmwareVersion(*this, "firmwareVersion", 0), prop_hasGyro(*this, "hasGyro", false), vx_(0), vy_(0), vt_(0), dribbler_(0), kick_(0), reboot_(false), killReasons_(0), lastFrameNum(0), waitingForReport(false), waitingForResponse(false), errorCount(0), noReportCount(0) {
	modem.connect_packet_received(XBeeModem::PACKET_TXSTATUS, sigc::mem_fun(*this, &XBeeBot::transmitResponse));
	modem.connect_packet_received(XBeeModem::PACKET_RX64, sigc::mem_fun(*this, &XBeeBot::receiveData));
}

bool XBeeBot::clearKick() {
	kick_ = 0;
	return false;
}

bool XBeeBot::clearReboot() {
	reboot_ = false;
	return false;
}

void XBeeBot::transmitResponse(const void *data, std::size_t length) {
	// This is what comes from the XBee.
	struct packet {
		uint8_t frameNum;
		uint8_t result;
	} __attribute__((__packed__));

	// Check that the packet is the right length.
	if (length == sizeof(packet)) {
		const packet *pkt = reinterpret_cast<const packet *>(data);

		// Check if the response is for our last frame.
		if (pkt->frameNum == lastFrameNum) {
			// This is a response to our last frame.
			lastFrameNum = 0;
			switch (pkt->result) {
				case 0:
					// Successful transmission.
					errorCount = 0;
					if (prop_commStatus == XBeeBot::STATUS_NO_ACK)
						prop_commStatus = XBeeBot::STATUS_NO_RECV;
					break;

				case 1:
					// No ACK.
					// In this case, the bot probably didn't get the packet.
					// Therefore, it probably won't send back a report.
					// Assume it won't; if it does, no real harm done.
					waitingForReport = false;
					error();
					break;

				case 2:
					// No clear channel.
					// In this case, the bot probably didn't get the packet.
					// Therefore, it probably won't send back a report.
					// Assume it won't; if it does, no real harm done.
					waitingForReport = false;
					error();
					break;

				default:
					// Unknown status.
					Log::log(Log::LEVEL_WARNING, "XBee") << "Transmit status with unknown code " << static_cast<unsigned int>(pkt->result) << '\n';
					// Who knows what's going on with the report? Assume it won't arrive.
					waitingForReport = false;
					break;
			}

			// We're no longer waiting for a response.
			waitingForResponse = false;

			if (!waitingForReport) {
				// If we aren't waiting for a report either, then cancel timeout and release the air.
				timeoutConnection.disconnect();
				botSet.schedule();
			} else {
				// If we're still waiting for a report, just leave well enough alone - the timeout is ticking!
			}
		}
	}
}

void XBeeBot::receiveData(const void *payload, std::size_t length) {
	// This is what comes from the XBee.
	struct packet {
		uint8_t sender[8];
		uint8_t rssi;
		uint8_t options;
		RXData data;
	} __attribute__((__packed__));

	// Check if the packet is the correct length.
	if (length == sizeof(packet)) {
		const packet *pkt = reinterpret_cast<const packet *>(payload);

		// Extract sender address.
		uint64_t senderAddress = 0;
		for (unsigned int i = 0; i < 8; i++)
			senderAddress = senderAddress * 256 + pkt->sender[i];

		// Check if the packet was sent by our bot.
		if (senderAddress == address) {
			// The data is a good packet from our bot. Extract information.
			errorCount = 0;
			noReportCount = 0;
			prop_commStatus = STATUS_OK;
			prop_greenVoltage = (pkt->data.vGreen[0] * 256 + pkt->data.vGreen[1]) * GREEN_BATTERY_CONVERSION_FACTOR;
			prop_motorVoltage = (pkt->data.vMotor[0] * 256 + pkt->data.vMotor[1]) * MOTOR_BATTERY_CONVERSION_FACTOR;
			prop_firmwareVersion = pkt->data.firmware[0] * 256 + pkt->data.firmware[1];
			prop_hasGyro = !!(pkt->data.flags & (1U << RXFLAG_HAS_GYRO));

			if (waitingForReport) {
				// We have now received the report. Mark it as such.
				waitingForReport = false;

				if (!waitingForResponse) {
					// If we aren't waiting for a response either, then cancel timeout and release the air.
					timeoutConnection.disconnect();
					botSet.schedule();
				} else {
					// If we're still waiting for a response, just leave well enough alone - the timeout is ticking!
				}
			} else {
				// No report was pending. Maybe one timed out. Don't break the state machine!
			}
		}
	}
}

bool XBeeBot::timeout() {
	// We were waiting for a report or response but it never arrived.
	// Indicate status.
	if (waitingForResponse)
		error();
	if (waitingForReport)
		noReport();

	waitingForResponse = waitingForReport = false;
	
	// Give the air back to the botset to schedule another bot.
	botSet.schedule();

	// Do not keep this timeout scheduled.
	return false;
}

void XBeeBot::sendPacket(bool requestReport) {
	// Stage the packet.
	struct packet {
		uint8_t frameNum;
		uint8_t address[8];
		uint8_t options;
		TXData data;
	} __attribute__((__packed__));
	packet pkt;
	lastFrameNum = generateFrameNumber();
	pkt.frameNum = lastFrameNum;
	for (unsigned int i = 0; i < 8; i++)
		pkt.address[7 - i] = address >> (8 * i);
	pkt.options = 0;
	pkt.data.vx = vx_;
	pkt.data.vy = vy_;
	pkt.data.vt = vt_;
	pkt.data.dribble = dribbler_;
	pkt.data.kick = kick_;
	pkt.data.flags = 0;
	if (!killReasons_)
		pkt.data.flags |= 1 << TXFLAG_RUN;
	if (requestReport)
		pkt.data.flags |= 1 << TXFLAG_REPORT;
	if (reboot_)
		pkt.data.flags |= 1 << TXFLAG_REBOOT;

	// Send the packet to the modem.
	modem.send(XBeeModem::PACKET_TX64, &pkt, sizeof(pkt));

	// Remember what we're waiting for.
	waitingForResponse = true;
	waitingForReport = requestReport;

	// Schedule a timeout on how long to wait for all the associated inbound data.
	timeoutConnection = Glib::signal_timeout().connect(sigc::mem_fun(*this, &XBeeBot::timeout), TIMEOUT);
}

void XBeeBot::error() {
	if (++errorCount >= 4) {
		errorCount = 4;
		prop_commStatus = STATUS_NO_ACK;
	}
}

void XBeeBot::noReport() {
	if (++noReportCount >= 4) {
		noReportCount = 4;
		if (prop_commStatus == STATUS_OK) {
			prop_commStatus = STATUS_NO_RECV;
		}
	}
}

XBeeBotSet::XBeeBotSet() : nextSender(0), nextReporter(0) {
	// Only one instance can exist at a time.
	assert(!inst);

	// Register for modem status packets.
	modem.connect_packet_received(XBeeModem::PACKET_MODEM_STATUS, sigc::mem_fun(*this, &XBeeBotSet::modemStatus));

	// Initialize the modem more.
	unsigned int channel = Config::instance().getInteger<unsigned int>("XBee", "Channel", 16);
	const struct init_command {
		std::string command;
		uint32_t parameter;
	} init_commands[] = {
		{"PL", Config::instance().getInteger<unsigned int>("XBee", "Power", 10)}, // Power level
		{"CH", channel},                                                          // Channel number
		{"ID", PAN_ID},                                                           // PAN ID
		{"MY", 0xFFFF},                                                           // Local 16-bit address
		{"A2", 4},                                                                // Coordinator parameters
		{"CE", 1},                                                                // Coordinator enable
	};
	for (unsigned int i = 0; i < sizeof(init_commands) / sizeof(*init_commands); i++) {
		ATCommand cmd(init_commands[i].command, init_commands[i].parameter, modem);
		cmd.execute();
	}

	// Initialize the bots.
	for (unsigned int i = 0; ; i++) {
		std::ostringstream oss;
		oss << "Bot" << i;
		const std::string &key = oss.str();
		if (Config::instance().hasKey("XBee", key)) {
			uint64_t address = Config::instance().getInteger<uint64_t>("XBee", key, 16);
			Glib::RefPtr<XBeeBot> bot(new XBeeBot(address, modem, *this));
			bots.push_back(bot);
		} else {
			break;
		}
	}
	if (bots.empty()) {
		Log::log(Log::LEVEL_ERROR, "XBee") << "The [XBee] section does not contain Bot<n> directives starting from <n>=0.\n";
		std::exit(1);
	}
	Log::log(Log::LEVEL_INFO, "XBee") << "Registered " << bots.size() << " robot addresses.\n";

	// Record this instance.
	inst = this;

	// Schedule a packet to be sent.
	schedule();
}

XBeeBotSet::~XBeeBotSet() {
	// Record destruction of the instance.
	inst = 0;
}

XBeeBotSet &XBeeBotSet::instance() {
	assert(inst);
	return *inst;
}

unsigned int XBeeBotSet::size() const {
	return bots.size();
}

Glib::RefPtr<XBeeBot> XBeeBotSet::operator[](unsigned int i) {
	assert(i < bots.size());
	return bots[i];
}

Glib::RefPtr<const XBeeBot> XBeeBotSet::operator[](unsigned int i) const {
	assert(i < bots.size());
	return bots[i];
}

XBeeBotSet *XBeeBotSet::inst;

void XBeeBotSet::modemStatus(const void *data, std::size_t length) {
	if (length != 1)
		return;

	uint8_t status = *reinterpret_cast<const uint8_t *>(data);
	switch (status) {
		case 0:
			Log::log(Log::LEVEL_WARNING, "XBee") << "Modem hardware reset.\n";
			break;

		case 1:
			Log::log(Log::LEVEL_WARNING, "XBee") << "Modem reset due to WDT.\n";
			break;

		case 2:
			Log::log(Log::LEVEL_INFO, "XBee") << "Associated.\n";
			break;

		case 3:
			Log::log(Log::LEVEL_INFO, "XBee") << "Disassociated.\n";
			break;

		case 4:
			Log::log(Log::LEVEL_WARNING, "XBee") << "Modem synchronization lost.\n";
			break;

		case 5:
			Log::log(Log::LEVEL_WARNING, "XBee") << "Modem coordinator realigning.\n";
			break;

		case 6:
			Log::log(Log::LEVEL_INFO, "XBee") << "Coordination started.\n";
			break;

		default:
			Log::log(Log::LEVEL_WARNING, "XBee") << "Unknown modem status " << static_cast<int>(status) << '\n';
			break;
	}
}

void XBeeBotSet::schedule() {
	if (nextSender == nextReporter) {
		// Ask for a report from this bot.
		bots[nextSender]->sendPacket(true);
		
		// Advance the indices. Next reporter advances backwards.
		nextSender = (nextSender + 1) % size();
		nextReporter = (nextReporter + size() - 1) % size();
	} else {
		// Do not ask for a report from this bot.
		bots[nextSender]->sendPacket(false);

		// Advance index.
		nextSender = (nextSender + 1) % size();
	}
}

