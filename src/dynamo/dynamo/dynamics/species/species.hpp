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

#include <dynamo/base.hpp>
#include <dynamo/dynamics/ranges/1range.hpp>
#include <dynamo/base/is_simdata.hpp>
#include <string>

namespace magnet { namespace xml { class Node; } }
namespace coil { class DataSet; }
namespace xml { class XmlStream; }

namespace dynamo {
  class Particle;
  class Interaction;
  class RenderObj;

  class Species: public dynamo::SimBase
  {
  public:
    virtual ~Species();

    inline bool isSpecies(const Particle& p1) const { return range->isInRange(p1); }  

    inline const double& getMass(size_t ID) const { return _mass->getProperty(ID); }
    inline unsigned long getCount() const { return range->size(); }
    inline unsigned int getID() const { return ID; }
    inline const std::string& getName() const { return spName; }
    inline const std::string& getIntName() const { return intName; }
    inline const shared_ptr<Range>& getRange() const { return range; }
    inline const Interaction* getIntPtr() const { return IntPtr; }
    inline void setIntPtr(Interaction* nPtr) { IntPtr = nPtr; }
    virtual double getScalarMomentOfInertia(size_t ID) const = 0;

    virtual void operator<<(const magnet::xml::Node&) = 0;

    virtual void initialise() = 0;

    friend magnet::xml::XmlStream& operator<<(magnet::xml::XmlStream&, const Species&);
  
    static shared_ptr<Species> getClass(const magnet::xml::Node&, dynamo::SimData*, size_t);

#ifdef DYNAMO_visualizer
    virtual shared_ptr<coil::DataSet> createDataSet() const;
    virtual void initDataSet() const;
    virtual void updateRenderData() const;
  protected:

    mutable shared_ptr<coil::DataSet> _renderData;
#endif

  protected:
    template<class T1>
    Species(dynamo::SimData* tmp, std::string name, 
	    Range* nr, T1 mass, std::string nName, 
	    unsigned int nID, std::string nIName):
      SimBase(tmp, name),
      _mass(Sim->_properties.getProperty(mass, Property::Units::Mass())),
      range(nr),
      spName(nName),
      intName(nIName),
      IntPtr(NULL),
      ID(nID)
    {}

    virtual void outputXML(magnet::xml::XmlStream&) const = 0;
  
    shared_ptr<Property> _mass;
    shared_ptr<Range> range;
    std::string spName;
    std::string intName;
    Interaction* IntPtr;
    unsigned int ID;
  };

  inline magnet::xml::XmlStream& operator<<(magnet::xml::XmlStream& XML, const Species& g)
  {
    g.outputXML(XML);
    return XML;
  }
}
