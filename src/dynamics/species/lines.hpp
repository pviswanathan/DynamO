/*  DYNAMO:- Event driven molecular dynamics simulator 
    http://www.marcusbannerman.co.uk/dynamo
    Copyright (C) 2010  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

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

#include "sphericalTop.hpp"

class SpLines : public SpSphericalTop
{
public:
  SpLines(DYNAMO::SimData* Sim, CRange* R, double nMass, std::string nName, 
	  unsigned int ID, double r, std::string nIName="Bulk"):
    SpSphericalTop(Sim, R, nMass, nName, ID, r * r / 12.0,  nIName)
  {}
  
  SpLines(const XMLNode& XML, DYNAMO::SimData* Sim, unsigned int ID):
    SpSphericalTop(XML, Sim, ID)
  {}

  virtual Species* Clone() const { return new SpLines(*this); }

#ifdef DYNAMO_visualizer
  virtual magnet::thread::RefPtr<RenderObj>& getCoilRenderObj() const;

  virtual void updateRenderObj(magnet::CL::CLGLState&) const;
#endif

protected:

  virtual void outputXML(xml::XmlStream& XML) const 
  { SpSphericalTop::outputXML(XML, "Lines"); }

#ifdef DYNAMO_visualizer
  mutable std::vector<cl_float> particleData;
#endif

};

