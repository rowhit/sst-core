// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include "sst/core/serialization.h"
#include "sst/core/exit.h"

#ifdef HAVE_MPI
#include <boost/mpi.hpp>
#endif

#include <sst/core/debug.h>
#include "sst/core/component.h"
#include "sst/core/simulation.h"
#include "sst/core/timeConverter.h"

namespace SST {

Exit::Exit( Simulation* sim, TimeConverter* period, bool single_rank ) :
    Action(),
//     m_functor( new EventHandler<Exit,bool,Event*> (this,&Exit::handler ) ),
    m_refCount( 0 ),
    m_period( period ),
    single_rank(single_rank)	
{
    _EXIT_DBG("\n");
    
    setPriority(EXITPRIORITY);
    // if (!single_rank) sim->insertActivity( period->getFactor(), this );
}

Exit::~Exit()
{
    m_idSet.clear();
}
    
bool Exit::refInc( ComponentId_t id )
{
    _EXIT_DBG( "refCount=%d\n", m_refCount );

    if ( m_idSet.find( id ) != m_idSet.end() ) {
	CompMap_t comp_map = Simulation::getSimulation()->getComponentMap();
	bool found_in_map = false;

	for(CompMap_t::iterator comp_map_itr = comp_map.begin();
		comp_map_itr != comp_map.end();
		++comp_map_itr) {

		if(comp_map_itr->second->getId() == id) {
			found_in_map = true;
			break;
		}
	}

	if(found_in_map) {
		_DBG( Exit, "component (%s) multiple increment\n",
                	Simulation::getSimulation()->getComponent(id)->getName().c_str() );
	} else {
		_DBG( Exit, "component in construction increments exit multiple times.\n" );
	}

        return true;
    }

    m_idSet.insert( id );

    ++m_refCount;

    return false;
}

bool Exit::refDec( ComponentId_t id )
{
    _EXIT_DBG("refCount=%d\n",m_refCount );

    if ( m_idSet.find( id ) == m_idSet.end() ) {
        _DBG( Exit, "component (%s) multiple decrement\n",
                Simulation::getSimulation()->getComponent(id)->getName().c_str() );
        return true;
    } 


    if ( m_refCount == 0 ) {
        _abort( Exit, "refCount is already 0\n" );
        return true;
    }

    m_idSet.erase( id );

    --m_refCount;

    if ( m_refCount == 0 ) {
        end_time = Simulation::getSimulation()->getCurrentSimCycle();
    }
    
    if ( single_rank && m_refCount == 0 ) {
        // std::cout << "Exiting..." << std::endl;
        Simulation* sim = Simulation::getSimulation();
        // sim->insertActivity( sim->getCurrentSimCycle() + m_period->getFactor(), this );
        sim->insertActivity( sim->getCurrentSimCycle() + 1, this );
    }

    return false;
}

void
Exit::execute()
{
    check();

    SimTime_t next = Simulation::getSimulation()->getCurrentSimCycle() + m_period->getFactor();
    Simulation::getSimulation()->insertActivity( next, this );
    
}
    
// bool Exit::handler( Event* e )
void Exit::check( void )
{
    Simulation *sim = Simulation::getSimulation();

    _EXIT_DBG("%lu\n", (unsigned long) sim->getCurrentSimCycle());

    int value = ( m_refCount > 0 );
    int out;

#ifdef HAVE_MPI
    boost::mpi::communicator world; 
    all_reduce( world, &value, 1, &out, std::plus<int>() );  
#else
    out = value;
#endif

    _EXIT_DBG("%d\n",out);

    // If out is 0, then it's time to end
    if ( !out ) {
#ifdef HAVE_MPI
        // Do an all_reduce to get the end_time
        SimTime_t end_value;
        all_reduce( world, &end_time, 1, &end_value, boost::mpi::maximum<SimTime_t>() );
        end_time = end_value;
#endif
        endSimulation(end_time);
    }
    // else {  
    //     // Reinsert into TimeVortex.  We do this even when ending so that
    //     // it will get deleted with the TimeVortex on termination.  We do
    //     // this in case we exit with a StopEvent instead.  In that case,
    //     // there is no way to know if the Exit object is deleted in the
    //     // TimeVortex or not, so we just make sure it is always deleted
    //     // there.
    //     SimTime_t next = sim->getCurrentSimCycle() + 
    //         m_period->getFactor();
    //     sim->insertActivity( next, this );
    // }
}


template<class Archive>
void
Exit::serialize(Archive & ar, const unsigned int version)
{
    printf("begin Exit::serialize\n");
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Action);
    ar & BOOST_SERIALIZATION_NVP(m_refCount);
    ar & BOOST_SERIALIZATION_NVP(m_period);
    ar & BOOST_SERIALIZATION_NVP(m_idSet);
    ar & BOOST_SERIALIZATION_NVP(single_rank);
    printf("end Exit::serialize\n");
}

} // namespace SST


SST_BOOST_SERIALIZATION_INSTANTIATE(SST::Exit::serialize)
BOOST_CLASS_EXPORT_IMPLEMENT(SST::Exit);
