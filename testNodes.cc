/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <string>
#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"

// Default Network Topology
//
//       10.1.1.0
// n0 -------------- n1
//    point-to-point
//
 
using namespace ns3;

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

NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");

int
main (int argc, char *argv[])
{
  CommandLine cmd (__FILE__);
  cmd.Parse (argc, argv);
  
  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  NodeContainer connectionA;
  connectionA.Create (2);

  NodeContainer connectionB;
  connectionB.Add (connectionA.Get (1));
  connectionB.Create (1);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devicesA, devicesB;
  devicesA = pointToPoint.Install (connectionA);
  devicesB = pointToPoint.Install (connectionB);

  InternetStackHelper stack;
  stack.Install (connectionA.Get (0));
  stack.Install (connectionB);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfacesA;
  interfacesA = address.Assign (devicesA);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer interfacesB;
  interfacesB = address.Assign (devicesB);

  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (connectionB.Get (1));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (interfacesB.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (connectionA.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  AnimationInterface anim("testNodes.xml");
  anim.SetConstantPosition(connectionA.Get(0), 10.0, 10.0 );
  anim.SetConstantPosition(connectionA.Get(1), 20.0, 20.0 );
  anim.SetConstantPosition(connectionB.Get(1), 30.0, 30.0 );

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
