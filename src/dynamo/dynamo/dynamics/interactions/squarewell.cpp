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

#include <dynamo/dynamics/interactions/squarewell.hpp>
#include <dynamo/dynamics/BC/BC.hpp>
#include <dynamo/dynamics/dynamics.hpp>
#include <dynamo/dynamics/units/units.hpp>
#include <dynamo/dynamics/globals/global.hpp>
#include <dynamo/simulation/particle.hpp>
#include <dynamo/dynamics/interactions/intEvent.hpp>
#include <dynamo/dynamics/species/species.hpp>
#include <dynamo/dynamics/2particleEventData.hpp>
#include <dynamo/dynamics/liouvillean/liouvillean.hpp>
#include <dynamo/base/is_simdata.hpp>
#include <dynamo/schedulers/scheduler.hpp>
#include <dynamo/dynamics/NparticleEventData.hpp>
#include <magnet/xmlwriter.hpp>
#include <magnet/xmlreader.hpp>
#include <cmath>
#include <iomanip>

namespace dynamo {
  ISquareWell::ISquareWell(const magnet::xml::Node& XML, dynamo::SimData* tmp):
    Interaction(tmp, NULL) //A temporary value!
  {
    operator<<(XML);
  }

  void 
  ISquareWell::operator<<(const magnet::xml::Node& XML)
  {
    if (strcmp(XML.getAttribute("Type"),"SquareWell"))
      M_throw() << "Attempting to load SquareWell from non SquareWell entry";
  
    Interaction::operator<<(XML);
  
    try {
      _diameter = Sim->_properties.getProperty(XML.getAttribute("Diameter"),
					       Property::Units::Length());
      _lambda = Sim->_properties.getProperty(XML.getAttribute("Lambda"),
					     Property::Units::Dimensionless());
      _wellDepth = Sim->_properties.getProperty(XML.getAttribute("WellDepth"),
						Property::Units::Energy());

      if (XML.hasAttribute("Elasticity"))
	_e = Sim->_properties.getProperty(XML.getAttribute("Elasticity"),
					  Property::Units::Dimensionless());
      else
	_e = Sim->_properties.getProperty(1.0, Property::Units::Dimensionless());
      intName = XML.getAttribute("Name");
      ISingleCapture::loadCaptureMap(XML);   
    }
    catch (boost::bad_lexical_cast &)
      {
	M_throw() << "Failed a lexical cast in CISquareWell";
      }
  }

  double 
  ISquareWell::getDiameter(size_t ID, size_t subID) const
  { return _diameter->getProperty(ID); }

  Vector 
  ISquareWell::getPosition(size_t ID, size_t subID) const
  { 
    Vector retval = Sim->particleList[ID].getPosition();
    Sim->dynamics.BCs().applyBC(retval);
    return retval;
  }


  double 
  ISquareWell::getExcludedVolume(size_t ID) const 
  { 
    double diam = _diameter->getProperty(ID);
    return diam * diam * diam * M_PI / 6.0; 
  }

  double 
  ISquareWell::maxIntDist() const 
  { return _diameter->getMaxValue() * _lambda->getMaxValue(); }

  void 
  ISquareWell::initialise(size_t nID)
  {
    ID = nID;
    ISingleCapture::initCaptureMap(Sim->particleList);
  }

  bool 
  ISquareWell::captureTest(const Particle& p1, const Particle& p2) const
  {
    if (&(*(Sim->dynamics.getInteraction(p1, p2))) != this) return false;

    double d = (_diameter->getProperty(p1.getID())
		+ _diameter->getProperty(p2.getID())) * 0.5;

    double l = (_lambda->getProperty(p1.getID())
		+ _lambda->getProperty(p2.getID())) * 0.5;

#ifdef DYNAMO_DEBUG
    if (Sim->dynamics.getLiouvillean().sphereOverlap(p1, p2, d))
      derr << "Warning! Two particles might be overlapping"
	   << "Overlap is " << Sim->dynamics.getLiouvillean().sphereOverlap(p1, p2, d) 
	/ Sim->dynamics.units().unitLength()
	   << "\nd = " << d / Sim->dynamics.units().unitLength() << std::endl;
#endif
 
    return Sim->dynamics.getLiouvillean().sphereOverlap(p1, p2, l * d);
  }

