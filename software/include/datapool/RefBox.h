#ifndef DATAPOOL_REFBOX_H
#define DATAPOOL_REFBOX_H

#include "datapool/Noncopyable.h"

#include <glibmm.h>
#include <sigc++/sigc++.h>

//
// A receiver for commands sent by the SSL referee box.
//
class RefBox : public Noncopyable, public virtual sigc::trackable {
public:
	//
	// Constructs a new RefBox.
	//
	RefBox();

	//
	// Destroys a RefBox.
	//
	~RefBox();

private:
	int fd, last_count;

	bool onIO(Glib::IOCondition cond);
};

#endif

