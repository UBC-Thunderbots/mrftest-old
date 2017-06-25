#include "ai/ai.h"
#include "ai/setup.h"
#include "util/cacheable.h"
#include "util/dprint.h"
#include "util/object_store.h"
#include <memory>

using AI::AIPackage;
using AI::BE::Backend;

namespace {
}

AIPackage::AIPackage(Backend &backend) : backend(backend), high_level(std::unique_ptr<AI::HL::HighLevel>()), show_hl_overlay(true) {
	backend.signal_tick().connect(sigc::mem_fun(this, &AIPackage::tick));
	backend.signal_draw_overlay().connect(sigc::mem_fun(this, &AIPackage::draw_overlay));
	high_level.signal_changed().connect(sigc::mem_fun(this, &AIPackage::save_setup));
	high_level.signal_changed().connect(sigc::mem_fun(this, &AIPackage::init_ai_notes));
	backend.defending_end().signal_changed().connect(sigc::mem_fun(this, &AIPackage::save_setup));
	backend.friendly_colour().signal_changed().connect(sigc::mem_fun(this, &AIPackage::save_setup));
}

void AIPackage::tick() {
	// Clear all cached data.
	CacheableBase::flush_all();

	// If we have a HighLevel installed, tick it.
	if (high_level.get()) {
		high_level->tick();
	}
}

void AIPackage::init_ai_notes() {
	if (high_level.get()) {
		high_level->ai_notes.signal_changed().connect(sigc::mem_fun(this, &AIPackage::ai_notes_changed));
	}
	signal_ai_notes_changed.emit(u8"");
}

void AIPackage::ai_notes_changed() {
	signal_ai_notes_changed.emit(high_level->ai_notes);
}

void AIPackage::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const {
	if (show_hl_overlay) {
		if (high_level.get()) {
			high_level->draw_overlay(ctx);
		}
	}
}

void AIPackage::save_setup() const {
	AI::Setup setup;
	setup.high_level_name = high_level.get() ? high_level->factory().name() : u8"";
	setup.defending_end = backend.defending_end();
	setup.friendly_colour = backend.friendly_colour();
	setup.save();
}

