/*
 *  Copyright (c) 2024 Liverpool Hope University, UK
 *  Authors:
 *      Mark Greenwood
 *      Nathan Nunes
 *
 *  File: ospf-protocol.cc
 *
 */

#include "ospf-l4-protocol.h"

#include "ipv4-end-point-demux.h"
#include "ipv4-end-point.h"
#include "ipv4-route.h"
#include "ipv4.h"
#include "ipv6-end-point-demux.h"
#include "ipv6-end-point.h"
#include "ipv6-route.h"
#include "ipv6.h"

#include "ns3/assert.h"
#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/object-map.h"
#include "ns3/packet.h"

#include "ipv4-packet-info-tag.h"

#include <unordered_map>
#include <set>

#define OSPF_ALL_NODE "224.0.0.5"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("OspfL4Protocol");
NS_OBJECT_ENSURE_REGISTERED(OspfL4Protocol);

/* see http://www.iana.org/assignments/protocol-numbers */
const uint8_t OspfL4Protocol::PROTOCOL_NUMBER = 89;


// Constructor
OspfL4Protocol::OspfL4Protocol()
        : m_endPoints(new Ipv4EndPointDemux()),
          m_endPoints6(new Ipv6EndPointDemux())
{
    NS_LOG_FUNCTION(this);
    m_neighbor_table = OspfNeighborTable();
}

// Destructor
OspfL4Protocol::~OspfL4Protocol(){
    NS_LOG_FUNCTION(this);
}

// GetTypeId
TypeId OspfL4Protocol::GetTypeId() {
        static TypeId tid;
        tid = TypeId("ns3::OspfL4Protocol")
                .SetParent<IpL4Protocol>()
                .SetGroupName("Internet")
                .AddConstructor<OspfL4Protocol>();
    return tid;
}

// Set the owning Node
void OspfL4Protocol::SetNode(Ptr<Node> node) {
    m_node = node;
    m_routerId = node->GetId();
}

/******************************************************************************
 *
 * MRG: Implementation of pure virtual and virtual functions inherited from
 * pv4L4Protocol.
 *
 * See documentation in ospf-protocol.h
 *
 ******************************************************************************/

// GetProtocolNumber
int OspfL4Protocol::GetProtocolNumber() const {
    return PROTOCOL_NUMBER;
}

// DoDispose
void OspfL4Protocol::DoDispose() {
    // TODO
    NS_LOG_FUNCTION(this);
    if (m_endPoints != nullptr)
    {
        delete m_endPoints;
        m_endPoints = nullptr;
    }
    if (m_endPoints6 != nullptr)
    {
        delete m_endPoints6;
        m_endPoints6 = nullptr;
    }
    m_node = nullptr;
    m_downTarget.Nullify();
    m_downTarget6.Nullify();
    IpL4Protocol::DoDispose();
}

// NotifyNewAggregate
void OspfL4Protocol::NotifyNewAggregate() {

    NS_LOG_FUNCTION(this);

    Ptr<Node> node = this->GetObject<Node>();
    Ptr<Ipv4> ipv4 = this->GetObject<Ipv4>();
    Ptr<Ipv6> ipv6 = node->GetObject<Ipv6>();

    if (!m_node)
    {
        if (node && (ipv4 || ipv6))
        {
            this->SetNode(node);
            // Ptr<UdpSocketFactoryImpl> udpFactory = CreateObject<UdpSocketFactoryImpl>();
            // udpFactory->SetUdp(this);
            // node->AggregateObject(udpFactory);
        }
    }

    if (ipv4 && m_downTarget.IsNull())
    {
        ipv4->Insert(this);
        this->SetDownTarget(MakeCallback(&Ipv4::Send, ipv4));
    }
    if (ipv6 && m_downTarget6.IsNull())
    {
        ipv6->Insert(this);
        this->SetDownTarget6(MakeCallback(&Ipv6::Send, ipv6));
    }
    IpL4Protocol::NotifyNewAggregate();
}

