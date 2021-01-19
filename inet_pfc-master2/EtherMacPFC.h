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

#ifndef _ETHERMACPFC_H_
#define _ETHERMACPFC_H_

#include "inet/linklayer/ethernet/EtherMac.h"
using namespace omnetpp;
using namespace inet;

enum PFC_Control {
    PFC_PAUSE = 1,
    PFC_RESUME = 0
};

class EtherMacPFC : public EtherMac
{
protected:
    EtherMac *upstream = nullptr;
    int64_t queue_threshold;
    int pause_time = 0;
    bool pause = false;

protected:
    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(cMessage *msg) override;
    virtual void handleUpperPacket(Packet *packet) override;
    virtual void sendPause(PFC_Control Opcode);
    bool isPaused() { return pause; }
    MacAddress findUpstream();
};
#endif
