#ifndef QUEUE_H
#define QUEUE_H

/**
 * \file
 *
 * \brief Provides convenient and type-safe ways to define linked-list-based queues.
 *
 * To use the definitions in this file, the structure being stored needs to have a field of type pointer to itself called \c next.
 */

/**
 * \brief Evaluates to the name of a queue of some type.
 *
 * \param[in] etype the element type to hold.
 */
#define QUEUE_TYPE(etype) queue_of_ ## etype

/**
 * \brief Defines the type of a queue of elements.
 *
 * \param[in] etype the element type to hold.
 */
#define QUEUE_DEFINE_TYPE(etype) typedef struct { __data etype *head; __data etype *tail; } QUEUE_TYPE(etype)

/**
 * \brief An initializer for an empty queue.
 */
#define QUEUE_INITIALIZER { 0, 0 }

/**
 * \brief Initializes a queue.
 *
 * \param[in] q the queue to initialize.
 */
#define QUEUE_INIT(q) do { (q).head = 0; } while (0)

/**
 * \brief Pushes an element on a queue.
 *
 * \param[in] q the queue to push an element onto.
 *
 * \param[in] elt the element to push.
 */
#define QUEUE_PUSH(q, elt) do { if ((q).head) { (q).tail->next = (elt); } else { (q).head = (elt); } (q).tail = (elt); (elt)->next = 0; } while (0)

/**
 * \brief Returns the next element in a queue.
 *
 * \param[in] q the queue to examine.
 *
 * \return the next element, or null if the \p q is empty.
 */
#define QUEUE_FRONT(q) ((q).head)

/**
 * \brief Removes the first element from a queue.
 *
 * \param[in] q the queue to pop.
 */
#define QUEUE_POP(q) do { if ((q).head) { (q).head = (q).head->next; } } while (0)

#endif

