/*  dynamo:- Event driven molecular dynamics simulator 
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
#include <dynamo/dynamics/locals/local.hpp>
#include <magnet/math/vector.hpp>

namespace dynamo {
  class LAndersenWall: public Local
  {
  public:
    LAndersenWall(const magnet::xml::Node&, dynamo::SimData*);

    LAndersenWall(dynamo::SimData*, double, Vector , Vector , 
		   std::string, Range*);

    virtual ~LAndersenWall() {}

    virtual LocalEvent getEvent(const Particle &) const;

    virtual void runEvent(const Particle&, const LocalEvent&) const;

    virtual void initialise(size_t);

    virtual void operator<<(const magnet::xml::Node&);

    virtual bool isInCell(const Vector &, const Vector &) const;

  protected:
    virtual void outputXML(magnet::xml::XmlStream&) const;

    Vector  vNorm;
    Vector  vPosition;
    double sqrtT;
  };
}
