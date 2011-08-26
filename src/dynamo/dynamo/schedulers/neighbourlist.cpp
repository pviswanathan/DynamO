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

#include <dynamo/schedulers/neighbourlist.hpp>
#include <dynamo/dynamics/interactions/intEvent.hpp>
#include <dynamo/simulation/particle.hpp>
#include <dynamo/dynamics/dynamics.hpp>
#include <dynamo/dynamics/liouvillean/liouvillean.hpp>
#include <dynamo/base/is_simdata.hpp>
#include <dynamo/dynamics/systems/system.hpp>
#include <dynamo/dynamics/globals/global.hpp>
#include <dynamo/dynamics/globals/globEvent.hpp>
#include <dynamo/dynamics/globals/neighbourList.hpp>
#include <dynamo/dynamics/locals/local.hpp>
#include <dynamo/dynamics/locals/localEvent.hpp>
#include <magnet/xmlreader.hpp>
#include <boost/bind.hpp>
#include <boost/progress.hpp>
#include <cmath>

namespace dynamo {
  void 
  SNeighbourList::operator<<(const magnet::xml::Node& XML)
  {
    sorter.set_ptr(CSSorter::getClass(XML.getNode("Sorter"), Sim));
  }

  void
  SNeighbourList::initialise()
  {
    try {
      NBListID = Sim->dynamics.getGlobal("SchedulerNBList")->getID();
    }
    catch(std::exception& cxp)
      {
	M_throw() << "Failed while finding the neighbour list global.\n"
		  << "You must have a neighbour list enabled for this\n"
		  << "scheduler called SchedulerNBList.\n"
		  << cxp.what();
      }
  
    if (dynamic_cast<const GNeighbourList*>
	(Sim->dynamics.getGlobals()[NBListID].get_ptr())
	== NULL)
      M_throw() << "The Global named SchedulerNBList is not a neighbour list!";

    static_cast<GNeighbourList&>
      (*Sim->dynamics.getGlobals()[NBListID].get_ptr())
      .markAsUsedInScheduler();

    dout << "Building all events on collision " << Sim->eventCount << std::endl;
    std::cout.flush();

    sorter->clear();
    //The plus one is because system events are stored in the last heap;
    sorter->resize(Sim->N+1);
    eventCount.clear();
    eventCount.resize(Sim->N+1, 0);

    //Now initialise the interactions
    {
      boost::progress_display prog(Sim->N);
 
      BOOST_FOREACH(const Particle& part, Sim->particleList)
	{
	  addEventsInit(part);
	  ++prog;
	}
    }
  
    sorter->init();

    rebuildSystemEvents();
  }

  void 
  SNeighbourList::rebuildList()
  { 
#ifdef DYNAMO_DEBUG
    initialise();
#else
    sorter->clear();
    //The plus one is because system events are stored in the last heap;
    sorter->resize(Sim->N+1);
    eventCount.clear();
    eventCount.resize(Sim->N+1, 0);

    BOOST_FOREACH(const Particle& part, Sim->particleList)
      addEventsInit(part);
  
    sorter->rebuild();
  
    rebuildSystemEvents();
#endif
  }

  void 
  SNeighbourList::outputXML(magnet::xml::XmlStream& XML) const
  {
    XML << magnet::xml::attr("Type") << "NeighbourList"
	<< magnet::xml::tag("Sorter")
	<< sorter
	<< magnet::xml::endtag("Sorter");
  }

  SNeighbourList::SNeighbourList(const magnet::xml::Node& XML, 
				   dynamo::SimData* const Sim):
    Scheduler(Sim,"NeighbourListScheduler", NULL)
  { 
    dout << "Neighbour List Scheduler Algorithmn Loaded" << std::endl;
    operator<<(XML);
  }

  SNeighbourList::SNeighbourList(dynamo::SimData* const Sim, CSSorter* ns):
    Scheduler(Sim,"NeighbourListScheduler", ns)
  { dout << "Neighbour List Scheduler Algorithmn Loaded" << std::endl; }

  void 
  SNeighbourList::addEvents(const Particle& part)
  {
    Sim->dynamics.getLiouvillean().updateParticle(part);
  
    //Add the global events
    BOOST_FOREACH(const magnet::ClonePtr<Global>& glob, Sim->dynamics.getGlobals())
      if (glob->isInteraction(part))
	sorter->push(glob->getEvent(part), part.getID());
  
#ifdef DYNAMO_DEBUG
    if (dynamic_cast<const GNeighbourList*>
	(Sim->dynamics.getGlobals()[NBListID].get_ptr())
	== NULL)
      M_throw() << "Not a GNeighbourList!";
#endif

    //Grab a reference to the neighbour list
    const GNeighbourList& nblist(*static_cast<const GNeighbourList*>
				 (Sim->dynamics.getGlobals()[NBListID]
				  .get_ptr()));
  
    //Add the local cell events
    nblist.getParticleLocalNeighbourhood
      (part, magnet::function::MakeDelegate(this, &Scheduler::addLocalEvent));

    //Add the interaction events
    nblist.getParticleNeighbourhood
      (part, magnet::function::MakeDelegate(this, &Scheduler::addInteractionEvent));  
  }

  void 
  SNeighbourList::addEventsInit(const Particle& part)
  {  
    Sim->dynamics.getLiouvillean().updateParticle(part);

    //Add the global events
    BOOST_FOREACH(const magnet::ClonePtr<Global>& glob, Sim->dynamics.getGlobals())
      if (glob->isInteraction(part))
	sorter->push(glob->getEvent(part), part.getID());
  
#ifdef DYNAMO_DEBUG
    if (dynamic_cast<const GNeighbourList*>
	(Sim->dynamics.getGlobals()[NBListID].get_ptr())
	== NULL)
      M_throw() << "Not a GNeighbourList!";
#endif

    //Grab a reference to the neighbour list
    const GNeighbourList& nblist(*static_cast<const GNeighbourList*>
				 (Sim->dynamics.getGlobals()[NBListID]
				  .get_ptr()));
  
    //Add the local cell events
    nblist.getParticleLocalNeighbourhood
      (part, magnet::function::MakeDelegate
       (this, &Scheduler::addLocalEvent));

    //Add the interaction events
    nblist.getParticleNeighbourhood
      (part, magnet::function::MakeDelegate
       (this, &Scheduler::addInteractionEventInit));  
  }
}