// SetDownTarget
void OspfL4Protocol::SetDownTarget(IpL4Protocol::DownTargetCallback cb) {
    NS_LOG_FUNCTION(this);
    m_downTarget = cb;
}

// SetDownTarget6
void OspfL4Protocol::SetDownTarget6(IpL4Protocol::DownTargetCallback6 cb) {
    NS_LOG_FUNCTION(this);
    m_downTarget6 = cb;
}

// GetDownTarget
IpL4Protocol::DownTargetCallback OspfL4Protocol::GetDownTarget() const {
    NS_LOG_FUNCTION(this);
    return m_downTarget;
}

// GetDownTarget6
IpL4Protocol::DownTargetCallback6 OspfL4Protocol::GetDownTarget6() const {
    NS_LOG_FUNCTION(this);
    return m_downTarget6;
}

void OspfL4Protocol::Send(Ptr<Packet> packet, Ipv4Address saddr, Ipv4Address daddr, Ipv4Mask ipv4Mask, OspfHeader ospfHeader, int packetType, int currentState)
{
    NS_LOG_FUNCTION(this << packet << saddr << daddr);

    ospfHeader.InitializeChecksum(saddr, daddr, OspfL4Protocol::PROTOCOL_NUMBER);
    ospfHeader.SetState(currentState);
    ospfHeader.SetPacketType(packetType);
    ospfHeader.SetMask(ipv4Mask);

    packet->AddHeader(ospfHeader);

    m_downTarget(packet, saddr, daddr, OspfL4Protocol::PROTOCOL_NUMBER, nullptr);
}

void OspfL4Protocol::Send(Ptr<Packet> packet, Ipv4Address saddr, Ipv4Address daddr, Ipv4Mask ipv4Mask, OspfHeader ospfHeader, int packetType, int currentState, Ptr<Ipv4Route> route)
{
    NS_LOG_FUNCTION(this << packet << saddr << daddr << route);

    ospfHeader.InitializeChecksum(saddr, daddr, OspfL4Protocol::PROTOCOL_NUMBER);
    ospfHeader.SetState(currentState);
    ospfHeader.SetPacketType(packetType);
    ospfHeader.SetMask(ipv4Mask);

    packet->AddHeader(ospfHeader);

    m_downTarget(packet, saddr, daddr, OspfL4Protocol::PROTOCOL_NUMBER, route);
}

void OspfL4Protocol::Send(Ptr<Packet> packet, Ipv6Address saddr, Ipv6Address daddr)
{
    NS_LOG_FUNCTION(this << packet << saddr << daddr);

    OspfHeader ospfHeader;

    //create iv6 for header later
    //ospfHeader.InitializeChecksum(saddr, daddr, OspfL4Protocol::PROTOCOL_NUMBER);

    packet->AddHeader(ospfHeader);

    m_downTarget6(packet, saddr, daddr, OspfL4Protocol::PROTOCOL_NUMBER, nullptr);
}

void OspfL4Protocol::Send(Ptr<Packet> packet, Ipv6Address saddr, Ipv6Address daddr, Ptr<Ipv6Route> route)
{
    NS_LOG_FUNCTION(this << packet << saddr << daddr << route);

    OspfHeader ospfHeader;

    //create ipv6 for header later
    //ospfHeader.InitializeChecksum(saddr, daddr, OspfL4Protocol::PROTOCOL_NUMBER);

    packet->AddHeader(ospfHeader);

    m_downTarget6(packet, saddr, daddr, OspfL4Protocol::PROTOCOL_NUMBER, route);
}

