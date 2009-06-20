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
#include <vector>
#include <gtkmm.h>
#include <getopt.h>

namespace {
	void usage(const char *app) {
		std::cerr <<
			"Usage:\n"
			<< app << " options\n"
			"Options are:\n"
			"  -s\n"
			"  --simulator\n"
			"    Turns on the simulator. By default, image recognition and XBee are used.\n"
			"\n"
			"  -v\n"
			"  --visualizer\n"
			"    Turns on the visualizer. By default, no visualization is shown.\n"
			"\n"
			"  -d\n"
			"  --debug\n"
			"    Enables debugging output. By default, only informational, warning, and error messages are output.\n"
			"\n"
			"  -e\n"
			"  --enemy-ai\n"
			"    Enables AI control of the enemy team. By default, this only occurs in the simulator.\n";
	}

	class AIUpdater : public virtual sigc::trackable, private virtual Noncopyable {
	public:
		AIUpdater(const std::vector<Updateable *> &updateables) : updateables(updateables), timer(71428571ULL) {
			timer.signal_expire().connect(sigc::mem_fun(*this, &AIUpdater::onExpire));
		}

	private:
		const std::vector<Updateable *> &updateables;
		IntervalTimer timer;

		void onExpire() {
			for (unsigned int i = 0; i < updateables.size(); i++)
				updateables[i]->update();
		}
	};

	void execute(std::vector<Updateable *> &updateables) {
		updateables.insert(updateables.begin(), &World::get());
		AIUpdater updater(updateables);
		Gtk::Main::run();
	}

	void execute(bool useVis, std::vector<Updateable *> &updateables) {
		if (useVis) {
			Visualizer vis;
			updateables.push_back(&vis);
			execute(updateables);
		} else {
			execute(updateables);
		}
	}
	
	void execute(bool useSim, bool useVis, Team &friendly, Team &enemy) {
		std::vector<Updateable *> updateables;
		if (useSim) {
			Simulator sim(friendly, enemy);
			updateables.push_back(&sim);
			execute(useVis, updateables);
		} else {
			XBeeBotSet xbee;
			HWRunSwitch runSwitch;
			ControlPanel cp;
			ImageRecognition ir(friendly, enemy);
			execute(useVis, updateables);
		}
	}

	void execute(bool useSim, bool useVis, bool useEnemyAI) {
		Config config;
		RefBox refBox;

		AITeam friendly(0);
		if (useEnemyAI) {
			AITeam enemy(1);
			execute(useSim, useVis, friendly, enemy);
		} else {
			Team enemy(1);
			execute(useSim, useVis, friendly, enemy);
		}
	}
}

int main(int argc, char **argv) {
	// Create GTK main object.
	Gtk::Main kit(argc, argv);

	// Read remaining options.
	bool useSim = false;
	bool useVis = false;
	bool useEnemyAI = false;
	static const option longopts[] = {
		{"simulator", 0, 0, 's'},
		{"visualizer", 0, 0, 'v'},
		{"debug", 0, 0, 'd'},
		{"enemy-ai", 0, 0, 'e'},
		{0, 0, 0, 0}
	};
	static const char shortopts[] = "svde";
	int ch;
	while ((ch = getopt_long(argc, argv, shortopts, longopts, 0)) != -1) {
		switch (ch) {
			case 's':
				useSim = true;
				useEnemyAI = true;
				break;

			case 'v':
				useVis = true;
				break;

			case 'd':
				Log::setLevel(Log::LEVEL_DEBUG);
				break;

			case 'e':
				useEnemyAI = true;
				break;

			case '?':
				usage(argv[0]);
				return 1;
		}
	}

	// Run!
	execute(useSim, useVis, useEnemyAI);

	return 0;
}

