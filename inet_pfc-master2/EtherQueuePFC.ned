//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

import inet.linklayer.ethernet.EtherQosQueue;
import inet.queueing.queue.PacketQueue;
import inet.queueing.contract.IPacketClassifier;
import inet.queueing.contract.IPacketQueue;
import inet.queueing.contract.IPacketScheduler;
import inet.queueing.filter.RedDropper;
import inet.queueing.queue.CompoundPacketQueue;
import inet.queueing.queue.DropTailQueue;

module EtherQueuePFC extends PacketQueue like IPacketQueue
{
    parameters:
		@class(EtherQueuePFC);
		bool is_host = default(false);
		packetCapacity = default(1000);
        comparatorClass = default("inet::EthernetFrameTypeComparator");
        dropperClass = default("inet::queueing::PacketAtCollectionEndDropper");
}
