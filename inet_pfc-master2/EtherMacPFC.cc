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

#include "EtherMacPFC.h"
#include "inet/linklayer/common/Ieee802Ctrl.h"
#include "inet/common/packet/Message_m.h"
#include "inet/common/packet/Message.h"
#include "inet/common/packet/Message.cc"
#include "inet/common/packet/printer/PacketPrinter.h"
#include "inet/linklayer/ethernet/EtherPhyFrame_m.h"
#include "inet/linklayer/ethernet/EtherMacBase.h"
#include "ingressTag_m.h"
#include "RouterPFC.h"

#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/linklayer/common/Ieee802Ctrl.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/ethernet/EtherEncap.h"
#include "inet/linklayer/ethernet/EtherFrame_m.h"
#include "inet/linklayer/ethernet/EtherMac.h"
#include "inet/linklayer/ethernet/EtherPhyFrame_m.h"
#include "inet/linklayer/ethernet/Ethernet.h"
#include "inet/networklayer/common/InterfaceEntry.h"


Define_Module(EtherMacPFC);


void EtherMacPFC::initialize(int stage)
{
    EtherMac::initialize(stage);
    queue_threshold = par("data_thres");
    pause_time = par("pause_time");
    EV_INFO << "Queue Threshold: " << queue_threshold << "\n";
    pause = false;
}

MacAddress EtherMacPFC::findUpstream() {
    cGate *ingate = gate("phys$i");
    cGate *prev_gate = ingate->getPreviousGate()->getPreviousGate();
    EtherMacBase *prev_module;
    MacAddress empty;

    while (prev_gate) {
        prev_module = dynamic_cast<EtherMacBase *>(prev_gate->getOwnerModule());
        if (prev_module) return prev_module->getMacAddress();
        else prev_gate = prev_gate->getPreviousGate();
    }
    return empty;
}

void EtherMacPFC::sendPause(PFC_Control Opcode) {
    Enter_Method("sendPause(%d)",Opcode);

    MacAddress upstream = findUpstream();
    Ieee802PauseCommand *ctrl = new Ieee802PauseCommand();
    ctrl -> setDestinationAddress(upstream);

    std::cout << getMacAddress() << "\t" << upstream << std::endl;
    auto msg = new Request();

    if (Opcode == PFC_PAUSE && !pause) {
        ctrl -> setPauseUnits(pause_time);
        EV_INFO << "!!!Sending Pause Frame \n";
        pause = true;
    }

    else {
        if (Opcode == PFC_RESUME && pause) {
            ctrl -> setPauseUnits(0);
            EV_INFO << "!!!Sending Resume Frame \n";
            pause = false;
        }
        // The rest cases that no operation should be done
    else {
            EV_INFO << "!!!Fuck \n";
            EV_INFO <<Opcode<< "!!!Fuck \n";
            return;
        }
    }

    msg -> setControlInfo(ctrl);
    send(msg, "controlmsgOut");
}

void EtherMacPFC::handleMessageWhenUp(cMessage *msg)
{
    auto *eth = getParentModule();


    if (channelsDiffer)
        readChannelParameters(true);

    printState();
    ingressTag *ingress;


    // some consistency check
    if (!duplexMode && transmitState == TRANSMITTING_STATE && receiveState != RX_IDLE_STATE)
        throw cRuntimeError("Inconsistent state -- transmitting and receiving at the same time");

    EV_INFO << "^^^Packet Arriving in " << msg->getArrivalGateId() <<" UpperLayer: " << upperLayerInGateId \
            << " PhysicalIn: " << physInGate << "\n";
    if (msg->isSelfMessage())
    {
        // 如果收到消息完成的信号,就释放入端口和出端口的内存
        if(msg->getKind() == ENDTRANSMISSION)
        {
            auto *eth = getParentModule();
            auto *parent = eth->getParentModule();
            RouterPFC *router = dynamic_cast<RouterPFC *>(parent);
            uint32_t msg_id = msg->getId();
            std::pair<uint32_t, uint32_t> ind_packetSize_map;
            ind_packetSize_map = msg_map[msg_id];
            uint32_t ind = ind_packetSize_map.first;
            uint32_t packetsize = ind_packetSize_map.second;
            router->RemoveFromIngressAdmission(ind, 0, packetsize);
            router->RemoveFromIngressAdmission(ind, 0, packetsize);
            msg_map.erase(msg_id);
        }
        handleSelfMessage(msg);
    }
    else if (msg->getArrivalGateId() == upperLayerInGateId) {
        handleUpperPacket(check_and_cast<Packet *>(msg));
    }
    else if (msg->getArrivalGate() == physInGate) {
        int ind = eth->getIndex();
        auto signal = check_and_cast<EthernetSignal *>(msg);
        auto packet = check_and_cast<Packet *>(signal->decapsulate());
        ingress = packet->addTag<ingressTag>();
        ingress->setIngressPort(ind);
        signal->encapsulate(packet);

        if (auto jamSignal = dynamic_cast<EthernetJamSignal *>(msg))
            processJamSignalFromNetwork(jamSignal);
        else
            processMsgFromNetwork(signal);
    }
    else
        throw cRuntimeError("Message received from unknown gate");

    processAtHandleMessageFinished();
    printState();
}


