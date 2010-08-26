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

#include "../datatypes/pluginpointer.hpp"
#include "../base/is_base.hpp"
#include <vector>
#include "interactions/interaction.hpp"
#include "interactions/intEvent.hpp"
#include <boost/foreach.hpp>

class BoundaryCondition;
class Units;
class Species;
class GlobalEvent;
class Global;
class Local;
class LocalEvent;
class System;
class Topology;
class Particle;
template<class T> class CVector;
class Liouvillean;
class XMLNode;
class NEventData;
class PairEventData;
class ParticleEventData;
namespace xmlw
{
  class XmlStream;
}

class Dynamics: public DYNAMO::SimBase
{
public:
  //Constructors
  Dynamics(DYNAMO::SimData*);

  Dynamics(const XMLNode&, DYNAMO::SimData*);

  explicit Dynamics(const Dynamics &);

  ~Dynamics();
  
  void setUnits(Units*);

  void setLiouvillean(Liouvillean*);

  Interaction* addInteraction(Interaction*);

  void addSpecies(const ClonePtr<Species>&);
  
  void addGlobal(Global*);

  void addLocal(Local*);

  void addSystem(System*);

  void addStructure(Topology*);

  const Species& getSpecies(const Particle&) const;
  
  const ClonePtr<Interaction>& 
    getInteraction(const Particle&, const Particle&) const; 
  
  void stream(const Iflt&);
  
  inline IntEvent getEvent(const Particle& p1, const Particle& p2) const
  {
    BOOST_FOREACH(const ClonePtr<Interaction>& ptr, interactions)
      if (ptr->isInteraction(p1,p2))
	{
#ifdef DYNAMO_UpdateCollDebug
	  std::cerr << "\nGOT INTERACTION P1 = " << p1.getID() << " P2 = " 
		    << p2.getID() << " NAME = " << typeid(*ptr.get_ptr()).name();
#endif
	  return ptr->getEvent(p1,p2);
	}
    
    D_throw() << "Could not find the right interaction to test for";
  }

  void operator<<(const XMLNode&);

  void initialise();
  
  Iflt getLongestInteraction() const;
  
  /*! \brief Sets the Centre of Mass (COM) velocity of the system 
   * 
   *  The COM momentum of the system is
   * \f[ \bm{P}_{system} = \sum_i m_i \bm{v}_i \f]
   * 
   * We want to first remove any motion of the system, so we subtract
   * the COM momentum based on the mass of each particle (E.g. \f$ m_i
   * / \sum_j m_j\f$). This has two nice effects, first, particles
   * store their velocities, not their momentums so we convert by
   * dividing by \f$m_i\f$ which gives 
   *
   * \f[ \bm{v}_i \to \bm{v}_i -
   * (\sum_i m_i \bm{v}_i) / \sum_i m_i \f] 
   *
   * So relative velocities are preserved as the subtraction is a
   * constant for all particles. Also we can now just add the offset to give
   *
   * \f[ \bm{v}_i \to \bm{v}_i -(\sum_i m_i \bm{v}_i) / \sum_i m_i  + \bm{V}_{COM}\f]  
   *
   * \param COMVelocity The target velocity for the COM of the system.
   */  
  void setCOMVelocity(const Vector COMVelocity = Vector(0,0,0));

  void rescaleLengths(Iflt);

  void SystemOverlapTest();
  
  Iflt calcInternalEnergy() const;

  Dynamics* Clone() const { return new Dynamics(*this); }

  std::vector<ClonePtr<Interaction> >& getInteractions() { return interactions; }
  const std::vector<ClonePtr<Interaction> >& getInteractions() const { return interactions; }

  ClonePtr<Interaction>& getInteraction(std::string);
  const ClonePtr<Interaction>& getInteraction(std::string) const;

  const std::vector<ClonePtr<Global> >& getGlobals() const { return globals; }
  std::vector<ClonePtr<Global> >& getGlobals() { return globals; }
  ClonePtr<Global>& getGlobal(std::string);
  const ClonePtr<Global>& getGlobal(std::string) const;

  const std::vector<ClonePtr<Local> >& getLocals() const { return locals; }
  ClonePtr<Local>& getLocal(std::string);
  const ClonePtr<Local>& getLocal(std::string) const;

  const std::vector<ClonePtr<Species> >& getSpecies() const { return species; }
  const Species& getSpecies(std::string) const;

  std::vector<ClonePtr<Topology> >& getTopology() { return topology; }
  const std::vector<ClonePtr<Topology> >& getTopology() const { return topology; }

  ClonePtr<Topology>& getTopology(std::string);

  const ClonePtr<Topology>& getTopology(std::string) const;

  std::vector<ClonePtr<System> >& getSystemEvents() { return systems; }
  const std::vector<ClonePtr<System> >& getSystemEvents() const { return systems; }
  const ClonePtr<System>& getSystem(std::string) const;
  ClonePtr<System>& getSystem(std::string);

  void addSystemTicker();
  
  friend xmlw::XmlStream& operator<<(xmlw::XmlStream&, const Dynamics&);

  //Inlines
  inline const Units& units() const 
  { return *p_units; }

  inline Units& units()
  { return *p_units; }
  
  inline const BoundaryCondition& BCs() const 
  { return *p_BC; }

  inline const Liouvillean& getLiouvillean() const
  { return *p_liouvillean; }

  inline  Liouvillean& getLiouvillean()
  { return *p_liouvillean; }

  template<class T>
  inline bool liouvilleanTypeTest() const
  { return dynamic_cast<const T*>(p_liouvillean.get_ptr()) != NULL; }

  template<class T>
  inline bool BCTypeTest() const
  { return dynamic_cast<const T*>(p_BC.get_ptr()) != NULL; }

  template<class T>
  inline bool unitTypeTest() const
  { return dynamic_cast<const T*>(p_units.get_ptr()) != NULL; }

  //templates
  template<class T> void applyBC()
    {
      if (p_BC.empty())
	I_cout() << "Warning, resetting the BC's";
      
      p_BC.set_ptr(new T(Sim));
    }

  Iflt getNumberDensity() const;
  
  Iflt getPackingFraction() const;

 protected:
  void outputXML(xmlw::XmlStream &) const;

  std::vector<ClonePtr<Interaction> > interactions;
  std::vector<ClonePtr<Global> > globals;
  std::vector<ClonePtr<Local> > locals;
  std::vector<ClonePtr<System> > systems;
  std::vector<ClonePtr<Topology> > topology;
  std::vector<ClonePtr<Species> > species;
  ClonePtr<BoundaryCondition> p_BC;
  ClonePtr<Liouvillean> p_liouvillean;
  mutable ClonePtr<Units> p_units;
};