IpL4Protocol::RxStatus
OspfL4Protocol::Receive(Ptr<Packet> packet, const Ipv4Header& header, Ptr<Ipv4Interface> interface)
{
    NS_LOG_FUNCTION("Receive" << this << packet << header);
    OspfHeader ospfHeader;

    ospfHeader.InitializeChecksum(header.GetSource(), header.GetDestination(), OspfL4Protocol::PROTOCOL_NUMBER);

    packet->PeekHeader(ospfHeader);

    Ipv4PacketInfoTag interfaceInfo;
    if (!packet->RemovePacketTag(interfaceInfo))
    {
        NS_ABORT_MSG("No incoming interface on RIP message, aborting.");
    }
    uint32_t incomingIf = interfaceInfo.GetRecvIf();

    if (ospfHeader.GetState() == States::DOWN){
        HandleDownResponse(packet, header, ospfHeader, interface, incomingIf);
    }
    if (ospfHeader.GetState() == States::INIT){
        HandleInitResponse(packet, header, ospfHeader, interface, incomingIf);
    }
    if (ospfHeader.GetState() == States::TWO_WAY){
        HandleTwoWayResponse(packet, header, ospfHeader, interface, incomingIf);
    }

    return IpL4Protocol::RX_OK;
}

IpL4Protocol::RxStatus
OspfL4Protocol::Receive(Ptr<Packet> packet, const Ipv6Header& header, Ptr<Ipv6Interface> interface)
{
    NS_LOG_FUNCTION("Receive" << this << packet << header.GetSource() << header.GetDestination());
    OspfHeader ospfHeader;
    //implement for ipv6
    return IpL4Protocol::RX_OK;
}


void
OspfL4Protocol::ReceiveIcmp(Ipv4Address icmpSource,
                           uint8_t icmpTtl,
                           uint8_t icmpType,
                           uint8_t icmpCode,
                           uint32_t icmpInfo,
                           Ipv4Address payloadSource,
                           Ipv4Address payloadDestination,
                           const uint8_t payload[8])
{
    NS_LOG_FUNCTION(this << icmpSource << icmpTtl << icmpType << icmpCode << icmpInfo
                         << payloadSource << payloadDestination);
    uint16_t src;
    uint16_t dst;
    src = payload[0] << 8;
    src |= payload[1];
    dst = payload[2] << 8;
    dst |= payload[3];

    Ipv4EndPoint* endPoint = m_endPoints->SimpleLookup(payloadSource, src, payloadDestination, dst);
    if (endPoint != nullptr)
    {
        endPoint->ForwardIcmp(icmpSource, icmpTtl, icmpType, icmpCode, icmpInfo);
    }
    else
    {
        NS_LOG_DEBUG("no endpoint found source=" << payloadSource
                                                 << ", destination=" << payloadDestination
                                                 << ", src=" << src << ", dst=" << dst);
    }
}

void
OspfL4Protocol::ReceiveIcmp(Ipv6Address icmpSource,
                           uint8_t icmpTtl,
                           uint8_t icmpType,
                           uint8_t icmpCode,
                           uint32_t icmpInfo,
                           Ipv6Address payloadSource,
                           Ipv6Address payloadDestination,
                           const uint8_t payload[8])
{
    NS_LOG_FUNCTION(this << icmpSource << icmpTtl << icmpType << icmpCode << icmpInfo
                         << payloadSource << payloadDestination);
    uint16_t src;
    uint16_t dst;
    src = payload[0] << 8;
    src |= payload[1];
    dst = payload[2] << 8;
    dst |= payload[3];

    Ipv6EndPoint* endPoint =
            m_endPoints6->SimpleLookup(payloadSource, src, payloadDestination, dst);
    if (endPoint != nullptr)
    {
        endPoint->ForwardIcmp(icmpSource, icmpTtl, icmpType, icmpCode, icmpInfo);
    }
    else
    {
        NS_LOG_DEBUG("no endpoint found source=" << payloadSource
                                                 << ", destination=" << payloadDestination
                                                 << ", src=" << src << ", dst=" << dst);
    }
}

