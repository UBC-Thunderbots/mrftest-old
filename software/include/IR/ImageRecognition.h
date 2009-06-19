#ifndef IR_IMAGERECOGNITION_H
#define IR_IMAGERECOGNITION_H

#include "AI/AITeam.h"
#include "datapool/Noncopyable.h"
#include "datapool/Team.h"

#include <sigc++/sigc++.h>
#include <glibmm.h>

class ImageRecognition : private virtual Noncopyable, public virtual sigc::trackable {
public:
	ImageRecognition();

private:
	int fd;
	AITeam friendly;
	Team enemy;

	bool onIO(Glib::IOCondition cond);
};

#endif

