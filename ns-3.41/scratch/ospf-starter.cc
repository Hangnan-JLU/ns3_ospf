/*
 *  Copyright (c) 2024 Liverpool Hope University, UK
 *  Authors:
 *      Mark Greenwood
 *      Nathan Nunes
 *
 *  File: ospf-starter.cc ns-3 scratch file
 *
 *  Simple starting topology
 *  ========================
 *  src(host) <--> r1 (OSPF enabled router) <--> r2 (OSPF enabled router) <--> dst (host)
 *
 *  Purpose is just to start building the basic plumbing for implementation of
 *  the OSPF protocol in ns-3/internet
 *
 *
 */

//terrys git tester
//nathans git tester
//testing git connectivity

#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ospf-header.h"

#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("OspfStarter");

int main(int argc, char** argv) {

    // We want verbose logging and at some point we'd like to print
    // the evolving routing tables.
    bool verbose = true;
    bool printRoutingTables = true;
    bool showPings = false;

    if (verbose)
    {
        LogComponentEnableAll(LogLevel(LOG_PREFIX_TIME | LOG_PREFIX_NODE));
        //LogComponentEnable("RipSimpleRouting", LOG_LEVEL_INFO);
        //LogComponentEnable("Rip", LOG_LEVEL_ALL);
        LogComponentEnable("Ipv4Interface", LOG_LEVEL_ALL);
        LogComponentEnable("Icmpv4L4Protocol", LOG_LEVEL_ALL);
        LogComponentEnable("Ipv4L3Protocol", LOG_LEVEL_ALL);
        LogComponentEnable("ArpCache", LOG_LEVEL_ALL);
        LogComponentEnable("Ping", LOG_LEVEL_ALL);
    }

    // Create our Nodes; src, r1, r2 and dst
    NS_LOG_INFO("Create nodes.");
    Ptr<Node> src = CreateObject<Node>();
    Names::Add("SrcNode", src);
    Ptr<Node> r1 = CreateObject<Node>();
    Names::Add("OspfRouter1", r1);
    Ptr<Node> r2 = CreateObject<Node>();
    Names::Add("OspfRouter2", r2);
    Ptr<Node> dst = CreateObject<Node>();
    Names::Add("DstNode", dst);

    // Wrap the Nodes into NodeContainers
    NodeContainer nc_src_r1(src, r1);
    NodeContainer nc_r1_r2(r1, r2);
    NodeContainer nc_r2_dst(r2, dst);
    NodeContainer nc_src_dst(src, dst);

    NS_LOG_INFO("Create channels.");
    CsmaHelper csma_helper;
    csma_helper.SetChannelAttribute("DataRate", DataRateValue(5000000));
    csma_helper.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
    NetDeviceContainer ndc_src_r1 = csma_helper.Install(nc_src_r1);
    NetDeviceContainer ndc_r1_r2 = csma_helper.Install(nc_r1_r2);
    NetDeviceContainer ndc_r2_dst = csma_helper.Install(nc_r2_dst);

    printf("Hello World");
    OspfHelper ospfHelper;

    //ospfHelper.Install(r1);
    //ospfHelper.Install(r2);

    Ipv4ListRoutingHelper routingHelperList;
    routingHelperList.Add(ospfHelper, 0);

    InternetStackHelper internet;
    internet.SetIpv6StackInstall(false);
    internet.SetRoutingHelper(routingHelperList);
    internet.Install(nc_r1_r2);

    InternetStackHelper internetNodes;
    internetNodes.SetIpv6StackInstall(false);
    internetNodes.Install(nc_src_dst);

    NS_LOG_INFO("Assign IPv4 Addresses.");
    Ipv4AddressHelper ipv4;

    ipv4.SetBase(Ipv4Address("10.0.0.0"), Ipv4Mask("255.255.255.0"));
    Ipv4InterfaceContainer iic1 = ipv4.Assign(ndc_src_r1);

    ipv4.SetBase(Ipv4Address("10.0.1.0"), Ipv4Mask("255.255.255.0"));
    Ipv4InterfaceContainer iic2 = ipv4.Assign(ndc_r1_r2);

    ipv4.SetBase(Ipv4Address("10.0.2.0"), Ipv4Mask("255.255.255.0"));
    Ipv4InterfaceContainer iic3 = ipv4.Assign(ndc_r2_dst);

    Ptr<Ipv4StaticRouting> staticRouting;
    staticRouting = Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting>(
            src->GetObject<Ipv4>()->GetRoutingProtocol());
    staticRouting->SetDefaultRoute("10.0.0.2", 1);
    staticRouting = Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting>(
            dst->GetObject<Ipv4>()->GetRoutingProtocol());
    staticRouting->SetDefaultRoute("10.0.2.1", 1);

    if(printRoutingTables) {

    }

    //ospfHelper.Install(r1);
    //ospfHelper.Install(r2);

    ospfHelper.AssignAreaNumber(r1, 0);
    ospfHelper.AssignAreaNumber(r2, 0);

    NS_LOG_INFO("Create Applications.");
    uint32_t packetSize = 1024;
    Time interPacketInterval = Seconds(1.0);
    PingHelper ping(Ipv4Address("10.0.2.2"));

    ping.SetAttribute("Interval", TimeValue(interPacketInterval));
    ping.SetAttribute("Size", UintegerValue(packetSize));
    if (showPings)
    {
        ping.SetAttribute("VerboseMode", EnumValue(Ping::VerboseMode::VERBOSE));
    }
    ApplicationContainer apps = ping.Install(src);
    apps.Start(Seconds(1.0));
    apps.Stop(Seconds(110.0));

    AsciiTraceHelper ascii;
    csma_helper.EnableAsciiAll(ascii.CreateFileStream("ospf-starter.tr"));
    csma_helper.EnablePcapAll("ospf-starter", true);

    /* Now, do the actual simulation. */
    NS_LOG_INFO("Run Simulation.");
    Simulator::Stop(Seconds(131.0));
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Done.");

    return 0;
}
