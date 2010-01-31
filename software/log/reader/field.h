#ifndef LOG_READER_FIELD_H
#define LOG_READER_FIELD_H

#include "world/field.h"

class log_reader;

//
// A field based on data read from a log.
//
class log_reader_field : public field {
	public:
		//
		// A pointer to a log_reader_field.
		//
		typedef Glib::RefPtr<log_reader_field> ptr;

		//
		// Creates a new log_reader_field.
		//
		static ptr create(log_reader &reader) {
			ptr p(new log_reader_field(reader));
			return p;
		}

		//
		// Updates the field based on data read from a log.
		//
		void update();

		double length()               const { return len; }
		double total_length()         const { return tlen; }
		double width()                const { return wid; }
		double total_width()          const { return twid; }
		double goal_width()           const { return gwid; }
		double centre_circle_radius() const { return ccr; }
		double defense_area_radius()  const { return dar; }
		double defense_area_stretch() const { return das; }

	private:
		log_reader &reader;
		double len, tlen, wid, twid, gwid, ccr, dar, das;

		log_reader_field(log_reader &r);
};

#endif

