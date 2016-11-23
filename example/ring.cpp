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

/**
 * @example ring.cpp
 *
 * @brief Data is send through a ring of nodes and every node
 *        transforms data with own function.
 *
 */

// STL
#include <iostream>   /* std::cout */
#include <vector>     /* std::vector */
#include <array>      /* std::array */

// GRAYBAT
#include <graybat/Cage.hpp>
#include <graybat/communicationPolicy/BMPI.hpp>
#include <graybat/graphPolicy/BGL.hpp>
// GRAYBAT mappings
#include <graybat/mapping/Consecutive.hpp>
#include <graybat/mapping/Random.hpp>
#include <graybat/mapping/Roundrobin.hpp>
// GRAYBAT patterns
#include <graybat/pattern/Ring.hpp>

struct Function {

    void process(std::tuple<unsigned, std::string> &a){
	std::get<0>(a)++;
	std::get<1>(a) += " world";
	
    }
    
};


int exp() {
    /***************************************************************************
     * Configuration
     ****************************************************************************/

    // CommunicationPolicy
    typedef graybat::communicationPolicy::BMPI CP;
    typedef CP::Config                         Config;
    
    // GraphPolicy
    typedef graybat::graphPolicy::BGL<Function>    GP;
    
    // Cage
    typedef graybat::Cage<CP, GP> Cage;
    typedef typename Cage::Event  Event;
    typedef typename Cage::Vertex Vertex;

    /***************************************************************************
     * Initialize Communication
     ****************************************************************************/
    const unsigned nRingLinks = 50;

    // Create GoL Graph
    Config config;
    Cage cage(config);
    assert(cage.getPeers().size() >= nRingLinks);

    // Create ring communication pattern
    cage.setGraph(graybat::pattern::Ring<GP>(nRingLinks));

    
    // Distribute vertices
    cage.distribute(graybat::mapping::Roundrobin());

    
    /***************************************************************************
     * Run Simulation
     ****************************************************************************/
    std::vector<Event> events;

    std::array<std::tuple<unsigned, std::string>, 1> input{{std::make_tuple(0, "hello")}};
    std::array<std::tuple<unsigned, std::string>, 1> output;
    std::array<std::tuple<unsigned, std::string>, 1> intermediate;

    const Vertex stimula = cage.getVertex(0);

    for(Vertex v : cage.getHostedVertices()){

    	// Entry Vertex
    	if(v == stimula){
    	    v.spread(input, events);
    	    std::cout << "Input: " << std::get<0>(input[0]) << std::endl;	    	    
    	}

	while(true){
	    v.collect(intermediate);
	    v().process(intermediate[0]);
	    std::cout << "Increment: " << std::get<0>(intermediate[0]) << std::endl;
	    v.spread(intermediate);
		
	}
	    
    }
    
    return 0;

}

int main(){
    exp();
    return 0;
}
