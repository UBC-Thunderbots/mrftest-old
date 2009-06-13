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
	while(direction > 360) direction -= 360;
	
	while(direction < 0) direction += 360;
	
	if (direction<90)
	{
		direction = (direction/180.0)*M_PI;
		x = std::cos(direction);
		y = -1*std::sin(direction);
	}
	else if (direction<180)
	{
		direction = ((direction-90)/180.0)*M_PI;
		x = -1*std::sin(direction);
		y = -1*std::cos(direction);
	}
	else if (direction<270)
	{
		direction = ((direction-180)/180.0)*M_PI;
		x = -1*std::cos(direction);
		y = 1*std::sin(direction);
	}
	else
	{
		direction = ((direction-270)/180.0)*M_PI;
		x = 1*std::sin(direction);
		y = 1*std::cos(direction);
	}
}

double Vector2::angle() const {
	double angle = 0;
	if (x>=0 && y<=0)
		angle = (std::atan(-y/x)/M_PI)*180;
	else if (x<0 && y<0)
		angle = 90+((std::atan(-x/-y)/M_PI)*180);
	else if (x<0 && y>0)
		angle = 180+((std::atan(y/-x)/M_PI)*180);
	else
		angle = 270+((std::atan(x/y)/M_PI)*180);
	return angle;
}

