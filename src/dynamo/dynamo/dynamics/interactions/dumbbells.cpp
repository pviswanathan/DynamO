/*  dynamo:- Event driven molecular dynamics simulator 
    http://www.marcusbannerman.co.uk/dynamo
    Copyright (C) 2011  Marcus N Campbell Bannerman <m.bannerman@gmail.com>
    Copyright (C) 2011  Sebastian Gonzalez <tsuresuregusa@gmail.com>

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

#include <dynamo/dynamics/interactions/dumbbells.hpp>
#include <dynamo/dynamics/interactions/intEvent.hpp>
#include <dynamo/dynamics/liouvillean/liouvillean.hpp>
#include <dynamo/dynamics/units/units.hpp>
#include <dynamo/base/is_simdata.hpp>
#include <dynamo/dynamics/2particleEventData.hpp>
#include <dynamo/dynamics/BC/BC.hpp>
#include <dynamo/dynamics/ranges/1range.hpp>
#include <dynamo/schedulers/scheduler.hpp>
#include <dynamo/dynamics/NparticleEventData.hpp>
#include <magnet/xmlwriter.hpp>
#include <magnet/xmlreader.hpp>
#include <cmath>
#include <iomanip>

namespace dynamo  {
  IDumbbells::IDumbbells(const magnet::xml::Node& XML, dynamo::SimData* tmp):
    Interaction(tmp, NULL)
  {
    operator<<(XML);
  }

  void 
  IDumbbells::initialise(size_t nID)
  {
    ID = nID; 
  
    ISingleCapture::initCaptureMap(Sim->particleList);
  }

  void 
  IDumbbells::operator<<(const magnet::xml::Node& XML)
  { 
    if (strcmp(XML.getAttribute("Type"),"Dumbbells"))
      M_throw() << "Attempting to load Dumbbells from non Dumbbells entry";
  
    Interaction::operator<<(XML);
  
    try 
      {
	_length = Sim->_properties.getProperty(XML.getAttribute("Length"),
					       Property::Units::Length());
	_e = Sim->_properties.getProperty(XML.getAttribute("Elasticity"),
					  Property::Units::Dimensionless());
	_diameter = Sim->_properties.getProperty(XML.getAttribute("Diameter"),
						 Property::Units::Length());
	intName = XML.getAttribute("Name");
	ISingleCapture::loadCaptureMap(XML);
      }
    catch (boost::bad_lexical_cast &)
      {
	M_throw() << "Failed a lexical cast in CIDumbbells";
      }
  }

  double 
  IDumbbells::maxIntDist() const 
  { return _length->getMaxValue() + _diameter->getMaxValue(); }

  double 
  IDumbbells::getDiameter(size_t ID, size_t subID) const
  { return _diameter->getProperty(ID); }

  Vector 
  IDumbbells::getPosition(size_t ID, size_t subID) const
  {
    Vector retval = Sim->particleList[ID].getPosition();
    Sim->dynamics.BCs().applyBC(retval);

    double l = _length->getProperty(ID);
    //Flip the direction depending on if the ID is odd or even
    l *=  0.5 * (1 - int(2 * (subID % 2)));

    retval += Sim->dynamics.getLiouvillean()
      .getRotData(Sim->particleList[ID]).orientation * l;

    return retval;
  }


  double 
  IDumbbells::getExcludedVolume(size_t ID) const 
  {
    double diam = _diameter->getProperty(ID);
    double length = _length->getProperty(ID);

    //The volume of the two spheres not overlapping
    double vol = 2 * diam * diam * diam * M_PI / 6.0;;

    //If the spheres are not overlapping just return the total volume
    if (length >= diam) return vol;

    //If they overlap, subtract the lens volume between the overlapping
    //spheres
    return vol - (1.0/12.0) * M_PI * (2 * diam + length) * std::pow(diam - length, 2); 
  }

  IntEvent 
  IDumbbells::getEvent(const Particle &p1,
		       const Particle &p2) const 
  {
#ifdef DYNAMO_DEBUG
    if (!Sim->dynamics.getLiouvillean().isUpToDate(p1))
      M_throw() << "Particle 1 is not up to date";
  
    if (!Sim->dynamics.getLiouvillean().isUpToDate(p2))
      M_throw() << "Particle 2 is not up to date";

    if (p1 == p2)
      M_throw() << "You shouldn't pass p1==p2 events to the interactions!";
#endif 
  
    double d = (_diameter->getProperty(p1.getID())
		+ _diameter->getProperty(p2.getID())) * 0.5;

    double l = (_length->getProperty(p1.getID())
		+ _length->getProperty(p2.getID())) * 0.5;
  
    if (isCaptured(p1, p2)) 
      {
	//Run this to determine when the spheres no longer intersect
	double dt = Sim->dynamics.getLiouvillean().SphereSphereOutRoot(p1, p2, l + d);

	double dt_offcenter = Sim->dynamics.getLiouvillean()
	  .getOffCenterSphereOffCenterSphereCollision(l, d, p1, p2, dt);

	if (dt_offcenter < dt)
	  return IntEvent(p1, p2, dt_offcenter, CORE, *this);
      
	return IntEvent(p1, p2, dt, WELL_OUT, *this);
      }
  
    double dt = Sim->dynamics.getLiouvillean()
      .SphereSphereInRoot(p1, p2, l + d);
    
    if (dt != HUGE_VAL)
      return IntEvent(p1, p2, dt, WELL_IN, *this);
  
    return IntEvent(p1, p2, HUGE_VAL, NONE, *this);
  }

  void
  IDumbbells::runEvent(const Particle& p1, 
		       const Particle& p2,
		       const IntEvent& iEvent) const
  {
    double d = (_diameter->getProperty(p1.getID())
		+ _diameter->getProperty(p2.getID())) * 0.5;

    double l = (_length->getProperty(p1.getID())
		+ _length->getProperty(p2.getID())) * 0.5;

    double e = (_e->getProperty(p1.getID())
		+ _e->getProperty(p2.getID())) * 0.5;

    switch (iEvent.getType())
      {
      case CORE:
	{
	  ++Sim->eventCount;
	  //We have a line interaction! Run it
	  PairEventData retval(Sim->dynamics.getLiouvillean()
			       .runOffCenterSphereOffCenterSphereCollision
			       (iEvent, e, l, d));

	  Sim->signalParticleUpdate(retval);
	
	  Sim->ptrScheduler->fullUpdate(p1, p2);
	
	  BOOST_FOREACH(shared_ptr<OutputPlugin> & Ptr, 
			Sim->outputPlugins)
	    Ptr->eventUpdate(iEvent, retval);

	  break;
	}
      case WELL_IN:
	{
	  addToCaptureMap(p1, p2);

	  //Unfortunately we cannot be smart as this well event may have
	  //been pushed into both particles update lists, therefore we
	  //must do a full update
	  Sim->ptrScheduler->fullUpdate(p1, p2);

	  Sim->freestreamAcc += iEvent.getdt();
	  break;
	}
      case WELL_OUT:
	{
	  removeFromCaptureMap(p1, p2);

	  //Unfortunately we cannot be smart as this well event may have
	  //been pushed into both particles update lists, therefore we
	  //must do a full update
	  Sim->ptrScheduler->fullUpdate(p1, p2);

	  Sim->freestreamAcc += iEvent.getdt();
	  break;
	}
      default:
	M_throw() << "Unknown collision type";
      }
  }
   
  void 
  IDumbbells::outputXML(magnet::xml::XmlStream& XML) const
  {
    XML << magnet::xml::attr("Type") << "Dumbbells"
	<< magnet::xml::attr("Length") << _length->getName()
	<< magnet::xml::attr("Elasticity") << _e->getName()
	<< magnet::xml::attr("Diameter") <<  _diameter->getName()
	<< magnet::xml::attr("Name") << intName
	<< *range;

    ISingleCapture::outputCaptureMap(XML);
  }

  bool 
  IDumbbells::captureTest(const Particle& p1, const Particle& p2) const
  {
    if (&(*(Sim->dynamics.getInteraction(p1, p2))) != this) return false;

    double d = (_diameter->getProperty(p1.getID())
		+ _diameter->getProperty(p2.getID())) * 0.5;

    double l = (_length->getProperty(p1.getID())
		+ _length->getProperty(p2.getID())) * 0.5;

    return Sim->dynamics.getLiouvillean().sphereOverlap(p1, p2, l + d);
  }

  void
  IDumbbells::checkOverlaps(const Particle& part1, const Particle& part2) const
  {}
}
