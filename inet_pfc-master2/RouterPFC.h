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

#ifndef ROUTERPFC_H_
#define ROUTERPFC_H_

#include <omnetpp.h>
using namespace omnetpp;

class RouterPFC : public cSimpleModule {
protected:
    int *bytecounter;
    int portcount;
    virtual void initialize();
public:
    static const uint32_t pCnt = 5;   // 受限于目前的架构,初期版本交换机上各只有一个端口与对应服务器连接
    static const uint32_t qCnt = 1; // 每个端口只有一个用于tcp的队列
    int total_packet_count = 0;
    int drop_packet_count = 0;
    int pfc_shift_default = 3;
    int headroom_default = 0;
    void updateCounter(int ind, int64_t x);
    int64_t getCounter(int ind);
    bool CheckIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize);   //检查接受端口
    bool CheckEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize);    //检查发送端口
    void UpdateIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize);  //如果可以接受,那么更新相关参数
    void UpdateEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize);   //如果可以发送,那么更新相关参数
    void RemoveFromIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize);
    void RemoveFromEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize);

    bool CheckShouldPause(uint32_t port, uint32_t qIndex);
    bool CheckShouldResume(uint32_t port, uint32_t qIndex);
    void SetPause(uint32_t port, uint32_t qIndex);
    void SetResume(uint32_t port, uint32_t qIndex);

    uint32_t GetPfcThreshold(uint32_t port);
    uint32_t GetSharedUsed(uint32_t port, uint32_t qIndex);

    bool ShouldSendCN(uint32_t ifindex, uint32_t qIndex);   //检查出端口是否拥塞

    // 配置参数
    void ConfigEcn(uint32_t port, uint32_t _kmin, uint32_t _kmax, double _pmax);
    void ConfigHdrm(uint32_t port, uint32_t size);
    void ConfigNPort(uint32_t n_port);
    void ConfigBufferSize(uint32_t size);

    // config
    uint32_t node_id;            //节点的id号
    uint32_t buffer_size;        //节点的buffer大小
    uint32_t pfc_a_shift[pCnt];  //右位移参数,用于设定pfc的阈值,默认值是3,即除以8
    uint32_t reserve;            //用于转发的缓存,是一个预设的固定值
    uint32_t headroom[pCnt];     //用于存储来自上游节点信息的缓存,优先级高于sharedbuffer,是一个预设的固定值
    uint32_t resume_offset;      //一个用于重启的阈值,当shared_used + resume_offset < pfc_threadsold时重启
    uint32_t kmin[pCnt], kmax[pCnt]; //用于ECN拥塞控制的参数
    double pmax[pCnt];
    uint32_t total_hdrm;         //headroom之和
    uint32_t total_rsrv;         //reserve之和
    uint32_t isJam[pCnt];        //某端口是否拥塞

    // runtime
    uint32_t shared_used_bytes;
    uint32_t hdrm_bytes[pCnt][qCnt];
    uint32_t ingress_bytes[pCnt][qCnt];
    uint32_t paused[pCnt][qCnt];
    uint32_t egress_bytes[pCnt][qCnt];
};

#endif /* ROUTERPFC_H_ */
