#include "datapool/Config.h"
#include "datapool/HWRunSwitch.h"
#include "datapool/IntervalTimer.h"
#include "datapool/Noncopyable.h"
#include "datapool/RefBox.h"
#include "datapool/Team.h"
#include "datapool/World.h"
#include "AI/AITeam.h"
#include "AI/Simulator.h"
#include "AI/Visualizer.h"
#include "IR/ImageRecognition.h"
#include "Log/Log.h"
#include "UI/ControlPanel.h"
#include "XBee/XBeeBot.h"

#include <iostream>
#include <memory>
#include <gtkmm.h>
#include <getopt.h>

namespace {
	void usage(const char *app) {
		std::cerr << "Usage:\n" << app << " [-s|--simulate|--simulator} [-v|--visualize|--visualizer] [-d|--debug]\n";
	}

	class AIUpdater : public virtual sigc::trackable, private virtual Noncopyable {
	public:
		AIUpdater(const std::auto_ptr<Visualizer> &vis, const std::auto_ptr<Simulator> &sim) : vis(vis), sim(sim), timer(71428571ULL) {
			timer.signal_expire().connect(sigc::mem_fun(*this, &AIUpdater::onExpire));
		}

	private:
		const std::auto_ptr<Visualizer> &vis;
		const std::auto_ptr<Simulator> &sim;
		IntervalTimer timer;

		void onExpire() {
			World::get().update();
			if (sim.get())
				sim->update();
			if (vis.get())
				vis->update();
		}
	};
}

int main(int argc, char **argv) {
	// Create GTK main object.
	Gtk::Main kit(argc, argv);

	// Read remaining options.
	bool useSim = false;
	bool useVis = false;
	static const option longopts[] = {
		{"simulate", 0, 0, 's'},
		{"simulation", 0, 0, 's'},
		{"visualize", 0, 0, 'v'},
		{"visualizer", 0, 0, 'v'},
		{"debug", 0, 0, 'd'},
		{0, 0, 0, 0}
	};
	static const char shortopts[] = "svd";
	int ch;
	while ((ch = getopt_long(argc, argv, shortopts, longopts, 0)) != -1) {
		switch (ch) {
			case 's':
				useSim = true;
				break;

			case 'v':
				useVis = true;
				break;

			case 'd':
				Log::setLevel(Log::LEVEL_DEBUG);
				break;

			case '?':
				usage(argv[0]);
				return 1;
		}
	}

	// Create objects.
	Config config;
	std::auto_ptr<XBeeBotSet> xbee;
	std::auto_ptr<HWRunSwitch> runSwitch;
	std::auto_ptr<Simulator> sim;
	std::auto_ptr<ImageRecognition> ir;
	std::auto_ptr<ControlPanel> cp;
	if (useSim)
		sim.reset(new Simulator);
	else {
		xbee.reset(new XBeeBotSet);
		runSwitch.reset(new HWRunSwitch);
		ir.reset(new ImageRecognition);
		cp.reset(new ControlPanel);
	}

	std::auto_ptr<Visualizer> vis;
	if (useVis) {
		vis.reset(new Visualizer);
	}

	RefBox refBox;

	AIUpdater upd(vis, sim);

	// Run!
	Gtk::Main::run();
}

