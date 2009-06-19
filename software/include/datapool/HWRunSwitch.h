#ifndef DATAPOOL_HWRUNSWITCH_H
#define DATAPOOL_HWRUNSWITCH_H

#include "datapool/Noncopyable.h"

#include <glibmm.h>

//
// Manages all attached hardware run switches.
//
class HWRunSwitch : public Glib::Object, private virtual Noncopyable {
public:
	//
	// Creates the HWRunSwitch. Only one instance may exist at a time.
	//
	HWRunSwitch();

	//
	// Destroys the HWRunSwitch.
	//
	~HWRunSwitch();

	//
	// Gets the currently-existing HWRunSwitch instance.
	//
	static HWRunSwitch &instance();

	//
	// Gets the property that is true when the switch is in RUN state and false in KILL state.
	//
	Glib::PropertyProxy<bool> property_state();

private:
	Glib::Property<bool> prop_state;
	static HWRunSwitch *inst;

	void onChange();
};

#endif

