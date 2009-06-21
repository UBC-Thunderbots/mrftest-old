#ifndef TEST_H
#define TEST_H

/*
 * Checks whether a test has already been run and data is pending.
 */
uint8_t test_has_run(void);

/*
 * Adds a data point to the test dump.
 */
void test_add(double elem);

/*
 * Checks whether the data dump is full.
 */
uint8_t test_full(void);

/*
 * Saves the dump data to EEPROM.
 */
void test_save(void);

/*
 * Waits for serial characters, dumps the data, and halts.
 */
void test_dump(void);

#endif

