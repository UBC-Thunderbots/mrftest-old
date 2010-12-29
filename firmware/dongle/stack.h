#ifndef STACK_H
#define STACK_H

/**
 * \file
 *
 * \brief Provides convenient and type-safe ways to define linked-list-based stacks.
 *
 * To use the definitions in this file, the structure being stored needs to have a field of type pointer to itself called \c next.
 */

/**
 * \brief Evaluates to the name of a stack of some type.
 *
 * \param[in] etype the element type to hold.
 */
#define STACK_TYPE(etype) stack_of_ ## etype

/**
 * \brief Defines the type of a stack of elements.
 *
 * \param[in] etype the element type to hold.
 */
#define STACK_DEFINE_TYPE(etype) typedef struct { __data etype *head; } STACK_TYPE(etype)

/**
 * \brief An initializer for an empty stack.
 */
#define STACK_INITIALIZER { 0 }

/**
 * \brief Initializes a stack.
 *
 * \param[in] q the stack to initialize.
 */
#define STACK_INIT(q) do { (q).head = 0; } while (0)

/**
 * \brief Pushes an element on a stack.
 *
 * \param[in] q the stack to push an element onto.
 *
 * \param[in] elt the element to push.
 */
#define STACK_PUSH(q, elt) do { (elt)->next = (q).head; (q).head = (elt); } while (0)

/**
 * \brief Returns the next element in a stack.
 *
 * \param[in] q the stack to examine.
 *
 * \return the next element, or null if the \p q is empty.
 */
#define STACK_TOP(q) ((q).head)

/**
 * \brief Removes the first element from a stack.
 *
 * \param[in] q the stack to pop.
 */
#define STACK_POP(q) do { if ((q).head) { (q).head = (q).head->next; } } while (0)

#endif

