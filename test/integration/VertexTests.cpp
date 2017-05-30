/**
 * Copyright 2016 Erik Zenker
 *
 * This file is part of Graybat.
 *
 * Graybat is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Graybat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Graybat.
 * If not, see <http://www.gnu.org/licenses/>.
 */

// BOOST
#include <boost/test/unit_test.hpp>
#include <boost/hana/tuple.hpp>

// STL
#include <vector>
#include <functional> // std::plus

// GRAYBAT
#include <graybat/Cage.hpp>
#include <graybat/communicationPolicy/BMPI.hpp>
#include <graybat/communicationPolicy/ZMQ.hpp>
#include <graybat/serializationPolicy/ByteCast.hpp>
#include <graybat/graphPolicy/BGL.hpp>
#include <graybat/mapping/Consecutive.hpp>
#include <graybat/pattern/Grid.hpp>
#include <graybat/pattern/Chain.hpp>

/*******************************************************************************
 * Test Suites
 *******************************************************************************/
BOOST_AUTO_TEST_SUITE( vertex )


/*******************************************************************************
 * Communication Policies to Test
 ******************************************************************************/
namespace hana = boost::hana;

using ZMQ        = graybat::communicationPolicy::ZMQ;
using BMPI       = graybat::communicationPolicy::BMPI;
using GP         = graybat::graphPolicy::BGL<>;
using Serialization = graybat::serializationPolicy::ByteCast;
using ZMQCage    = graybat::Cage<ZMQ, GP, Serialization >;
using BMPICage   = graybat::Cage<BMPI, GP, Serialization >;
using ZMQConfig  = ZMQ::Config;
using BMPIConfig = BMPI::Config;

ZMQConfig zmqConfig = {"tcp://127.0.0.1:5000",
                       "tcp://127.0.0.1:5001",
                       static_cast<size_t>(std::stoi(std::getenv("OMPI_COMM_WORLD_SIZE"))),
					   "context_vertex_test"};

BMPIConfig bmpiConfig;

ZMQCage zmqCage(zmqConfig);
BMPICage bmpiCage(bmpiConfig);

auto cages = hana::make_tuple(std::ref(zmqCage),
                              std::ref(bmpiCage) );

/***************************************************************************
 * Test Cases
 ****************************************************************************/
BOOST_AUTO_TEST_CASE( spread_collect ){
    hana::for_each(cages, [](auto cageRef){
	    // Test setup
            using Cage    = typename decltype(cageRef)::type;
            using GP      = typename Cage::GraphPolicy;
	    using Event   = typename Cage::Event;
	    using Vertex  = typename Cage::Vertex;

	    // Test run
	    {	
    
		std::vector<Event> events;
                auto& grid = cageRef.get();

		grid.setGraph(graybat::pattern::Grid<GP>(grid.getPeers().size(),
                                                         grid.getPeers().size()));
		
		grid.distribute(graybat::mapping::Consecutive());
    
		const unsigned nElements = 10;
		const unsigned testValue = 5;
    
		std::vector<unsigned> send(nElements, testValue);

    
		for(Vertex v : grid.getHostedVertices()){
		    v.spread(send, events);
        
		}

		for(Vertex v : grid.getHostedVertices()){
		    std::vector<unsigned> recv(nElements * v.nInEdges(), 0);
		    v.collect(recv);
		    for(unsigned r: recv){
			BOOST_CHECK_EQUAL(r, testValue);
            
		    }
        
		}

		// Wait to finish events
		for(unsigned i = 0; i < events.size(); ++i){
		    events.back().wait();
		    events.pop_back();
		}

	    }

	});
    
}


BOOST_AUTO_TEST_CASE( accumulate ){
    hana::for_each(cages, [](auto cageRef){
	    // Test setup
            using Cage    = typename decltype(cageRef)::type;
            using GP      = typename Cage::GraphPolicy;            
	    using Event   = typename Cage::Event;
	    using Vertex  = typename Cage::Vertex;

	    // Test run
	    {	
    
		std::vector<Event> events;
                auto& grid = cageRef.get();
                
		grid.setGraph(graybat::pattern::Grid<GP>(grid.getPeers().size(),
                                                         grid.getPeers().size()));
		
		grid.distribute(graybat::mapping::Consecutive());
    
		const unsigned nElements = 1;
		const unsigned testValue = 5;
    
		std::vector<unsigned> send(nElements, testValue);

    
		for(Vertex v : grid.getHostedVertices()){
		    v.spread(send, events);
        
		}

		for(Vertex v : grid.getHostedVertices()){
		    BOOST_CHECK_EQUAL(v.accumulate(std::plus<unsigned>(), 0),
				      testValue * nElements * v.nInEdges());
                
		}

		// Wait to finish events
		for(unsigned i = 0; i < events.size(); ++i){
		    events.back().wait();
		    events.pop_back();
		}
    
	    }

	});
    
}

BOOST_AUTO_TEST_CASE( forward ){
    hana::for_each(cages, [](auto cageRef){
	    // Test setup
            using Cage    = typename decltype(cageRef)::type;
            using GP      = typename Cage::GraphPolicy;            
	    using Event   = typename Cage::Event;
	    using Vertex  = typename Cage::Vertex;

	    // Test run
	    {	
    
		std::vector<Event> events;
                auto& chain = cageRef.get();
                
		chain.setGraph(graybat::pattern::Chain<GP>(chain.getPeers().size()));
		chain.distribute(graybat::mapping::Consecutive());
    
		const unsigned nElements = 100;
		const unsigned testValue = 1;
    
		std::vector<unsigned> input(nElements, testValue);
		std::vector<unsigned> output(nElements, 0);
		std::vector<unsigned> intermediate(nElements, 0);

		const Vertex entry = chain.getVertex(0);
		const Vertex exit  = chain.getVertex(chain.getVertices().size()-1);
    
    
		for(Vertex v : chain.getHostedVertices()){

		    if(v == entry){
			v.spread(input, events);
            
		    }

		    if(v == exit){
			v.collect(output);
			for(unsigned u : output){
			    if(chain.getVertices().size() == 1)
				BOOST_CHECK_EQUAL(u, testValue);
			    if(chain.getVertices().size() == 2)
				BOOST_CHECK_EQUAL(u, testValue);
			    if(chain.getVertices().size() > 2)
				BOOST_CHECK_EQUAL(u, testValue + (1 * chain.getVertices().size() - 2));
                    
			}
            
		    }

		    if(v != entry and v != exit){
			v.forward(intermediate, [](std::vector<unsigned> &v)->void{for(unsigned &u : v){u++;}});
            
		    }
            
        
		}

		// Wait to finish events
		for(unsigned i = 0; i < events.size(); ++i){
		    events.back().wait();
		    events.pop_back();
		}

	    }
	    
	});
    
}


BOOST_AUTO_TEST_SUITE_END()