void OspfL4Protocol::startDownState()
{
    OspfNeighborTable::neighborList neighbor_table = m_neighbor_table.getCurrentNeighbors();
    for (uint32_t i = 0; i < m_ipv4->GetNInterfaces(); i++)
    {
        Ptr<LoopbackNetDevice> check = DynamicCast<LoopbackNetDevice>(m_ipv4->GetNetDevice(i));
        if (check) {
            continue;
        }

        bool activeInterface = false;
        if (m_interfaceExclusions.find(i) == m_interfaceExclusions.end())
        {
            activeInterface = true;
            m_ipv4->SetForwarding(i, true);
        }

        for (uint32_t j = 0; j < m_ipv4->GetNAddresses(i); j++)
        {
            bool isInterfaceFull = false;
            Ipv4InterfaceAddress address = m_ipv4->GetAddress(i, j);
            if (address.GetScope() != Ipv4InterfaceAddress::HOST && activeInterface) {
                if (!neighbor_table.empty()) {
                    for (const auto &row: neighbor_table) {
                        for (const auto &tableItems: row) {
                            if (address == tableItems.ipInterface->GetAddress(i)) {
                                isInterfaceFull = true;
                            }
                        }
                    }
                }
                if (!isInterfaceFull) {
                    SendDownPacket(address);
                }
            }
        }
    }
}

void OspfL4Protocol::SetIpv4(Ptr<Ipv4> the_ipv4)
{
    m_ipv4 = the_ipv4;
}

void OspfL4Protocol::SetExclusions(std::set<uint32_t> iExclusions)
{
    m_interfaceExclusions = iExclusions;
}

void OspfL4Protocol::SendDownPacket(Ipv4InterfaceAddress address)
{
    Ptr<Packet> p = Create<Packet>();
    OspfHello helloHeader;
    helloHeader.setNeighbors(m_neighbor_table.getCurrentNeighbors());
    helloHeader.setRouterId(m_routerId);
    helloHeader.setAreaId(m_areaId);
    if (address.GetScope() != Ipv4InterfaceAddress::HOST)
    {
        Send(p, address.GetLocal(), OSPF_ALL_NODE, address.GetMask(), helloHeader, PacketType::HELLO, States::DOWN);
    }
}


void OspfL4Protocol::HandleDownResponse(Ptr<Packet> packet, Ipv4Header header, OspfHeader ospfHeader, Ptr<Ipv4Interface> interface, uint32_t incomingIf){
    //TODO manage when down is received but no down is sent i.e. straight to 2-Way
    OspfHello helloHeader;
    packet->PeekHeader(helloHeader);
    if (helloHeader.getAreaId() == m_areaId and ospfHeader.GetMask() == interface->GetAddress(incomingIf).GetMask()) {
        m_neighbor_table.addNeighbors(header.GetSource(), ospfHeader.GetMask(), interface, ospfHeader.GetState(),
                                      helloHeader.getRouterId());
        SendInitPacket(interface->GetAddress(incomingIf), header.GetSource());
    }
}

void OspfL4Protocol::SendInitPacket(Ipv4InterfaceAddress address, Ipv4Address daddr){
    Ptr<Packet> p = Create<Packet>();
    OspfHello helloHeader;
    helloHeader.setNeighbors(m_neighbor_table.getCurrentNeighbors());
    helloHeader.setRouterId(m_routerId);
    helloHeader.setAreaId(m_areaId);
    if (address.GetScope() != Ipv4InterfaceAddress::HOST)
    {
        Send(p, address.GetLocal(), daddr, address.GetMask(), helloHeader, PacketType::HELLO, States::INIT);
    }
}

void OspfL4Protocol::HandleInitResponse(Ptr<Packet> packet, Ipv4Header header, OspfHeader ospfHeader, Ptr<Ipv4Interface> interface, uint32_t incomingIf){
    //TODO loop through ospfheader table if our IP address exists, if so move to 2-Way state
    OspfHello helloHeader;
    packet->PeekHeader(helloHeader);
    OspfNeighborTable::neighborList neighbors = helloHeader.getNeighbors();
    if (neighbors.empty()){
        SendDownPacket(interface->GetAddress(incomingIf));
    }else{
        bool check = false;
        for (const auto& row : neighbors){
            for (const auto& neighborItems : row){
                if (neighborItems.router_id == m_routerId){
                    check = true;
                }
            }
        }
        if (helloHeader.getAreaId() != m_areaId){
            check = false;
        }
        if (check){
            bool add_neighbor = false;
            OspfNeighborTable::neighborList m_current_neighbors = m_neighbor_table.getCurrentNeighbors();
            if (m_current_neighbors.empty()){
                add_neighbor = true;
            }else{
                bool iterator_check = false;
                for (const auto& row : m_current_neighbors){
                    for (const auto& neighborItems : row){
                        if (neighborItems.router_id == helloHeader.getRouterId()){
                            iterator_check = true;
                        }
                    }
                }
                if (iterator_check){
                    add_neighbor = false;
                }else{
                    add_neighbor = true;
                }
            }
            if (add_neighbor) {
                m_neighbor_table.addNeighbors(header.GetSource(), ospfHeader.GetMask(), interface,
                                              ospfHeader.GetState(), helloHeader.getRouterId());
                SendTwoWayPacket(interface->GetAddress(incomingIf), header.GetSource());
            }else{
                m_neighbor_table.set_State(States::INIT, m_routerId);
                SendTwoWayPacket(interface->GetAddress(incomingIf), header.GetSource());
            }
        }else{
            SendDownPacket(interface->GetAddress(incomingIf));
        }
    }
}

