/*  DYNAMO:- Event driven molecular dynamics simulator 
    http://www.marcusbannerman.co.uk/dynamo
    Copyright (C) 2011  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

    This program is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    version 3 as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "../../extcode/xmlwriter.hpp"
#include "point.hpp"

class SpFixedCollider:public SpPoint
{
public:  
  SpFixedCollider(DYNAMO::SimData* sim, CRange* r, std::string nName, 
		  unsigned int ID, std::string nIName="Bulk"):
    SpPoint(sim, r, 0, nName, ID, nIName)
  { name = "SpFixedCollider"; }
  
  SpFixedCollider(const XMLNode& XML, DYNAMO::SimData* nSim, unsigned int nID):
    SpPoint(nSim, NULL, 0, "", nID,"")
  { name = "SpFixedCollider"; operator<<(XML); }
  
  virtual void initialise();

  virtual void operator<<(const XMLNode& XML);

  virtual SpFixedCollider* Clone() const { return new SpFixedCollider(*this); }

protected:

  virtual void outputXML(xml::XmlStream& XML) const;
};
