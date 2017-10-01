#include "drive/null/dongle.h"
#include <cassert>

Drive::Null::Dongle::Dongle() : bot(*this)
{
}

Drive::Robot &Drive::Null::Dongle::robot(unsigned int i)
{
    assert(!i);
    return bot;
}
