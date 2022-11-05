/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 University of Kansas
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Justin Rohrer <rohrej@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  http://wiki.ittc.ku.edu/resilinets
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 */

/*
 * This example program allows one to run ns-3 DSDV, AODV, or OLSR under
 * a typical random waypoint mobility model.
 *
 * By default, the simulation runs for 200 simulated seconds, of which
 * the first 50 are used for start-up time.  The number of nodes is 50.
 * Nodes move according to RandomWaypointMobilityModel with a speed of
 * 20 m/s and no pause time within a 300x1500 m region.  The WiFi is
 * in ad hoc mode with a 2 Mb/s rate (802.11b) and a Friis loss model.
 * The transmit power is set to 7.5 dBm.
 *
 * It is possible to change the mobility and density of the network by
 * directly modifying the speed and the number of nodes.  It is also
 * possible to change the characteristics of the network by changing
 * the transmit power (as power increases, the impact of mobility
 * decreases and the effective density increases).
 *
 * By default, OLSR is used, but specifying a value of 2 for the protocol
 * will cause AODV to be used, and specifying a value of 3 will cause
 * DSDV to be used.
 *
 * By default, there are 10 source/sink data pairs sending UDP data
 * at an application rate of 2.048 Kb/s each.    This is typically done
 * at a rate of 4 64-byte packets per second.  Application data is
 * started at a random time between 50 and 51 seconds and continues
 * to the end of the simulation.
 *
 * The program outputs a few items:
 * - packet receptions are notified to stdout such as:
 *   <timestamp> <node-id> received one packet from <src-address>
 * - each second, the data reception statistics are tabulated and output
 *   to a comma-separated value (csv) file
 * - some tracing and flow monitor configuration that used to work is
 *   left commented inline in the program
 */

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/mobility-module.h"
#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/applications-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/core-module.h"
#include "ns3/netanim-module.h"
#include "ns3/opengym-module.h"
#include "ns3/flow-monitor-module.h"
#include <cstdio>


using namespace ns3;
using namespace dsr;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("manet-routing-compare");

uint32_t global_PacketsReceived;
uint32_t global_PacketsSent;
multimap<uint32_t, Time> SendingTimes;
multimap<uint32_t, Time> ReceivingTimes;
float distance_change = 1.5; 
vector<double> send_times = { 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 10.0, 11.0,
                              11.0, 12.0, 12.0, 13.0, 13.0, 14.0, 14.0,
                              15.0, 15.0, 16.0, 16.0, 17.0, 17.0, 18.0,
                              18.0, 19.0, 19.0, 20.0, 20.0, 21.0, 22.0,
                              23.0, 24.0, 25.0};
uint32_t ptr_send_times [3];
vector <double> latency_values;

class RoutingExperiment
{
public:
  RoutingExperiment ();
  void Run (int nSinks, double txp, std::string CSVfileName);
  std::string CommandSetup (int argc, char **argv);
  


private:
  Ptr<Socket> SetupPacketReceive (Ipv4Address addr, Ptr<Node> node, uint32_t checkPort);
  Ptr<Socket> SetupPacketSend(Ipv4Address addr, Ptr<Node> node, uint32_t checkPort);
  void ReceivePacket (Ptr<Socket> socket);
  void SendPacket (Ptr<Socket> socket, uint32_t bytes);
  void CheckThroughput ();
  

  uint32_t port;
  uint32_t bytesTotal;
  uint32_t packetsReceived;

  std::string m_CSVfileName;
  int m_nSinks;
  std::string m_protocolName;
  double m_txp;
  bool m_traceMobility;
  uint32_t m_protocol;
};

Ptr<OpenGymSpace> MyGetObservationSpace(void)
{
  uint32_t nodeNum = 3;
  float low = 0.0;
  float high = 100.0;
  std::vector<uint32_t> shape = {nodeNum,};
  std::string dtype = TypeNameGet<uint32_t> ();
  Ptr<OpenGymBoxSpace> space = CreateObject<OpenGymBoxSpace> (low, high, shape, dtype);
  NS_LOG_UNCOND ("MyGetObservationSpace: " << space);
  return space;
}




