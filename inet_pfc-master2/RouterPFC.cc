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


#include "RouterPFC.h"


Define_Module(RouterPFC);

void RouterPFC::initialize() {
    portcount = par("portcount");
    //EV_INFO << "!!!portcount: " << portcount << "\n";
    //EV_INFO << "!!!pCnt: " << pCnt << "\n";
    bytecounter = new int[portcount];
    for (int i = 0; i< portcount; ++i) {
        bytecounter[i] = 0;
    }
    buffer_size = 12*1024;
    reserve = 2*1024;
    headroom_default = 0;
    resume_offset = 1024;
    pfc_shift_default = 3;

    // 分配内存
    shared_used_bytes = 0;
    memset(hdrm_bytes, 0, sizeof(hdrm_bytes));
    memset(ingress_bytes, 0, sizeof(ingress_bytes));
    //memset(paused, 0, sizeof(paused));
    memset(egress_bytes, 0, sizeof(egress_bytes));
    memset(isJam, 0, sizeof(isJam));
    for(int i = 0; i < pCnt; i++){
        pfc_a_shift[i] = pfc_shift_default;
    }
    // TODO config ECN
    for (uint32_t i = 1; i < pCnt; i++)
        ConfigHdrm(i,headroom_default);
    for (uint32_t i = 1; i < pCnt; i++)
        ConfigNPort(i);
}

void RouterPFC::updateCounter(int ind, int64_t x) {
    if (ind < 0 || ind >= portcount) {
        throw cRuntimeError("Index exceeds router's size");
    }
    int new_counter = bytecounter[ind] + x;
    if (new_counter < 0) {
        throw cRuntimeError("Byte counter value becomes less than 0");
    }
    bytecounter[ind] = new_counter;
}

int64_t RouterPFC::getCounter(int ind) {
    if (ind < 0 || ind >= portcount) {
        throw cRuntimeError("Index exceeds router's size");
    }
    return bytecounter[ind];
}


// 在入端口出检查是否可以接受该message,具体步骤:
// 如果headroom有足够大的内存空间,并且shared_memory+消息大小psize没有达到PFC阈值,那么可以接收
bool RouterPFC::CheckIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize){
        if (psize + hdrm_bytes[port][qIndex] > headroom[port] && psize + GetSharedUsed(port, qIndex) > GetPfcThreshold(port)){
            printf("%u Drop: queue:%u,%u: Headroom full\n", node_id, port, qIndex); //TODO use inet API to show system time
            for (uint32_t i = 1; i < pCnt; i++)
                printf("(%u,%u)", hdrm_bytes[i][0], ingress_bytes[i][0]);
            printf("\n");
            return false;
        }
        return true;
    }

// 丢包
bool RouterPFC::CheckEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize){
        return true;
    }

// 若能够接收,则更新相关参数,具体步骤:
// Step1 如果接受端口有足够的reserve空间,那么直接把消息放入该队列,相当于占用reserve_buffer的空间
// Step2 否则,如果超出的大小仍然小于PFC阈值,那么占用headroom空间
// Step3 否则,尽量占用reserve的空间,如果reserve的空间占满,那么占用shared_used_bytes的空间
void RouterPFC::UpdateIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize){
    uint32_t new_bytes = ingress_bytes[port][qIndex] + psize;
//    EV_INFO << "!!!new_bytes \n"<<new_bytes<<"\n";
//    EV_INFO << "!!!buffer size \n"<<buffer_size<<"\n";
    if (new_bytes <= reserve){
        ingress_bytes[port][qIndex] += psize;
    }else {
        uint32_t thresh = GetPfcThreshold(port);
        if (new_bytes - reserve > thresh){
            hdrm_bytes[port][qIndex] += psize; //?
        }else {
            ingress_bytes[port][qIndex] += psize;
            shared_used_bytes += std::min(psize, new_bytes - reserve);
        }
    }
}

// 若能够发送,则更新相关参数,具体步骤:
// 更新出端口队列
void RouterPFC::UpdateEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize){
        egress_bytes[port][qIndex] += psize;
    }

// 若成功发送,则更新输入端口相关参数,具体步骤:
// 计算占用headroom部分的内存大小
// 计算占用shared_memory部分的内存大小
// 更新相关参数
void RouterPFC::RemoveFromIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize){
        int32_t from_hdrm = std::min(hdrm_bytes[port][qIndex], psize);
        int32_t from_shared = std::min(psize - from_hdrm, ingress_bytes[port][qIndex] > reserve ? ingress_bytes[port][qIndex] - reserve : 0);
        EV_INFO << "!!!ingress_bytes: "<<ingress_bytes[port][qIndex]<<" ingress_bytes: "<<(psize - from_hdrm)<<"\n";
        EV_INFO << "!!!shared_used_bytes: "<<shared_used_bytes<<" from_shared: "<<from_shared<<"\n";
        EV_INFO << "Remove from hdrm: "<<from_hdrm<<"\n";
        EV_INFO << "Remove from ingress_bytes: "<<(psize - from_hdrm)<<"\n";
        EV_INFO << "Remove from shared_used_bytes: "<<from_shared<<"\n";
        if (hdrm_bytes[port][qIndex]>from_hdrm) hdrm_bytes[port][qIndex] -= from_hdrm;
        if (ingress_bytes[port][qIndex]>(psize - from_hdrm)) ingress_bytes[port][qIndex] -= (psize - from_hdrm);
        if (shared_used_bytes > from_shared) shared_used_bytes -= from_shared;
    }

