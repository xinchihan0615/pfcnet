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
import inet.common.misc.ThruputMeteringChannel;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import Pod;

network Clos_test
{
    parameters:
        int numOfTor = default(2);
        int numOfPod = default(2);
    types:
        channel fiberline extends ThruputMeteringChannel
        {
            delay = 1us;
            datarate = 512Mbps;
            thruputDisplayFormat = "u";
        }
        channel ethernetline extends ThruputMeteringChannel
        {
            delay = 0.1us;
            datarate = 10Mbps;
            thruputDisplayFormat = "u";
        }
    submodules:
        configurator: Ipv4NetworkConfigurator {
            parameters:
                @display("p=47,233;is=s");
        }
        pod[numOfPod]: Pod {
            @display("p=222,231,r,240;i=device/mainframe");
            baseNum = numOfTor;
        }
        switch_core[numOfPod*numOfTor]: RouterPFC {
            @display("p=222,61,r,80");
        }
    connections:
        for i=0..numOfPod*numOfTor-1, for j=0..numOfPod-1 {
            switch_core[i].ethg++ <--> ethernetline <--> pod[j].upperInout++;
        }
}
