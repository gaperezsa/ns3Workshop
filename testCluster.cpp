/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
 
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");

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

    vector<NodeContainer> clusters, clusterHeads;
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
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

    vector <NetDeviceContainer> pairwiseConnections;

    for(int cluster = 0 ; cluster < maxClusters ; cluster ++){
        for(int node_origin = 0 ; node_origin < clusters[cluster].size() ; node_origin ++){
            for(int node_destination = node_origin+1 ; node_destination < clusters[cluster].size() ; node_destination ++){
                // Create container with the two nodes that we want to connect pointToPoint 
                NodeContainer currentPairwiseConnection;
                currentPairwiseConnection.Add ( clusters[cluster].Get (node_origin) );
                currentPairwiseConnection.Add ( clusters[cluster].Get (node_destination) );

                // Set up the Net Device for this connection
                NetDeviceContainer currentDevices;
                currentDevices = pointToPointInCluster.Install (currentPairwiseConnection);
            }
        }
    }


    InternetStackHelper stack;
    stack.Install (poolA.Get (0));
    stack.Install (poolB);

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfacesA;
    interfacesA = address.Assign (devicesA);

    address.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer interfacesB;
    interfacesB = address.Assign (devicesB);

    UdpEchoServerHelper echoServer (9);

    ApplicationContainer serverApps = echoServer.Install (poolB.Get (1));
    serverApps.Start (Seconds (1.0));
    serverApps.Stop (Seconds (10.0));

    UdpEchoClientHelper echoClient (interfacesB.GetAddress (1), 9);
    echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
    echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer clientApps = echoClient.Install (poolA.Get (0));
    clientApps.Start (Seconds (2.0));
    clientApps.Stop (Seconds (10.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    Simulator::Run ();
    Simulator::Destroy ();
    return 0;
}
