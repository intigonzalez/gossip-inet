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

package inet.applications.gossip;

import inet.applications.contract.IUDPApp;

//
// This module is an application that implement the Gossip protocol
// to disemminate messages.
//
// Some nodes distribute messages in the network using unicast UDP.
//
simple GossipPush like IUDPApp
{
    parameters:
        
        int destinationPort = default(10000);
        int localPort = default(10000);
        
        bool isSource = default(false); // indicates whether the app is the source of messages
        int numMessages = default(1); // how many messages to send
        double intervalAmongNewMessages @unit(s) = default(5s); // how much time to wait between two different new messages created in this node
        
        // gossip stuff
        int nodesPerRound = default(1); // this node will gossip at most with 'nodesPerRound' in each round
        int roundRatio = default(2); // the number of rounds is 'roundRatio*numberOfAddresses'
        
        string addresses = default(""); // network members
          
	gates:
        input udpIn @labels(UDPControlInfo/up);
        output udpOut @labels(UDPControlInfo/down);
}