  IntEvent
  ISquareWell::getEvent(const Particle &p1, 
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

    double l = (_lambda->getProperty(p1.getID())
		+ _lambda->getProperty(p2.getID())) * 0.5;

    IntEvent retval(p1, p2, HUGE_VAL, NONE, *this);

    if (isCaptured(p1, p2))
      {
	double dt = Sim->dynamics.getLiouvillean()
	  .SphereSphereInRoot(p1, p2, d);
	if (dt != HUGE_VAL)
	  {
#ifdef DYNAMO_OverlapTesting
	if (Sim->dynamics.getLiouvillean().sphereOverlap(p1, p2, d))
	  M_throw() << "Overlapping particles found"
		    << ", particle1 " << p1.getID()
		    << ", particle2 " << p2.getID()
		    << "\nOverlap = " 
		    << Sim->dynamics.getLiouvillean()
	    .sphereOverlap(p1, p2, d)
	    / Sim->dynamics.units().unitLength();
#endif	    
	    retval = IntEvent(p1, p2, dt, CORE, *this);
	  }

	dt = Sim->dynamics.getLiouvillean().SphereSphereOutRoot(p1, p2, l * d);
	if (retval.getdt() > dt)
	    retval = IntEvent(p1, p2, dt, WELL_OUT, *this);
      }
    else
      {
	double dt = Sim->dynamics.getLiouvillean()
	  .SphereSphereInRoot(p1, p2, l * d);

      if (dt != HUGE_VAL)
	{
#ifdef DYNAMO_OverlapTesting
	  if (Sim->dynamics.getLiouvillean().sphereOverlap(p1, p2, l * d))
	    {
	      if (Sim->dynamics.getLiouvillean().sphereOverlap(p1, p2, d))
		M_throw() << "Overlapping cores (but not registerd as captured) particles found in square well" 
			  << "\nparticle1 " << p1.getID() << ", particle2 " 
			  << p2.getID() << "\nOverlap = " 
			  << Sim->dynamics.getLiouvillean().sphereOverlap(p1, p2, d)
		  / Sim->dynamics.units().unitLength();
	      else
		M_throw() << "Overlapping wells (but not registerd as captured) particles found" 
			  << "\nparticle1 " << p1.getID() << ", particle2 " 
			  << p2.getID() << "\nOverlap = " 
			  << Sim->dynamics.getLiouvillean().sphereOverlap(p1, p2, l * d)
		  / Sim->dynamics.units().unitLength();
	      
	    }
#endif
	  retval = IntEvent(p1, p2, dt, WELL_IN, *this);
	}
      }

    return retval;
  }

  void
  ISquareWell::runEvent(const Particle& p1, 
			const Particle& p2,
			const IntEvent& iEvent) const
  {
    ++Sim->eventCount;

    double d = (_diameter->getProperty(p1.getID())
		+ _diameter->getProperty(p2.getID())) * 0.5;
    double d2 = d * d;

    double e = (_e->getProperty(p1.getID())
		+ _e->getProperty(p2.getID())) * 0.5;

    double l = (_lambda->getProperty(p1.getID())
		+ _lambda->getProperty(p2.getID())) * 0.5;
    double ld2 = d * l * d * l;

    double wd = (_wellDepth->getProperty(p1.getID())
		 + _wellDepth->getProperty(p2.getID())) * 0.5;
    switch (iEvent.getType())
      {
      case CORE:
	{
	  PairEventData retVal(Sim->dynamics.getLiouvillean().SmoothSpheresColl(iEvent, e, d2, CORE));
	  Sim->signalParticleUpdate(retVal);
	
	  Sim->ptrScheduler->fullUpdate(p1, p2);
	
	  BOOST_FOREACH(shared_ptr<OutputPlugin> & Ptr, Sim->outputPlugins)
	    Ptr->eventUpdate(iEvent, retVal);

	  break;
	}
      case WELL_IN:
	{
	  PairEventData retVal(Sim->dynamics.getLiouvillean()
			       .SphereWellEvent(iEvent, wd, ld2));
	
	  if (retVal.getType() != BOUNCE)
	    addToCaptureMap(p1, p2);      
	
	  Sim->ptrScheduler->fullUpdate(p1, p2);
	  Sim->signalParticleUpdate(retVal);
	
	  BOOST_FOREACH(shared_ptr<OutputPlugin> & Ptr, Sim->outputPlugins)
	    Ptr->eventUpdate(iEvent, retVal);


	  break;
	}
      case WELL_OUT:
	{
	  PairEventData retVal(Sim->dynamics.getLiouvillean()
			       .SphereWellEvent(iEvent, -wd, ld2));
	
	  if (retVal.getType() != BOUNCE)
	    removeFromCaptureMap(p1, p2);      

	  Sim->signalParticleUpdate(retVal);

	  Sim->ptrScheduler->fullUpdate(p1, p2);
	
	  BOOST_FOREACH(shared_ptr<OutputPlugin> & Ptr, Sim->outputPlugins)
	    Ptr->eventUpdate(iEvent, retVal);
	  break;
	}
      default:
	M_throw() << "Unknown collision type";
      } 
  }

