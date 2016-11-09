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

#include <vector>

namespace graybat {
    
    namespace communicationPolicy {

        namespace traits {

            template <typename T_CommunicationPolicy>
            struct ContextType;

            template <typename T_CommunicationPolicy>
            struct ContextIDType;

            template <typename T_CommunicationPolicy>
            struct EventType;

            template <typename T_CommunicationPolicy>
            struct ConfigType;

			template <typename T_RecvType>
			void unfoldMessage(std::vector<T_RecvType> recvData, std::int8_t* messageData, size_t messageSize) {
				//This is the specialisation for std::vector. Vector is a dynamic data structure and the
				//size should be according to the received message. If the vector has the right size in the
				//beginning the runtimeoverhead is negectable.
				recvData.resize(messageSize / sizeof(T_RecvType));
				memcpy (static_cast<void*>(recvData.data()),
                        messageData,
                        sizeof(T_RecvType) * recvData.size());
			}
			
			template <typename T_RecvType>
			void unfoldMessage(T_RecvType recvData, std::int8_t* messageData, size_t messageSize) {
				//This is the standard behaviour in the old api
				//I think it is not smart, but that is another discussion
				memcpy (static_cast<void*>(recvData.data()),
                        messageData,
                        recvData.size());
			}
        } // namespace traits

        template <typename T_CommunicationPolicy>        
        using VAddr = unsigned;

        template <typename T_CommunicationPolicy>
        using Tag = unsigned;


        enum class MsgTypeType : std::int8_t { VADDR_REQUEST = 0,
                VADDR_LOOKUP = 1,
                DESTRUCT = 2,
                RETRY = 3,
                ACK = 4,
                CONTEXT_INIT = 5,
                CONTEXT_REQUEST = 6,
                PEER = 7,
                CONFIRM = 8,
                SPLIT = 9};
        
        template <typename T_CommunicationPolicy>
        using MsgType = MsgTypeType;

        template <typename T_CommunicationPolicy>        
        using MsgID = unsigned;

        template <typename T_CommunicationPolicy>
        using Context = typename traits::ContextType<T_CommunicationPolicy>::type;

        template <typename T_CommunicationPolicy>
        using ContextID = typename traits::ContextIDType<T_CommunicationPolicy>::type;


        template <typename T_CommunicationPolicy>
        using Event = typename traits::EventType<T_CommunicationPolicy>::type;

        template <typename T_CommunicationPolicy>
        using Config = typename traits::ConfigType<T_CommunicationPolicy>::type;
        
    } // namespace communicationPolicy
    
} // namespace graybat
