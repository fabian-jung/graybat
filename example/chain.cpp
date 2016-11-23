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
 * @example chain.cpp
 *
 * @brief Data is send through a chain of compute 
 *        nodes and every node increments the value.
 *
 */

// STL
#include <iostream>   /* std::cout */
#include <vector>     /* std::vector */
#include <array>      /* std::array */
#include <functional> /* std::bind */
#include <cmath>      /* sqrt */
#include <cstdlib>    /* atoi */
#include <numeric>    /* std::accumulate */

// GRAYBAT
#include <graybat/Cage.hpp>
#include <graybat/communicationPolicy/BMPI.hpp>
#include <graybat/graphPolicy/BGL.hpp>
// GRAYBAT mappings
#include <graybat/mapping/Consecutive.hpp>
#include <graybat/mapping/Random.hpp>
#include <graybat/mapping/Roundrobin.hpp>
#include <graybat/mapping/Filter.hpp>
// GRAYBAT pattern
#include <graybat/pattern/Chain.hpp>


struct Tag {
    std::size_t tag;
    
};

int exp() {
    /***************************************************************************
     * Configuration
     ****************************************************************************/

    // CommunicationPolicy
    typedef graybat::communicationPolicy::BMPI CP;
    typedef CP::Config                         Config;    
    
    // GraphPolicy
    typedef graybat::graphPolicy::BGL<Tag, Tag>    GP;
    
    // Cage
    typedef graybat::Cage<CP, GP> Cage;
    typedef typename Cage::Event  Event;
    typedef typename Cage::Vertex Vertex;

    /***************************************************************************
     * Initialize Communication
     ****************************************************************************/
    const unsigned nChainLinks = 6;
    auto inc = [](unsigned &a){a++;};

    // Create GoL Graph
    Config config;
    Cage cage(config);

    cage.setGraph(graybat::pattern::Chain<GP>(nChainLinks));
    
    // Distribute vertices
    cage.distribute(graybat::mapping::Filter(cage.getCommunicationPolicy()->getGlobalContext().getVAddr() % 3));

    /***************************************************************************
     * Run Simulation
     ****************************************************************************/
    std::vector<Event> events;

    std::array<unsigned, 1> input {{0}};
    std::array<unsigned, 1> output {{0}};
    std::array<unsigned, 1> intermediate {{0}};

    const Vertex entry = cage.getVertex(0);
    const Vertex exit  = cage.getVertex(nChainLinks-1);

    for(Vertex v : cage.getHostedVertices()){

        if(v == entry){
            v.spread(input, events);
            std::cout << "Input: " << input[0] << " " << cage.getCommunicationPolicy()->getGlobalContext().getVAddr() << std::endl;
        }

        if(v == exit){
            v.collect(output);
            std::cout << "Output: " << output[0] << " " << cage.getCommunicationPolicy()->getGlobalContext().getVAddr() << std::endl;
        }

        if(v != entry and v != exit){
            v.collect(intermediate);
            inc(intermediate[0]);
            std::cout << "Increment: " << intermediate[0] << " " << cage.getCommunicationPolicy()->getGlobalContext().getVAddr() << std::endl;
            v.spread(intermediate, events);
	    
        }
	
    }

    for(unsigned i = 0; i < events.size(); ++i){
        events.back().wait();
        events.pop_back();
    }
    
    return 0;

}

int main(){
    exp();
    return 0;
}
