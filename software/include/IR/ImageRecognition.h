#ifndef IR_IMAGERECOGNITION_H
#define IR_IMAGERECOGNITION_H

#include "datapool/Noncopyable.h"
#include "datapool/Team.h"

#include <sigc++/sigc++.h>
#include <glibmm.h>

class ImageRecognition : private virtual Noncopyable, public virtual sigc::trackable {
public:
	static const unsigned int ANTIFLICKER_CIRCLE_RADIUS = 500;
	ImageRecognition(Team &friendly, Team &enemy);

private:
	int fd;

	bool onIO(Glib::IOCondition cond);
	bool onTimer();
};

#endif

