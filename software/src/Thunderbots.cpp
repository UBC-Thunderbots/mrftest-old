#include <iostream>
#include <cstdlib>
#include <cstring>
#include <memory>
using namespace std;

#include <gtkmm/main.h>
#include <gtkmm/window.h>

#include "datapool/EmergencyStopButton.h"
#include "datapool/RefBox.h"
#include "datapool/Team.h"
#include "datapool/World.h"
#include "AI/AITeam.h"
#include "AI/Simulator.h"
#include "AI/Visualizer.h"
#include "IR/ImageRecognition.h"
#include "UI/ControlPanel.h"
#include "XBee/XBee.h"

namespace {
	class ControlPanelWindow : public Gtk::Window {
	public:
		ControlPanelWindow() {
			set_title("Thunderbots Control Panel");
			add(cp);
			show_all();
		}

		void update() {
			cp.update();
		}

	private:
		ControlPanel cp;
	};
}

int main(int argc, char **argv) {
	// Initialize random number generator
	srand(time(0));

	// Read options.
	bool useSim = false;
	bool useVis = false;
	for (unsigned int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-s") == 0) {
			useSim = true;
		} else if (strcmp(argv[i], "-v") == 0) {
			useVis = true;
		} else if (strcmp(argv[i], "-h") == 0) {
			cerr << "Usage:\n" << argv[0] << " [-s] [-v]\n-s: use simulator (instead of IR)\n-v: use visualizer\n";
			return 1;
		} else {
			cerr << "Unrecognized option: " << argv[i] << '\n';
			return 1;
		}
	}

	// Create objects.
	Gtk::Main kit(0, 0);
	auto_ptr<DataSource> ds;
	auto_ptr<ControlPanelWindow> cp;
	if (useSim)
		ds.reset(new Simulator);
	else {
		XBee::init();
		EmergencyStopButton::init();
		ds.reset(new ImageRecognition);
		cp.reset(new ControlPanelWindow);
	}

	auto_ptr<Visualizer> vis;
	if (useVis) {
		vis.reset(new Visualizer);
	}

	RefBox::init();

	// Run!
	for (;;) {
		EmergencyStopButton::update();
		ds->update();
		RefBox::update();
		World::get().update();
		if (useVis)
			vis->update();
		if (!useSim)
			cp->update();
	}
}