// 若成功发送,则更新输出端口相关参数
void RouterPFC::RemoveFromEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize){
        egress_bytes[port][qIndex] -= psize;
    }

// 检测是否发送pause,判断逻辑为:
// 1.该队列是否没有被暂停 2.该队列的headroom是否大于0或者sharedmomory是否达到该端口的阈值
bool RouterPFC::CheckShouldPause(uint32_t port, uint32_t qIndex){
        EV_INFO << "!!!total_hdrm :"<<total_hdrm<<"\n";
        EV_INFO << "!!!total_rsrv :"<<total_rsrv<<"\n";
        EV_INFO << "!!!shared_used_bytes :"<<shared_used_bytes<<"\n";
        EV_INFO << "!!!buffer size :"<<buffer_size<<"\n";
        EV_INFO << "!!!ingress_bytes[port][qIndex] :"<<ingress_bytes[port][qIndex]<<"\n";
        EV_INFO << "!!!hdrm_bytes[port][qIndex] :"<<hdrm_bytes[port][qIndex]<<"\n";
        EV_INFO << "!!!GetSharedUsed(port, qIndex) :"<<GetSharedUsed(port, qIndex)<<"\n";
        EV_INFO << "!!!GetPfcThreshold(port) :"<<GetPfcThreshold(port)<<"\n";
        //return !paused[port][qIndex] && (hdrm_bytes[port][qIndex] > 0 || GetSharedUsed(port, qIndex) >= GetPfcThreshold(port));
        return (hdrm_bytes[port][qIndex] > 0 ||GetSharedUsed(port, qIndex) >= GetPfcThreshold(port));
    }

// 检测是否Resume,判断逻辑为:
// 1.该队列是否已经被暂停 2.该队列的headroom是否等于0并且sharedmomory+resume_offset是否小于该端口的阈值
bool RouterPFC::CheckShouldResume(uint32_t port, uint32_t qIndex){
        uint32_t shared_used = GetSharedUsed(port, qIndex);
        return 1;
        if(hdrm_bytes[port][qIndex] == 0 || (shared_used == 0 || ingress_bytes[port][qIndex]  <= GetPfcThreshold(port))) EV_INFO << "!!!FuckF"<<"\n";
        return hdrm_bytes[port][qIndex] == 0 || (shared_used == 0 || ingress_bytes[port][qIndex]  <= GetPfcThreshold(port));
    }

//// 暂停某个端口
//void RouterPFC::SetPause(uint32_t port, uint32_t qIndex){
//        paused[port][qIndex] = true;
//    }
//
//// 重启某个端口
//void RouterPFC::SetResume(uint32_t port, uint32_t qIndex){
//        paused[port][qIndex] = false;
//    }

// 设定PFC阈值为 buffer大小 - headroom总空间 - reserve总空间 - shared_used空间 的 八分之一
uint32_t RouterPFC::GetPfcThreshold(uint32_t port){
        return (buffer_size - total_hdrm - total_rsrv - shared_used_bytes) >> pfc_a_shift[port];
    }

// 计算shared_used,看起来每个端口的队列的sharedused彼此独立,其实在计算PFC阈值的时候是加在一起算的,
// 可以认为所有端口共享交换机中的一块shared memory区域
uint32_t RouterPFC::GetSharedUsed(uint32_t port, uint32_t qIndex){
        uint32_t used = ingress_bytes[port][qIndex];
        return used > reserve ? used - reserve : 0;
    }

// 检查出端口是否拥塞
bool RouterPFC::ShouldSendCN(uint32_t ifindex, uint32_t qIndex){
        if (qIndex == 0)
            return false;
        if (egress_bytes[ifindex][qIndex] > kmax[ifindex])
            return true;
        if (egress_bytes[ifindex][qIndex] > kmin[ifindex]){
            double p = pmax[ifindex] * double(egress_bytes[ifindex][qIndex] - kmin[ifindex]) / (kmax[ifindex] - kmin[ifindex]);
            if (0.5 < p)  // 调用api
                return true;
        }
        return false;
    }

void RouterPFC::ConfigEcn(uint32_t port, uint32_t _kmin, uint32_t _kmax, double _pmax){
        kmin[port] = _kmin * 1000;
        kmax[port] = _kmax * 1000;
        pmax[port] = _pmax;
    }

void RouterPFC::ConfigHdrm(uint32_t port, uint32_t size){
        headroom[port] = size;
    }

void RouterPFC::ConfigNPort(uint32_t n_port){
        total_hdrm = 0;
        total_rsrv = 0;
        for (uint32_t i = 1; i <= n_port; i++){
            total_hdrm += headroom[i];
            total_rsrv += reserve;
        }
    }

void RouterPFC::ConfigBufferSize(uint32_t size){
        buffer_size = size;
    }


