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

#include <dynamo/dynamics/ranges/2RAll.hpp>
#include <dynamo/simulation/particle.hpp>
#include <magnet/xmlwriter.hpp>
#include <magnet/xmlreader.hpp>
#include <cstring>

namespace dynamo {
  C2RAll::C2RAll(const magnet::xml::Node& XML, const dynamo::SimData*)
  { 
    if (strcmp(XML.getAttribute("Range"),"2All"))
      M_throw() << "Attempting to load a 2All from a non 2All";
  }

  void 
  C2RAll::operator<<(const magnet::xml::Node&)
  {
    M_throw() << "Due to problems with RAll C2RAll operator<< cannot work for this class";
  }

  void 
  C2RAll::outputXML(magnet::xml::XmlStream& XML) const
  {
    XML << magnet::xml::attr("Range") << "2All"; 
  }
}
