#include <deferred.h>
#include <registers.h>

static deferred_fn_t *head = 0, *tail = 0;

void deferred_fn_register(deferred_fn_t *deferred, void (*fn)(void *), void *cookie) {
	deferred->fn = fn;
	deferred->cookie = cookie;
	if (!deferred->next) {
		if (tail) {
			tail->next = deferred;
			tail = deferred;
		} else {
			head = tail = deferred;
		}
	}
	SCS_ICSR = PENDSVSET;
}

void deferred_fn_pendsv_handler(void) {
	if (head) {
		deferred_fn_t *pfn = head;
		head = pfn->next;
		if (!head) {
			tail = 0;
		}
		if (pfn->fn) {
			pfn->fn(pfn->cookie);
		}
	}

	if (head) {
		SCS_ICSR = PENDSVSET;
	}
}

