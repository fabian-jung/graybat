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
 * @name Game of Life 
 * @example gol.cpp
 *
 * @brief Simple example that shows how to instantiate and use the Cage within a game of life.
 *
 */

// STL
#include <iostream>   /* std::cout */
#include <vector>     /* std::vector */
#include <array>      /* std::array */
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
// GRAYBAT patterns
#include <graybat/pattern/GridDiagonal.hpp>

struct Cell {
    Cell() : isAlive{{0}}, aliveNeighbors(0){
	unsigned random = rand() % 10000;
	if(random < 3125){
	    isAlive[0] = 1;
	}

    }
	
    std::array<unsigned, 1> isAlive;
    unsigned aliveNeighbors;

};


void printGolDomain(const std::vector<unsigned> domain, const unsigned width, const unsigned height, const unsigned generation){
    for(unsigned i = 0; i < domain.size(); ++i){
	if((i % (width)) == 0){
	    std::cerr << std::endl;
	}

	if(domain.at(i)){
	    std::cerr << "#";
	}
	else {
	    std::cerr << " ";
	}


    }
    std::cerr << "Generation: " << generation << std::endl;
    for(unsigned i = 0; i < height+1; ++i){
	std::cerr << "\033[F";
    }

}

template <class T_Cell>
void updateState(T_Cell &cell){

    switch(cell().aliveNeighbors){

    case 0:
    case 1:
	cell().isAlive[0] = 0;
	break;

    case 2:
	cell().isAlive[0] = cell().isAlive[0];
	break;
	    
    case 3: 
	cell().isAlive[0] = 1;
	break; 

    default: 
	cell().isAlive[0] = 0;
	break;

    };

}

std::array<unsigned,1> one{{1}};

int gol(const unsigned nCells, const unsigned nTimeSteps ) {
    /***************************************************************************
     * Configuration
     ****************************************************************************/

    // CommunicationPolicy
    typedef graybat::communicationPolicy::BMPI CP;
    
    // GraphPolicy
    typedef graybat::graphPolicy::BGL<Cell>    GP;
    
    // Cage
    typedef graybat::Cage<CP, GP> Cage;
    typedef typename Cage::Event  Event;
    typedef typename Cage::Vertex Vertex;

    /***************************************************************************
     * Initialize Communication
     ****************************************************************************/
    // Set Graph properties
    const unsigned height = sqrt(nCells);
    const unsigned width  = height;

    // Create GoL Graph
    Cage grid;    
    grid.setGraph(graybat::pattern::GridDiagonal<GP>(height, width));

    
    // Distribute vertices
    grid.distribute(graybat::mapping::Consecutive());

    /***************************************************************************
     * Run Simulation
     ****************************************************************************/
    std::vector<Event> events;   
    std::vector<unsigned> golDomain(grid.getVertices().size(), 0); 
    Vertex root = grid.getVertex(0);

    // Simulate life
    for(unsigned timestep = 0; timestep < nTimeSteps; ++timestep){

	// Print life field by owner of vertex 0
	if(grid.isHosting(root)){
	    printGolDomain(golDomain, width, height, timestep);
	}
	
	// Send cell state to neighbor cells
	std::vector<Event> es;	 
	for(Vertex &cell : grid.getHostedVertices()){
	    cell.spread(cell().isAlive, events);
	}

	// Recv cell state from neighbor cells and update own cell states
	for(Vertex &cell : grid.getHostedVertices()){
	    cell().aliveNeighbors = cell.accumulate(std::plus<unsigned>(), 0);
	    updateState(cell);
	}

	// Wait to finish events
	for(unsigned i = 0; i < events.size(); ++i){
	    events.back().wait();
	    events.pop_back();
	}

	// Gather state by vertex with id = 0
	for(Vertex &cell: grid.getHostedVertices()){
	    grid.gather(root, cell, cell().isAlive, golDomain, true);
	}
	
    }
    
    return 0;

}

int main(int argc, char** argv){

    if(argc < 3){
	std::cout << "Usage ./GoL [nCells] [nTimeteps]" << std::endl;
	return 0;
    }

    const unsigned nCells    = atoi(argv[1]);
    const unsigned nTimeSteps = atoi(argv[2]);


    gol(nCells, nTimeSteps);


    return 0;
}
