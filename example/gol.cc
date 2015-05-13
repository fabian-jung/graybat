// GrayBat
#include <graybat.hpp>

// Mappings
#include <mapping/Consecutive.hpp>
#include <mapping/Random.hpp>
#include <mapping/Roundrobin.hpp>
//#include <mapping/GraphPartition.hpp>

// Pattern
#include <pattern/GridDiagonal.hpp>

/** @name Game of Life Example
 *
 */


// STL
#include <iostream>   /* std::cout */
#include <vector>     /* std::vector */
#include <array>      /* std::array */
#include <cmath>      /* sqrt */
#include <cstdlib>    /* atoi */

std::array<unsigned,1> testValue {{1}};

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
void updateState(std::vector<T_Cell> &cells){
    for(T_Cell &cell : cells){
	updateState(cell);

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


int gol(const unsigned nCells, const unsigned nTimeSteps ) {
    /***************************************************************************
     * Configuration
     ****************************************************************************/

    // CommunicationPolicy
    typedef graybat::communicationPolicy::BMPI CP;
    
    // GraphPolicy
    typedef graybat::graphPolicy::BGL<Cell>    GP;
    
    // Cage
    typedef graybat::Cage<CP, GP>   MyCage;
    typedef typename MyCage::Event  Event;
    typedef typename MyCage::Vertex Vertex;
    typedef typename MyCage::Edge   Edge;

    /***************************************************************************
     * Initialize Communication
     ****************************************************************************/
    // Set Graph properties
    const unsigned height = sqrt(nCells);
    const unsigned width  = height;

    // Create GoL Graph
    MyCage grid(graybat::pattern::GridDiagonal(height, width));
    
    // Distribute vertices
    grid.distribute(graybat::mapping::Roundrobin());

    /***************************************************************************
     * Run Simulation
     ****************************************************************************/
     std::vector<Event> events;   
     std::vector<unsigned> golDomain(grid.getVertices().size(), 0); 
     Vertex root = grid.getVertex(0);

     // Simulate life
     for(unsigned timestep = 0; timestep < nTimeSteps; ++timestep){

	 // Print life field by owner of vertex 0
    	 // if(grid.peerHostsVertex(root)){
    	 //     printGolDomain(golDomain, width, height, timestep);
    	 // }
	
    	// Send state to neighbor cells
    	for(Vertex &v : grid.hostedVertices){
    	    for(Edge &edge : grid.getOutEdges(v)){
		//std::cout << "Send to Vertex:" << edge.vertex.id << " over Edge:" << edge.id << std::endl;

		//std::cerr << v().isAlive[0] << std::endl;
	
		events.push_back(edge << v().isAlive);
	    }

	}

     	// Recv state from neighbor cells
     	 for(Vertex &v : grid.hostedVertices){
    	     for(Edge &edge : grid.getInEdges(v)){
	 	 //std::cout << "Recv from Vertex:" << edge.vertex.id << " over Edge:" << edge.id << std::endl;
		 std::array<unsigned, 1> isAlive{{0}};
	 	 edge >> isAlive;

		 if(isAlive[0]) {
		     v().aliveNeighbors++;
		 }

	     }
	 }



    	// Wait to finish events
    	for(unsigned i = 0; i < events.size(); ++i){
    	    events.back().wait();
    	    events.pop_back();
    	}


    	// Calculate state for next generation
    	updateState(grid.hostedVertices);

	//Gather state by vertex with id = 0
	for(Vertex &v: grid.hostedVertices){
	    //std::cout << v().aliveNeighbors << std::endl;
	    v().aliveNeighbors = 0;
	    grid.gather(root, v, v().isAlive, golDomain, true);
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