Ptr<OpenGymSpace> MyGetActionSpace(void)
{
  uint32_t nodeNum = 3;
  Ptr<OpenGymDiscreteSpace> space = CreateObject<OpenGymDiscreteSpace> (nodeNum);
  NS_LOG_UNCOND ("MyGetActionSpace: " << space);
  return space;
}


bool MyGetGameOver(void)
{

  bool isGameOver = false;
  static float stepCounter = 0.0;
  stepCounter += 1;
  global_PacketsSent = stepCounter*4;
  if ( Simulator::Now().Compare(Time("29s")) >= 0) {
      isGameOver = true;
  }
  NS_LOG_UNCOND ("MyGetGameOver: " << isGameOver);
  return isGameOver;
}


Ptr<OpenGymDataContainer> MyGetObservation(void)
{
  //Define the base observation space
  uint32_t nodeNum = 3;

  std::vector<uint32_t> shape = {nodeNum,};
  Ptr<OpenGymBoxContainer<uint32_t> > box = CreateObject<OpenGymBoxContainer<uint32_t> >(shape);
  
    for (uint32_t i=0; i<nodeNum; i++)
    {
      Ptr<Node> node = NodeList::GetNode(i);

        //Extract the position from the hierarchy 2 nodes
      Ptr<MobilityModel> cpMob = node->GetObject<MobilityModel>();
      Vector m_position = cpMob->GetPosition();
      box->AddValue(m_position.x);
      
    }
  NS_LOG_UNCOND ("MyGetObservation: " << box);
  return box;
}

float MyGetReward(void)
{
  if(latency_values.empty()) return 0.0;
  double sum = 0.0;
  for(const auto &lat : latency_values){
    sum += lat;
  }
  double mean = sum / (double) latency_values.size();
  return  (float) mean;
}

bool MyExecuteActions(Ptr<OpenGymDataContainer> action)
{
  Ptr<OpenGymDiscreteContainer> discrete = DynamicCast<OpenGymDiscreteContainer>(action);
  NS_LOG_UNCOND ("MyExecuteActions: " << action);
  return true;
}

void ScheduleNextStateRead(double envStepTime, Ptr<OpenGymInterface> openGym)
{
  Simulator::Schedule (Seconds(envStepTime), &ScheduleNextStateRead, envStepTime, openGym);
  openGym->NotifyCurrentState();
}


RoutingExperiment::RoutingExperiment ()
  : port (9),
    bytesTotal (0),
    packetsReceived (0),
    m_CSVfileName ("manet-simulation.output.csv"),
    m_traceMobility (false),
    m_protocol (2) // AODV
{
}

static inline std::string
PrintReceivedPacket (Ptr<Socket> socket, Ptr<Packet> packet, Address senderAddress)
{
  std::ostringstream oss;

  oss << Simulator::Now ().GetSeconds () << " " << socket->GetNode ()->GetId ();

  if (InetSocketAddress::IsMatchingType (senderAddress))
    {
      InetSocketAddress addr = InetSocketAddress::ConvertFrom (senderAddress);
      oss << " received one packet from " << addr.GetIpv4 ();
    }
  else
    {
      oss << " received one packet!";
    }

  
  return oss.str ();
}

void RoutingExperiment::ReceivePacket (Ptr<Socket> socket)
{ 
  ReceivingTimes.insert(pair<uint32_t,Time>(socket->GetNode()->GetId() , Simulator::Now ()));
  uint32_t nodeId = socket->GetNode()->GetId();
  double currentLatency = Simulator::Now ().GetSeconds() - send_times[ptr_send_times[nodeId]];
  ptr_send_times[nodeId] ++;
  latency_values.push_back(currentLatency);
  NS_LOG_UNCOND ("node: "+ std::to_string(socket->GetNode()->GetId())+" received back a packet at "+std::to_string(Simulator::Now().GetSeconds())+ " seconds " + std::to_string(currentLatency) + "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");
}

