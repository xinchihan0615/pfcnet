#include "EtherEncapPFC.h"

Define_Module(EtherEncapPFC);

void EtherEncapPFC::handleMessageWhenUp(cMessage *msg) {
    if (msg->arrivedOn("upperLayerIn")|| msg->arrivedOn("controlmsgIn")) {
        // from upper layer
        EV_INFO << "!!!Fuck Encap" << "\n";
        EV_INFO << "Received " << msg << " from upper layer." << endl;
        if (msg->isPacket())
            processPacketFromHigherLayer(check_and_cast<Packet *>(msg));
        else
            processCommandFromHigherLayer(check_and_cast<Request *>(msg));
    }
    else if (msg->arrivedOn("lowerLayerIn") ) {
        EV_INFO << "Received " << msg << " from lower layer." << endl;
        processPacketFromMac(check_and_cast<Packet *>(msg));
    }
    else
        throw cRuntimeError("Unknown gate");
}