  void
  ISquareWell::checkOverlaps(const Particle& part1, const Particle& part2) const
  {
    Vector  rij = part1.getPosition() - part2.getPosition();
    Sim->dynamics.BCs().applyBC(rij);
    double r2 = rij.nrm2();

    double d = (_diameter->getProperty(part1.getID())
		+ _diameter->getProperty(part2.getID())) * 0.5;

    double l = (_lambda->getProperty(part1.getID())
		+ _lambda->getProperty(part2.getID())) * 0.5;
  
    double d2 = d * d;
    double ld2 = d * l * d * l;

    if (isCaptured(part1, part2))
      {
	if (r2 < d2)
	  derr << "Possible captured overlap occured in diagnostics\n ID1=" << part1.getID() 
	       << ", ID2=" << part2.getID() << "\nR_ij^2=" 
	       << r2 / pow(Sim->dynamics.units().unitLength(),2)
	       << "\nd^2=" 
	       << d2 / pow(Sim->dynamics.units().unitLength(),2) << std::endl;
      
	if (r2 > ld2)
	  derr << "Possible escaped captured pair in diagnostics\n ID1=" << part1.getID() 
	       << ", ID2=" << part2.getID() << "\nR_ij^2=" 
	       << r2 / pow(Sim->dynamics.units().unitLength(),2)
	       << "\n(lambda * d)^2=" 
	       << ld2 / pow(Sim->dynamics.units().unitLength(),2) << std::endl;
      }
    else 
      if (r2 < ld2)
	{
	  if (r2 < d2)
	    derr << "Overlap error\n ID1=" << part1.getID() 
		 << ", ID2=" << part2.getID() << "\nR_ij^2=" 
		 << r2 / pow(Sim->dynamics.units().unitLength(),2)
		 << "\n(d)^2=" 
		 << d2 / pow(Sim->dynamics.units().unitLength(),2) << std::endl;
	  else
	    derr << "Possible missed captured pair in diagnostics\n ID1=" << part1.getID() 
		 << ", ID2=" << part2.getID() << "\nR_ij^2=" 
		 << r2 / pow(Sim->dynamics.units().unitLength(),2)
		 << "\n(lambda * d)^2=" 
		 << ld2 / pow(Sim->dynamics.units().unitLength(),2) << std::endl;
	}
  }
  
  void 
  ISquareWell::outputXML(magnet::xml::XmlStream& XML) const
  {
    XML << magnet::xml::attr("Type") << "SquareWell"
	<< magnet::xml::attr("Diameter") << _diameter->getName()
	<< magnet::xml::attr("Elasticity") << _e->getName()
	<< magnet::xml::attr("Lambda") << _lambda->getName()
	<< magnet::xml::attr("WellDepth") << _wellDepth->getName()
	<< magnet::xml::attr("Name") << intName
	<< *range;
  
    ISingleCapture::outputCaptureMap(XML);  
  }

  double 
  ISquareWell::getInternalEnergy() const
  { 
    //Once the capture maps are loaded just iterate through that determining energies
    double Energy = 0.0;
    typedef std::pair<size_t, size_t> locpair;

    BOOST_FOREACH(const locpair& IDs, captureMap)
      Energy += 0.5 * (_wellDepth->getProperty(IDs.first)
		       +_wellDepth->getProperty(IDs.second));
  
    return -Energy; 
  }

  double 
  ISquareWell::getInternalEnergy(const Particle& p1, const Particle& p2) const
  {
    return - 0.5 * (_wellDepth->getProperty(p1.getID())
		    +_wellDepth->getProperty(p2.getID()))
      * isCaptured(p1, p2);
  }
}
