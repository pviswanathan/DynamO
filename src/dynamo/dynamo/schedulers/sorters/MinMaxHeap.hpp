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

#include <dynamo/schedulers/sorters/event.hpp>
#include <magnet/xmlwriter.hpp>
#include <magnet/containers/MinMaxHeap.hpp>

namespace dynamo {
  //! There is a trick used here to speed up comparisons between
  //! MinMaxHeaps.  The top element is set to HUGE_VAL, whenever the
  //! queue is cleared, or pop'd empty. This means no conditional logic
  //! is required to deal with the comparison of empty queues.
  template<size_t Size>
  class PELMinMax
  {
    magnet::containers::MinMaxHeap<Event,Size> _innerHeap;

  public:
    PELMinMax() 
    { 
      //We use the iterators to do this as it doesn't check the size of
      //the heap!
      _innerHeap.begin()->dt = HUGE_VAL; 
    }

    inline size_t size() const { return _innerHeap.size(); }
    inline bool empty() const { return _innerHeap.empty(); }
    inline bool full() const { return _innerHeap.full(); }

    inline const Event& front() const { return *_innerHeap.begin(); }
    inline const Event& top() const { return front(); }  

    inline void pop() 
    { 
      _innerHeap.pop(); 
      if (empty())  _innerHeap.begin()->dt = HUGE_VAL; 
    }

    inline void clear() { _innerHeap.clear(); _innerHeap.begin()->dt = HUGE_VAL; }

    inline bool operator> (const PELMinMax& ip) const throw()
    { 
      return _innerHeap.begin()->dt > ip._innerHeap.begin()->dt; 
    }

    inline bool operator< (const PELMinMax& ip) const throw()
    { 
      return _innerHeap.begin()->dt < ip._innerHeap.begin()->dt; 
    }

    inline double getdt() const 
    {
      return _innerHeap.begin()->dt;
    }
  
    inline void stream(const double& ndt) throw()
    {
      BOOST_FOREACH(Event& dat, _innerHeap)
	dat.dt -= ndt;
    }

    inline void addTime(const double& ndt) throw()
    {
      BOOST_FOREACH(Event& dat, _innerHeap)
	dat.dt += ndt;
    }

    inline void push(const Event& __x)
    {
      if (!_innerHeap.full())
	_innerHeap.insert(__x);
      else 
	{
	  if (__x < _innerHeap.bottom())
	    _innerHeap.replaceMax(__x);
	  _innerHeap.unsafe_bottom().type = VIRTUAL;
	}
    }

    inline void rescaleTimes(const double& scale) throw()
    { 
      BOOST_FOREACH(Event& dat, _innerHeap)
	dat.dt *= scale;
    }

    inline void swap(PELMinMax& rhs)
    {
      _innerHeap.swap(rhs._innerHeap);
    }
  
  };
}

namespace std
{
  /*! \brief Template specialisation of the std::swap function for PELHeap*/
  template<size_t Size>
  void swap(dynamo::PELMinMax<Size>& lhs, dynamo::PELMinMax<Size>& rhs)
  {
    lhs.swap(rhs);
  }
}
