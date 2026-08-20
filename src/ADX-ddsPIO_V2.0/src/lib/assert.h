#ifndef THIRD_PARTY_ASSERT_H
#define THIRD_PARTY_ASSERT_H

#include <stdbool.h>

/*
 * Declare assert_ only once.
 * Other headers must NOT redeclare it, or -Wredundant-decls will trigger.
 */

void assert_(bool val);
void assert_checkpoint(bool val, int n_blink);

#endif /* THIRD_PARTY_ASSERT_H */
