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

#pragma once

#include <numeric> // std::accumulate
#include <iostream> // std::cout
#include <vector>

namespace graybat {

    template <class T_Cage>
    struct CommunicationVertex {

        typedef unsigned                             VertexID; 
        typedef T_Cage                               Cage;
        typedef typename Cage::GraphPolicy           GraphPolicy;
        typedef typename Cage::Edge                  Edge;
        typedef typename Cage::Event                 Event;
        typedef typename GraphPolicy::VertexProperty VertexProperty;

        VertexID id;
        VertexProperty &vertexProperty;
        Cage &cage;

        CommunicationVertex(const VertexID id, VertexProperty &vertexProperty, Cage &cage) :
            id(id),
            vertexProperty(vertexProperty),
            cage(cage){
	    
        }

        /***************************************************************************
         * Graph Operations
         ****************************************************************************/
        VertexProperty& operator()(){
            return vertexProperty;
        }

        CommunicationVertex& operator=(const CommunicationVertex &other){
            id = other.id;
            vertexProperty = other.vertexProperty;

            return *this;
        }

    
        size_t nInEdges() const {
            return cage.getInEdges(*this).size();
        }

        size_t nOutEdges() const {
            return cage.getOutEdges(*this).size();
        }

        bool operator==(CommunicationVertex v){
            return (id == v.id);
        }

        bool operator!=(CommunicationVertex v){
            return (id != v.id);
        }

        /***************************************************************************
         * Communication Operations
         ****************************************************************************/

        template <class T_Data>
        void spread(const T_Data& data, std::vector<Event> &events){
            cage.spread(*this, data, events);
        }

        template <class T_Data>
        void spread(const T_Data& data){
            cage.spread(*this, data);
        }
    

        template <class T_Data>
        void collect(T_Data& data){
            cage.collect(*this, data);
	
        }

        template <class T_Data, class T_Functor>
        void forward(T_Data& data, T_Functor f){
            cage.collect(*this, data);
            f(data);
            cage.spread(*this, data);
	
        }

        template <class T_Data>
        void forward(T_Data& data){
            assert(nInEdges() == nOutEdges());
            cage.collect(*this, data);
            cage.spread(*this, data);
	
        }    

        /**
         * @brief Collects from each incoming edge one elements and
         *        reduces them by the binary operator op.
         *
         * @param op   binary operator used for reduction (e.g. std::plus).
         * @param init initial value of the reduction.
         *
         * @hint Each adjacent vertex can send a most one element.
         *
         * @return reduced value
         *
         */
        template <typename T_Op>
        typename T_Op::result_type accumulate(const T_Op op, const typename T_Op::result_type init){
            std::vector<typename T_Op::result_type> data (nInEdges());
            cage.collect(*this, data);
            return std::accumulate(data.begin(), data.end(), init, op);
	
        }
	
    };

} /* namespace graybat */
