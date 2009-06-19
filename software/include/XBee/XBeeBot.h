#ifndef XBEE_XBEEBOT_H
#define XBEE_XBEEBOT_H

#include "XBee/XBeeModem.h"

#include <vector>

#include <sigc++/sigc++.h>
#include <gtkmm.h>

class XBeeBotSet;

//
// An individual bot addressable by XBee.
//
class XBeeBot : public Glib::Object, public virtual sigc::trackable {
public:
	//
	// The status of communication with a bot.
	//
	enum STATUS {
		// Communication is OK.
		STATUS_OK,
		// Data is received and ACKed by XBee, but no data is coming back.
		STATUS_NO_RECV,
		// The XBee is not ACKing messages.
		STATUS_NO_ACK,
	};

	//
	// The communication status.
	//
	Glib::PropertyProxy<STATUS> property_commStatus();

	//
	// The voltage of the green battery.
	//
	Glib::PropertyProxy<double> property_greenVoltage();

	//
	// The voltage of the motor battery.
	//
	Glib::PropertyProxy<double> property_motorVoltage();

	//
	// The version of the firmware running on the bot.
	//
	Glib::PropertyProxy<unsigned int> property_firmwareVersion();

	//
	// Sets the VX setpoint for this bot.
	//
	void vx(double vx);

	//
	// Sets the VY setpoint for this bot.
	//
	void vy(double vy);

	//
	// Sets the VT setpoint for this bot.
	//
	void vt(double vt);

	//
	// Sets the speed at which the dribbler should run.
	//
	void dribbler(double dribbler);

	//
	// Starts a kick of the specified strength.
	//
	void kick(double strength);

	//
	// Gets the run/stop control for this bot.
	//
	bool run() const;

	//
	// Sets the run/stop control for this bot.
	//
	void run(bool run);

	//
	// Reboots the bot.
	//
	void reboot();

private:
	uint64_t address;
	XBeeModem &modem;
	XBeeBotSet &botSet;
	Glib::Property<STATUS> prop_commStatus;
	Glib::Property<double> prop_greenVoltage;
	Glib::Property<double> prop_motorVoltage;
	Glib::Property<unsigned int> prop_firmwareVersion;
	int8_t vx_, vy_, vt_;
	uint8_t dribbler_, kick_, flags;
	uint8_t lastFrameNum;
	sigc::connection timeoutConnection;
	bool waitingForReport, waitingForResponse;
	unsigned int errorCount, noReportCount;

	XBeeBot(uint64_t address, XBeeModem &modem, XBeeBotSet &botSet);
	bool clearKick();
	bool clearReboot();
	void transmitResponse(const void *data, std::size_t length);
	void receiveData(const void *data, std::size_t length);
	bool timeout();
	void sendPacket(bool requestReport);
	void error();
	void noReport();

	friend class Glib::RefPtr<XBeeBot>;
	friend class XBeeBotSet;
};

//
// A collection of all the bots the XBee can address.
//
class XBeeBotSet : public virtual sigc::trackable {
public:
	//
	// Creates a bot set. Only one can exist at a time.
	//
	XBeeBotSet();

	//
	// Destroys a bot set.
	//
	~XBeeBotSet();

	//
	// Gets the current instance of the bot set.
	//
	static XBeeBotSet &instance();

	//
	// Returns the number of bots in the set.
	//
	unsigned int size() const;

	//
	// Returns an individual bot.
	//
	Glib::RefPtr<XBeeBot> operator[](unsigned int i);
	Glib::RefPtr<const XBeeBot> operator[](unsigned int i) const;

private:
	XBeeModem modem;
	std::vector<Glib::RefPtr<XBeeBot> > bots;
	unsigned int nextSender, nextReporter;
	static XBeeBotSet *inst;

	void modemStatus(const void *data, std::size_t length);
	void schedule();

	friend class XBeeBot;
};

#endif

