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

import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import inet.common.misc.ThruputMeteringChannel;

network Pod
{
    parameters:
        int baseNum = default(2);
    gates:
        inout upperInout[];
    types:
        channel ethernetline extends ThruputMeteringChannel
        {
            delay = 0.1us;
            datarate = 10Mbps;
            thruputDisplayFormat = "u";
        }

    submodules:
        host[baseNum]: StandardHost {
            @display("i=device/server;p=86,387,r");
        }
        switch_tor[baseNum]: RouterPFC {
            @display("p=86,260,r");
        }
        switch_agg[baseNum]: RouterPFC {

            @display("p=86,144,r");
        }
    connections:
        for i=0..baseNum-1, for j=0..baseNum-1 {
            switch_tor[i].ethg++ <--> ethernetline <--> switch_agg[j].ethg++;
        }
        for i=0..baseNum-1 {
            host[i].ethg++ <--> ethernetline <--> switch_tor[i].ethg++;
        }
        for i=0..sizeof(upperInout)-1 {
            switch_agg[i%baseNum].ethg++ <--> upperInout[i];
        }
}
