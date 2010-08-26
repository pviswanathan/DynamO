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

#ifndef OPMutualDiffusionE_H
#define OPMutualDiffusionE_H

#include <boost/circular_buffer.hpp>
#include "../outputplugin.hpp"

class OPMutualDiffusionE: public OutputPlugin
{
public:
  OPMutualDiffusionE(const DYNAMO::SimData*, const XMLNode&);
  
  virtual void operator<<(const XMLNode&);

  virtual OutputPlugin* Clone() const { return new OPMutualDiffusionE(*this); }

  virtual void stream(const Iflt);

  virtual void eventUpdate(const GlobalEvent&, const NEventData&);

  virtual void eventUpdate(const LocalEvent&, const NEventData&);

  virtual void eventUpdate(const System&, const NEventData&, const Iflt&);
  
  virtual void eventUpdate(const IntEvent&, const PairEventData&);

  virtual Iflt rescaleFactor();

  virtual void output(xmlw::XmlStream&);

  virtual void initialise();
 
  std::list<Vector  > getAvgAcc() const;
  
 protected:  
  virtual void updateDelG(const PairEventData&);

  virtual void updateDelG(const ParticleEventData&);

  virtual void updateDelG(const NEventData&);
    
  virtual void newG();
  
  virtual void accPass();

  Iflt getdt();
    
  boost::circular_buffer<Vector  > G1;
  boost::circular_buffer<Vector  > G2;
  std::vector<Vector  > accG;
  size_t count;
  Iflt dt, currentdt;

  Vector  delGsp1, delGsp2, Gsp1, Gsp2;

  size_t species1;
  size_t species2;
  
  Vector  sysMom;

  Iflt massFracSp1;
  Iflt massFracSp2;

  size_t CorrelatorLength;
  size_t currCorrLen;
  bool notReady;
  std::string species1name, species2name;

};

#endif
