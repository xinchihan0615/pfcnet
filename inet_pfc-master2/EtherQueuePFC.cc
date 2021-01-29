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

#include "EtherQueuePFC.h"
#include "EtherMacPFC.h"
#include "ingressTag_m.h"
#include "RouterPFC.h"



Define_Module(EtherQueuePFC);

void EtherQueuePFC::initialize(int stage){
    PacketQueue::initialize(stage);
    is_host_ = par("is_host").boolValue();
    EV_INFO << "is host:"<<is_host_<<"\n";
}

Packet *EtherQueuePFC::popPacket(cGate *gate)
{
    Enter_Method("popPacket");
    auto packet = check_and_cast<Packet *>(queue.front());
    if(!is_host_){
        auto *mac_pfc = getParentModule();
        auto *eth = mac_pfc->getParentModule();
        auto *parent = eth->getParentModule();
        RouterPFC *router = dynamic_cast<RouterPFC *>(parent);
        int ind = eth->getIndex();
        int packetsize = getTotalLength().get();
        EV_INFO << "Releasing memory of " << ind<< ": "<<packetsize <<"mb"<< endl;
        router->RemoveFromIngressAdmission(ind, 0, packetsize);
        router->RemoveFromEgressAdmission(ind, 0, packetsize);
        if (router->CheckShouldResume(ind,0)){
            EtherMacPFC * mac_p = dynamic_cast<EtherMacPFC *>(mac_pfc);
            mac_p->sendPause(PFC_RESUME);
          }
    }
    if (buffer != nullptr) {
        EV_INFO << "Popping packet " << packet->getName() << " from the queue." << endl;
        queue.remove(packet);
        buffer->removePacket(packet);
    }
    else
        queue.pop();
    emit(packetPoppedSignal, packet);
    updateDisplayString();
    animateSend(packet, PacketQueue::outputGate);
    return packet;
}
