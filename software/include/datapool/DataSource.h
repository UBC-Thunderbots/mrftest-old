#include <tr1/memory>
class DataSource;
typedef std::tr1::shared_ptr<DataSource> PDataSource;

#ifndef TB_DATASOURCE_H
#define TB_DATASOURCE_H

/*
 * An abstract source of data about the world. Either ImageRecognition or Simulator.
 */
class DataSource {
public:
	/*
	 * Constructs the DataSource.
	 */
	DataSource() {
	}

	/*
	 * Destroys the DataSource.
	 */
	virtual ~DataSource() {
	}

	/*
	 * Updates the state of the world.
	 */
	virtual void update() = 0;

private:
	DataSource(const DataSource &copyref); // Prohibit copying.
};

#endif

