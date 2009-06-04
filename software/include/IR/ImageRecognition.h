#ifndef TB_IMAGERECOGNITION_H
#define TB_IMAGERECOGNITION_H

#include "datapool/DataSource.h"

class ImageRecognition : public DataSource {
public:
	ImageRecognition();
	virtual ~ImageRecognition();
	virtual void update();

private:
	ImageRecognition(const ImageRecognition &copyref); // Prohibit copying.
	int fd;
};

#endif