void OspfL4Protocol::SendTwoWayPacket(Ipv4InterfaceAddress address, Ipv4Address daddr){
    Ptr<Packet> p = Create<Packet>();
    OspfHello helloHeader;
    helloHeader.setNeighbors(m_neighbor_table.getCurrentNeighbors());
    helloHeader.setRouterId(m_routerId);
    helloHeader.setAreaId(m_areaId);
    if (address.GetScope() != Ipv4InterfaceAddress::HOST)
    {
        Send(p, address.GetLocal(), daddr, address.GetMask(), helloHeader, PacketType::HELLO, States::TWO_WAY);
    }
}

void OspfL4Protocol::HandleTwoWayResponse(Ptr<Packet> packet, Ipv4Header header, OspfHeader ospfHeader, Ptr<Ipv4Interface> interface, uint32_t incomingIf){
    OspfHello helloHeader;
    packet->PeekHeader(helloHeader);
    OspfNeighborTable::neighborList neighbors = helloHeader.getNeighbors();
    if (neighbors.empty()){
        SendDownPacket(interface->GetAddress(incomingIf));
    }else{
        bool check = false;
        for (const auto& row : neighbors){
            for (const auto& neighborItems : row){
                if (neighborItems.router_id == m_routerId){
                    check = true;
                }
            }
        }
        if (helloHeader.getAreaId() != m_areaId){
            check = false;
        }
        if (check){
            bool neighbor_exist = true;
            OspfNeighborTable::neighborList m_current_neighbors = m_neighbor_table.getCurrentNeighbors();
            if (m_current_neighbors.empty()){
                neighbor_exist = false;
            }else{
                //check routers own neighbor table to see if entry for neighbor exists
                bool iterator_check = false;
                for (const auto& row : m_current_neighbors){
                    for (const auto& neighborItems : row){
                        if (neighborItems.router_id == helloHeader.getRouterId()){
                            iterator_check = true;
                        }
                    }
                }
                if (iterator_check){
                    neighbor_exist = true;
                }else{
                    neighbor_exist = false;
                }
            }
            if (neighbor_exist) {
                //check if state is already TWO_WAY if so needs to proceed to Exchange State else send TWO_WAY packet
                int state = m_neighbor_table.get_State(m_routerId);
                if (state == States::TWO_WAY){
                    //Start Exchange state....send DBD packets
                }else{
                    m_neighbor_table.set_State(States::TWO_WAY, m_routerId);
                    SendTwoWayPacket(interface->GetAddress(incomingIf), header.GetSource());
                }
            }else{
                //should send DOWN state packet as entry should already be recorded
                SendDownPacket(interface->GetAddress(incomingIf));
            }
        }else{
            SendDownPacket(interface->GetAddress(incomingIf));
        }
    }
}


void OspfL4Protocol::SetOspfAreaType(int area_id){
    m_areaId = area_id;
}

Ipv4EndPoint* OspfL4Protocol::Allocate(Ipv4Address address)
{
    NS_LOG_FUNCTION(this << address);
    return m_endPoints->Allocate(address);
}

}