void RoutingExperiment::SendPacket (Ptr<Socket> socket, uint32_t bytes)
{ 
  SendingTimes.insert(pair<uint32_t,Time>(socket->GetNode()->GetId(), Simulator::Now ()));
  NS_LOG_UNCOND ("node: "+ std::to_string(socket->GetNode()->GetId())+" sent a packet at "+std::to_string(Simulator::Now().GetSeconds())+ " seconds AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
}

void
RoutingExperiment::CheckThroughput ()
{
  NS_LOG_UNCOND ("Checking sending and receiving timessss \n");

  for ( const auto &p : SendingTimes )
  {
    NS_LOG_UNCOND("SendingTimes: " << p.first << '\t' << p.second<<'\n');
  }
  for ( const auto &p : ReceivingTimes )
  {
    NS_LOG_UNCOND("ReceivingTimes: " << p.first << '\t' << p.second<<'\n');
  }
  
  NS_LOG_UNCOND ("Current latencies logged \n");
  for ( const auto &p : latency_values )
  {
    NS_LOG_UNCOND("Latency Value: " << p <<'\n');
  }
  // double kbs = (bytesTotal * 8.0) / 1000;
  // bytesTotal = 0;

  // std::ofstream out (m_CSVfileName.c_str (), std::ios::app);

  // out << (Simulator::Now ()).GetSeconds () << ","
  //     << kbs << ","
  //     << packetsReceived << ","
  //     << m_nSinks << ","
  //     << m_protocolName << ","
  //     << m_txp << ""
  //     << std::endl;

  // out.close ();
  // packetsReceived = 0;
  Simulator::Schedule (Seconds (1.0), &RoutingExperiment::CheckThroughput, this);
}


std::string
RoutingExperiment::CommandSetup (int argc, char **argv)
{
  CommandLine cmd (__FILE__);
  cmd.AddValue ("CSVfileName", "The name of the CSV output file name", m_CSVfileName);
  cmd.AddValue ("traceMobility", "Enable mobility tracing", m_traceMobility);
  cmd.AddValue ("protocol", "1=OLSR;2=AODV;3=DSDV;4=DSR", m_protocol);
  cmd.Parse (argc, argv);
  return m_CSVfileName;
}

int main (int argc, char *argv[])
{
  RoutingExperiment experiment;
  std::string CSVfileName = experiment.CommandSetup (argc,argv);

  //blank out the last output file and write the column headers
  std::ofstream out (CSVfileName.c_str ());
  out << "SimulationSecond," <<
  "ReceiveRate," <<
  "PacketsReceived," <<
  "NumberOfSinks," <<
  "RoutingProtocol," <<
  "TransmissionPower" <<
  std::endl;
  out.close ();

  int nSinks = 3;
  double txp = 7.5;

  experiment.Run (nSinks, txp, CSVfileName);
}


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

Ptr<Socket>
RoutingExperiment::SetupPacketReceive(Ipv4Address addr, Ptr<Node> node, uint32_t checkPort)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> sink = Socket::CreateSocket (node, tid);
  InetSocketAddress local = InetSocketAddress (addr, checkPort);
  sink->Bind (local);
  sink->SetRecvCallback (MakeCallback (&RoutingExperiment::ReceivePacket, this));

  return sink;
}

Ptr<Socket>
RoutingExperiment::SetupPacketSend(Ipv4Address addr, Ptr<Node> node, uint32_t checkPort)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> sink = Socket::CreateSocket (node, tid);
  InetSocketAddress local = InetSocketAddress (addr, checkPort);
  sink->Bind (local);
  sink->SetSendCallback (MakeCallback (&RoutingExperiment::SendPacket, this));

  return sink;
}

