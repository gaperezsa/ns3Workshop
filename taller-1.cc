/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <string>
#include <iostream>
#include <vector>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
 
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");

std::string intToString(int num){
    std::string retString = "";
    do{
        char digit = '0'+(num%10);
        retString.push_back(digit);
        num /= 10;
    }while( num > 0 );
    reverse(retString.begin(),retString.end());
    return retString;
}

std::string getBaseIP(int clusterId){
    std::string baseAddress = "10.0." + intToString(clusterId) + ".0";
    return baseAddress;
}

int main (int argc, char *argv[])
{
    CommandLine cmd (__FILE__);
    cmd.Parse (argc, argv);
    
    Time::SetResolution (Time::NS);
    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

    // Create clusters and cluster heads

    const int nodesPerCluster = 3;
    const int maxClusters = 3;

    std::vector<NodeContainer> clusters, clusterHeads;
    for(int cluster = 0 ; cluster < maxClusters ; cluster ++){
        NodeContainer currentCluster;
        currentCluster.Create (nodesPerCluster);
        clusters.push_back(currentCluster);

        NodeContainer clusterHead;
        clusterHead.Create (1);
        clusterHeads.push_back(clusterHead);
    }

    // Set up in-cluster connections

    PointToPointHelper pointToPointInCluster;
    pointToPointInCluster.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    pointToPointInCluster.SetChannelAttribute ("Delay", StringValue ("2ms"));

    std::vector <NetDeviceContainer> pairwiseConnectionDevices;

    for(int cluster = 0 ; cluster < maxClusters ; cluster ++){
        for(int node_origin = 0 ; node_origin < (int)clusters[cluster].GetN() ; node_origin ++){
            for(int node_destination = node_origin+1 ; node_destination < (int)clusters[cluster].GetN() ; node_destination ++){
                // Create container with the two nodes that we want to connect pointToPoint 
                NodeContainer currentPairwiseConnection;
                currentPairwiseConnection.Add ( clusters[cluster].Get (node_origin) );
                currentPairwiseConnection.Add ( clusters[cluster].Get (node_destination) );

                // Set up the Net Device for this connection
                NetDeviceContainer currentDevices;
                currentDevices = pointToPointInCluster.Install (currentPairwiseConnection);
                pairwiseConnectionDevices.push_back(currentDevices);
            }
        }
    }

    // Install InternetStackHelper in each node

    InternetStackHelper stack;
    for(int cluster = 0 ; cluster < maxClusters ; cluster ++){
        stack.Install (clusters[cluster]);
    }

    // Set up the Ipv4AddressHelper for each pairwise subnet

    std::vector <Ipv4InterfaceContainer> pairwiseConnectionInterfaces;
    Ipv4AddressHelper address;
    int currentSubnet = 1;
    std::string mask = "255.255.255.0";
    for(int device = 0 ; device < (int)pairwiseConnectionDevices.size() ; device ++){
        std::string baseIP = getBaseIP(currentSubnet);
        address.SetBase (baseIP.c_str(), mask.c_str());
        currentSubnet ++;

        Ipv4InterfaceContainer interface;
        interface = address.Assign (pairwiseConnectionDevices[device]);
        pairwiseConnectionInterfaces.push_back(interface);
    }

    // Set up inter-cluster connections

    PointToPointHelper pointToPointBetweenClusters;
    pointToPointBetweenClusters.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    pointToPointBetweenClusters.SetChannelAttribute ("Delay", StringValue ("2ms"));

    std::vector <NetDeviceContainer> clusterConnectionDevices;

    // Set up the connections between cluster heads
    for(int cluster_origin = 0 ; cluster_origin < maxClusters ; cluster_origin ++){
        for(int cluster_destination = cluster_origin+1 ; cluster_destination < maxClusters ; cluster_destination ++){
            // Create container with the two cluster heads that we want to connect pointToPoint 
            NodeContainer currentConnection;
            currentConnection.Add ( clusterHeads[cluster_origin].Get(0) );
            currentConnection.Add ( clusterHeads[cluster_destination].Get(0) );

            // Set up the Net Device for this connection
            NetDeviceContainer currentDevices;
            currentDevices = pointToPointInCluster.Install (currentConnection);
            clusterConnectionDevices.push_back(currentDevices);
        }
    }

    // Set up the connection of each node to its cluster head
    std::vector < std::vector<NetDeviceContainer> > intoClusterHeadDevices;
    for(int cluster = 0 ; cluster < maxClusters ; cluster ++){
        std::vector<NetDeviceContainer> currentClusterHeadDevices;
        for(int node = 0 ; node < (int)clusters[cluster].GetN() ; node ++){
            // Create container with the node and its cluster head that we want to connect pointToPoint 
            NodeContainer currentConnection;
            currentConnection.Add ( clusters[cluster].Get (node) );
            currentConnection.Add ( clusterHeads[cluster].Get(0) );

            // Set up the Net Device for this connection
            NetDeviceContainer currentDevices;
            currentDevices = pointToPointInCluster.Install (currentConnection);
            currentClusterHeadDevices.push_back(currentDevices);
        }
        intoClusterHeadDevices.push_back(currentClusterHeadDevices);
    }

    // Install InternetStackHelper in each cluster head

    for(int cluster = 0 ; cluster < maxClusters ; cluster ++){
        stack.Install (clusterHeads[cluster]);
    }

    // Set up the Ipv4AddressHelper for each inter-cluster subnet

    std::vector <Ipv4InterfaceContainer> connectionInterfaces;
    for(int device = 0 ; device < (int)clusterConnectionDevices.size() ; device ++){
        std::string baseIP = getBaseIP(currentSubnet);
        address.SetBase (baseIP.c_str(), mask.c_str());
        currentSubnet ++;

        Ipv4InterfaceContainer interface;
        interface = address.Assign (clusterConnectionDevices[device]);
        connectionInterfaces.push_back(interface);
    }

    // Set up and store the Ipv4AddressHelper for each subnet between nodes and their cluster head
    std::vector < std::vector <Ipv4InterfaceContainer> > intoClusterHeadInterfaces;
    for(int cluster = 0 ; cluster < (int)intoClusterHeadDevices.size() ; cluster ++){
        std::vector <Ipv4InterfaceContainer> currentClusterHeadInterfaces;
        for(int device = 0 ; device < (int)intoClusterHeadDevices[cluster].size() ; device ++){
            std::string baseIP = getBaseIP(currentSubnet);
            address.SetBase (baseIP.c_str(), mask.c_str());
            currentSubnet ++;

            Ipv4InterfaceContainer interface;
            interface = address.Assign (intoClusterHeadDevices[cluster][device]);
            currentClusterHeadInterfaces.push_back(interface);
        }
        intoClusterHeadInterfaces.push_back(currentClusterHeadInterfaces);
    }

    UdpEchoServerHelper echoServer (9);

    ApplicationContainer serverApps = echoServer.Install (clusters[0].Get (0));
    serverApps.Start (Seconds (1.0));
    serverApps.Stop (Seconds (10.0));

    UdpEchoClientHelper echoClient (intoClusterHeadInterfaces[0][0].GetAddress (0), 9);
    echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
    echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer clientApps = echoClient.Install (clusters[2].Get (0));
    clientApps.Start (Seconds (2.0));
    clientApps.Stop (Seconds (10.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    AnimationInterface anim("testCluster.xml");
    for(int cluster = 0 ; cluster < maxClusters ; cluster ++){
        anim.SetConstantPosition(clusterHeads[cluster].Get(0), 10.0+cluster*30.0, (cluster == 1) ? 5.0 : 10.0 );
    }

    for(int cluster = 0 ; cluster < maxClusters ; cluster ++){
        for(int node = 0 ; node < (int)clusters[cluster].GetN() ; node ++){
            anim.SetConstantPosition(clusters[cluster].Get(node), 10.0 + (double)cluster*30.0 + 4.0*(double)node, 20.0 );
        }
    }

    Simulator::Run ();
    Simulator::Destroy ();
    return 0;
}
