#ifndef BRAC_H
#define BRAC_H

//
// User-level command processor.
//

#include "less.hpp"

namespace bracket {

void match_brac(int obrac, int cbrac, int forwdir, int n);

}; // namespace bracket

#endif