void EtherMacPFC::handleUpperPacket(Packet *packet)
{
    auto *eth = getParentModule();
    auto *parent = eth->getParentModule();
    RouterPFC *router = dynamic_cast<RouterPFC *>(parent);

    EV_INFO << "Received " << packet << " from upper layer." << endl;

    if (!packet->findTag<ingressTag>())
    {
        return EtherMac::handleUpperPacket(packet);
    }

    auto ingress = packet->getTag<ingressTag>();
    int ind = ingress->getIngressPort();

    numFramesFromHL++;
    emit(packetReceivedFromUpperSignal, packet);

    MacAddress address = getMacAddress();

    auto frame = packet->peekAtFront<EthernetMacHeader>();
    if (frame->getDest().equals(address)) {
        throw cRuntimeError("Logic error: frame %s from higher layer has local MAC address as dest (%s)",
                packet->getFullName(), frame->getDest().str().c_str());
    }

    if (packet->getDataLength() > MAX_ETHERNET_FRAME_BYTES) {
        throw cRuntimeError("Packet length from higher layer (%s) exceeds maximum Ethernet frame size (%s)",
                packet->getDataLength().str().c_str(), MAX_ETHERNET_FRAME_BYTES.str().c_str());
    }

    if (!connected || disabled) {
        EV_WARN << (!connected ? "Interface is not connected" : "MAC is disabled") << " -- dropping packet " << frame << endl;
        PacketDropDetails details;
        details.setReason(INTERFACE_DOWN);
        emit(packetDroppedSignal, packet, &details);
        numDroppedPkFromHLIfaceDown++;
        delete packet;
        EV_DETAIL << "!!!FUCK\n";
        return;
    }

    // fill in src address if not set
    if (frame->getSrc().isUnspecified()) {
        //frame is immutable
        frame = nullptr;
        auto newFrame = packet->removeAtFront<EthernetMacHeader>();
        newFrame->setSrc(address);
        packet->insertAtFront(newFrame);
        frame = newFrame;
    }

    addPaddingAndSetFcs(packet, MIN_ETHERNET_FRAME_BYTES);  // calculate valid FCS

    // store frame and possibly begin transmitting
    EV_DETAIL << "Frame " << packet << " arrived from higher layer, enqueueing\n";

    txQueue->pushPacket(packet);

    std::cout << router->getFullName() << std::endl;
    auto *inEth = router->getSubmodule("eth", ind);
    auto *inEthMac = dynamic_cast<EtherMacPFC *>(inEth->getSubmodule("mac"));
    int packetsize = packet->getTotalLength().get();
    //router->updateCounter(ind, packetsize);
    //EV_INFO << "!!!ind: " << ind << "\n";
    // 统计发送的包的总数
    router->total_packet_count++;
    router->UpdateIngressAdmission(ind, 0, packetsize);
    router->UpdateEgressAdmission(ind, 0, packetsize);
    // 判断是否发送pause
    //EV_INFO << "!!!PFC_PAUSE"<<router->CheckShouldPause(ind,0)<<"\n";
    if (router->CheckShouldPause(ind,0)){
        EV_INFO << "***PFC_PAUSE"<<"\n";
        router->SetPause(ind,0);
        inEthMac->sendPause(PFC_PAUSE);
    }
    //if (router->getCounter(ind) >= queue_threshold) inEthMac->sendPause(PFC_PAUSE);
    if (!currentTxFrame && !txQueue->isEmpty())
    {
        currentTxFrame = txQueue->popPacket();
        packetsize = currentTxFrame->getTotalLength().get();
        EV_DETAIL << "Frame" << packet << " With Length" << packetsize << " bits\n";
        //router->updateCounter(ind, -packetsize);
        //if (router->getCounter(ind) < queue_threshold) inEthMac->sendPause(PFC_RESUME);
        // 判断是否resume
        if (router->CheckShouldResume(ind,0)){
            router->SetResume(ind,0);
            inEthMac->sendPause(PFC_RESUME);
            EV_INFO << "***PFC_RESUME"<<"\n";
        }
    }
    // TODO 设计ECN拥塞控制
    for (int i=0; i < router->pCnt; ++i)
    {
        if(router->ShouldSendCN(i,0)) router->isJam[i] = 0;
    }


    if ((duplexMode || receiveState == RX_IDLE_STATE) && transmitState == TX_IDLE_STATE) {
        EV_DETAIL << "No incoming carrier signals detected, frame clear to send\n";
        startFrameTransmission();
        // 用map记录收到端口与包的映射关系,便于后续释放内存
        msg_map.insert(std::make_pair(static_cast<uint32_t>(packet->getId()), std::make_pair(static_cast<uint32_t>(ind),static_cast<uint32_t>(packetsize))));
    }
}