void RoutingExperiment::Run(int nSinks, double txp, std::string CSVfileName)
{
  m_protocolName = "protocol";
  const int nodesPerCluster = 3;
  const int maxClusters = 3;
  Packet::EnablePrinting ();
  m_txp = txp;
  m_CSVfileName = CSVfileName;
    
  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_ALL);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_ALL);

  // Create clusters and cluster heads

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

    // Animation parameters

  double leftmost_cluster = 10.0;
  double cluster_x_delta = 30.0;
  double cluster_head_y = 10.0;
  double cluster_y = 60.0;

  // Movement
  for(int cluster = 0 ; cluster < maxClusters ; cluster ++){
      MobilityHelper currentMobility;
      currentMobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                              "MinX", DoubleValue (leftmost_cluster + cluster*cluster_x_delta),
                                              "MinY", DoubleValue (cluster_y),
                                              "DeltaX", DoubleValue (10.0),
                                              "DeltaY", DoubleValue (30.0),
                                              "GridWidth", UintegerValue (3),
                                              "LayoutType", StringValue ("RowFirst"));

      currentMobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                                  "Bounds", RectangleValue (
                                      Rectangle ( leftmost_cluster + cluster*cluster_x_delta,
                                                  leftmost_cluster + (cluster+1.0)*cluster_x_delta,
                                                  -100,
                                                  100 )));
      currentMobility.Install (clusters[cluster]);
  }
  

  AnimationInterface anim("manetSimulator.xml");
  for(int cluster = 0 ; cluster < maxClusters ; cluster ++){
      anim.SetConstantPosition(clusterHeads[cluster].Get(0),
          leftmost_cluster+cluster*30.0, (cluster%2 == 0) ? cluster_head_y : cluster_head_y*1.5 );
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

  // Program calls

  UdpEchoServerHelper echoServer (9);

  for( int mainClusterNode = 0 ; mainClusterNode < nodesPerCluster ; mainClusterNode ++ ){
      ApplicationContainer serverApps = echoServer.Install (clusters[0].Get (mainClusterNode));
      serverApps.Start (Seconds (0.0));
      serverApps.Stop (Seconds (30.0));
  }

  std::vector <UdpEchoClientHelper> echoClients;
  for(int clientApp = 0 ; clientApp < nodesPerCluster ; clientApp ++){
      UdpEchoClientHelper echoClient (intoClusterHeadInterfaces[0][clientApp].GetAddress (0), 9);
      echoClient.SetAttribute ("MaxPackets", UintegerValue (15));
      echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
      echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
      echoClients.push_back(echoClient);
  }

  // Set up calls from cluster 1
  for(int node = 0 ; node < nodesPerCluster ; node ++){
      UdpEchoClientHelper echoClient = echoClients[(node)%nodesPerCluster];
      ApplicationContainer clientApps = echoClient.Install (clusters[1].Get (node));
      clientApps.Start (Seconds (5.0));
      clientApps.Stop (Seconds (20.0));
  }

  // Set up calls from cluster 2
  for(int node = 0 ; node < nodesPerCluster ; node ++){
      UdpEchoClientHelper echoClient = echoClients[(node)%nodesPerCluster];
      ApplicationContainer clientApps = echoClient.Install (clusters[2].Get (node));
      clientApps.Start (Seconds (10.0));
      clientApps.Stop (Seconds (25.0));
  }

  //Set up latency logger
  std::vector <Ptr<Socket>> sinks;
  NS_LOG_UNCOND ("setting up address: " << intoClusterHeadInterfaces[0][0].GetAddress(1) << " with node: " << std::to_string(clusters[0].Get(0)->GetId()));
  sinks.push_back( SetupPacketReceive(intoClusterHeadInterfaces[0][0].GetAddress(1), clusters[0].Get(0), 9) );
  // sinks.push_back( SetupPacketSend(intoClusterHeadInterfaces[0][0].GetAddress(1), clusters[0].Get(0), 9) );

  NS_LOG_UNCOND ("setting up address: " << intoClusterHeadInterfaces[0][1].GetAddress(0) << " with node: " << std::to_string(clusters[0].Get(1)->GetId()));
  sinks.push_back( SetupPacketReceive(intoClusterHeadInterfaces[0][1].GetAddress(0), clusters[0].Get(1), 9) );
  // sinks.push_back( SetupPacketSend(intoClusterHeadInterfaces[0][1].GetAddress(0), clusters[0].Get(1), 9) );

  NS_LOG_UNCOND ("setting up address: " << intoClusterHeadInterfaces[0][2].GetAddress(0) << " with node: " << std::to_string(clusters[0].Get(2)->GetId()));
  sinks.push_back( SetupPacketReceive(intoClusterHeadInterfaces[0][2].GetAddress(0), clusters[0].Get(2), 9) );
  // sinks.push_back( SetupPacketSend(intoClusterHeadInterfaces[0][2].GetAddress(0), clusters[0].Get(2), 9) );

  // NS_LOG_UNCOND ("setting up address: " << intoClusterHeadInterfaces[1][0].GetAddress(0) << " with node: " << std::to_string(clusters[1].Get(0)->GetId()));
  // sinks.push_back( SetupPacketReceive(intoClusterHeadInterfaces[1][0].GetAddress(0), clusters[1].Get(0), 49153) );
  // sinks.push_back( SetupPacketSend(intoClusterHeadInterfaces[1][0].GetAddress(0), clusters[1].Get(0), 49153) );

  // NS_LOG_UNCOND ("setting up address: " << intoClusterHeadInterfaces[1][1].GetAddress(0) << " with node: " << std::to_string(clusters[1].Get(1)->GetId()));
  // sinks.push_back( SetupPacketReceive(intoClusterHeadInterfaces[1][1].GetAddress(0), clusters[1].Get(1), 49153) );
  // sinks.push_back( SetupPacketSend(intoClusterHeadInterfaces[1][1].GetAddress(0), clusters[1].Get(1), 49153) );

  // NS_LOG_UNCOND ("setting up address: " << intoClusterHeadInterfaces[1][2].GetAddress(0) << " with node: " << std::to_string(clusters[1].Get(2)->GetId()));
  // sinks.push_back( SetupPacketReceive(intoClusterHeadInterfaces[1][2].GetAddress(0), clusters[1].Get(2), 49153) );
  // sinks.push_back( SetupPacketSend(intoClusterHeadInterfaces[1][2].GetAddress(0), clusters[1].Get(2), 495153) );

  // NS_LOG_UNCOND ("setting up address: " << intoClusterHeadInterfaces[2][0].GetAddress(0) << " with node: " << std::to_string(clusters[2].Get(0)->GetId()));
  // sinks.push_back( SetupPacketReceive(intoClusterHeadInterfaces[2][0].GetAddress(0), clusters[2].Get(0), 49153) );
  // sinks.push_back( SetupPacketSend(intoClusterHeadInterfaces[2][0].GetAddress(0), clusters[2].Get(0), 49153) );

  // NS_LOG_UNCOND ("setting up address: " << intoClusterHeadInterfaces[2][1].GetAddress(0) << " with node: " << std::to_string(clusters[2].Get(1)->GetId()));
  // sinks.push_back( SetupPacketReceive(intoClusterHeadInterfaces[2][1].GetAddress(0), clusters[2].Get(1), 49153) );
  // sinks.push_back( SetupPacketSend(intoClusterHeadInterfaces[2][1].GetAddress(0), clusters[2].Get(1), 49153) );

  // NS_LOG_UNCOND ("setting up address: " << intoClusterHeadInterfaces[2][2].GetAddress(0) << " with node: " << std::to_string(clusters[2].Get(2)->GetId()));
  // sinks.push_back( SetupPacketReceive(intoClusterHeadInterfaces[2][2].GetAddress(0), clusters[2].Get(2), 49153) );
  // sinks.push_back( SetupPacketSend(intoClusterHeadInterfaces[2][2].GetAddress(0), clusters[2].Get(2), 49153) );
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  


  AsciiTraceHelper ascii;
  std::string tr_name ("manet-routing-compare");
  MobilityHelper::EnableAsciiAll (ascii.CreateFileStream (tr_name + ".mob"));

  double envStepTime = 0.5; //seconds, ns3gym env step time interval
  uint32_t openGymPort = 5555;
  Ptr<OpenGymInterface> openGym = CreateObject<OpenGymInterface> (openGymPort);
  openGym->SetGetActionSpaceCb( MakeCallback (&MyGetActionSpace) );
  openGym->SetGetObservationSpaceCb( MakeCallback (&MyGetObservationSpace) );
  openGym->SetGetGameOverCb( MakeCallback (&MyGetGameOver) );
  openGym->SetGetObservationCb( MakeCallback (&MyGetObservation) );
  
  openGym->SetGetRewardCb( MakeCallback (&MyGetReward) );
  openGym->SetExecuteActionsCb( MakeCallback (&MyExecuteActions) );
  Simulator::Schedule (Seconds(0.0), &ScheduleNextStateRead, envStepTime, openGym);


  CheckThroughput ();

  //Simulator::Stop (Seconds (TotalTime));
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();
  Simulator::Stop (Seconds (30.0));
  Simulator::Run ();
  flowMonitor->SerializeToXmlFile("NameOfFile.xml", true, true);
  

  Simulator::Destroy ();
}

