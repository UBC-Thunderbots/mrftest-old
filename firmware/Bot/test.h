#ifndef TEST_H
#define TEST_H

/*
 * Adds a data point to the test dump.
 */
void test_add(double elem);

/*
 * Checks whether the data dump is full.
 */
uint8_t test_full(void);

/*
 * Waits for serial characters, dumps the data, and halts.
 */
void test_dump(void);

#endif

