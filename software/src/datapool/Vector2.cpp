#include "datapool/Vector2.h"

// Adapted from CMU Ubersim Vector2 class:

/*=========================================================================
UberSim Source Code Release
-------------------------------------------------------------------------
Copyright (C) 2002 Manuela Veloso, Brett Browning, Mike Bowling,
                   James Bruce; {mmv, brettb, mhb, jbruce}@cs.cmu.edu
                   Erick Tryzelaar {erickt}@andrew.cmu.edu
School of Computer Science, Carnegie Mellon University
-------------------------------------------------------------------------
This software is distributed under the GNU General Public License,
version 2.  If you do not have a copy of this licence, visit
www.gnu.org, or write: Free Software Foundation, 59 Temple Place,
Suite 330 Boston, MA 02111-1307 USA.  This program is distributed
in the hope that it will be useful, but WITHOUT ANY WARRANTY,
including MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
-------------------------------------------------------------------------*/

Vector2::Vector2(double direction)
{
	direction = direction / 180.0 * M_PI;
	x = std::cos(direction);
	y = -std::sin(direction);
}

double Vector2::angle() const {
	double ans = std::atan2(-y, x) * 180.0 / M_PI;
	if (ans < 0) ans += 360.0;
	return ans;
}

