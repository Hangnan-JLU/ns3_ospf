/*
 *  Copyright (c) 2024 Liverpool Hope University, UK
 *
 *  Authors:
 *      Mark Greenwood
 *      Nathan Nunes
 *
 *  File: ospf-l4-protocol.h
 *
 *  Represents the OSPF protocol.
 *
 *  MRG: In ns3 any protocol that uses IPv4 as its transport layer extends
 *  the IpL4Protocol class which contains a lot of the obvious plumbing
 *  for inserting into a node and connecting to the underlying network layer
 *
 */

#ifndef OSPF_L4_PROTOCOL_H
#define OSPF_L4_PROTOCOL_H

#include "ip-l4-protocol.h"
#include "ipv6-end-point-demux.h"
#include "ipv4.h"

#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/node.h"
#include "ospf-hello.h"

#include <stdint.h>
#include <unordered_map>
#include <set>

#include "loopback-net-device.h"

namespace ns3 {

// Forward class declarations
class Node;
class Ipv4EndPointDemux;
class Ipv4EndPoint;
class Ipv6EndPointDemux;
class Ipv6EndPoint;
class NetDevice;
class OspfNeighborTable;
class OspfHello;
class OspfHeader;
class OspfRouting;


class OspfL4Protocol : public IpL4Protocol {

  public:

    static const uint8_t PROTOCOL_NUMBER; // protocol number ( 0x59 or 89 decimal )

    OspfL4Protocol();
    ~OspfL4Protocol() override;

    enum States
    {
        DOWN = 0,
        ATTEMPT = 1,
        INIT = 2,
        TWO_WAY = 3,
        EXSTART = 4,
        EXCHANGE = 5,
        LOADING = 6,
        FULL = 7
    };

    enum PacketType
    {
        HELLO = 0,
        DBD = 1,
        LSA = 2,
        LSU = 3,
        LSAck = 4
    };

    // Delete copy constructor and assignment operator to avoid misuse
    OspfL4Protocol(const OspfL4Protocol&) = delete;
    OspfL4Protocol& operator=(const OspfL4Protocol&) = delete;

    /**
     * Set node associated with this stack
     * \param node the node
     */
    void SetNode(Ptr<Node> node);

    int GetProtocolNumber() const override;

    void SetOspfAreaType(int);

    /**************************************************************************
     *
     * MRG: Inherited from IpL4Protocol
     *
     * You should look at the header file for this class but - I've copied this
     * documentation into this file.
     *
     * In our (LHU) implementation we are only going to support Ipv4 routing
     *
     *************************************************************************/

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * \brief Called from lower-level layers to send the packet up in the stack.
     * \param p packet to forward up
     * \param header IPv4 Header information
     * \param incomingInterface the Ipv4Interface on which the packet arrived
     * \returns Rx status code
     */
    IpL4Protocol::RxStatus Receive(Ptr<Packet> p,
                                   const Ipv4Header& header,
                                   Ptr<Ipv4Interface> interface) override;

    /**
     * \brief Called from lower-level layers to send the packet up in the stack.
     * \param p packet to forward up
     * \param header IPv6 Header information
     * \param incomingInterface the Ipv6Interface on which the packet arrived
     * \returns Rx status code
     */
    IpL4Protocol::RxStatus Receive(Ptr<Packet> p,
                                   const Ipv6Header& header,
                                   Ptr<Ipv6Interface> interface) override;

    /**
     * This method allows a caller to set the current down target callback
     * set for this L4 protocol (IPv4 case)
     *
     * \param cb current Callback for the L4 protocol
     */
    void SetDownTarget(IpL4Protocol::DownTargetCallback cb) override;

    /**
     * This method allows a caller to set the current down target callback
     * set for this L4 protocol (IPv6 case)
     *
     * \param cb current Callback for the L4 protocol
     */
    void SetDownTarget6(IpL4Protocol::DownTargetCallback6 cb) override;

    /**
     * This method allows a caller to get the current down target callback
     * set for this L4 protocol (IPv4 case)
     *
     * \return current Callback for the L4 protocol
     */
    IpL4Protocol::DownTargetCallback GetDownTarget() const override;

    /**
     * This method allows a caller to get the current down target callback
     * set for this L4 protocol (IPv6 case)
     *
     * \return current Callback for the L4 protocol
     */
    IpL4Protocol::DownTargetCallback6 GetDownTarget6() const override;

    /**
     * Destructor implementation.
     *
     * This method is called by Dispose() or by the Object's
     * destructor, whichever comes first.
     *
     * Subclasses are expected to implement their real destruction
     * code in an overridden version of this method and chain
     * up to their parent's implementation once they are done.
     * _i.e_, for simplicity, the destructor of every subclass should
     * be empty and its content should be moved to the associated
     * DoDispose() method.
     *
     * It is safe to call GetObject() from within this method.
     */

    void Send(Ptr<Packet> packet, Ipv4Address saddr, Ipv4Address daddr, Ipv4Mask ipv4Mask, OspfHeader ospfHeader, int packetType, int currentState);

    void Send(Ptr<Packet> packet, Ipv4Address saddr, Ipv4Address daddr, Ipv4Mask ipv4Mask, OspfHeader ospfHeader, int packetType, int currentState, Ptr<Ipv4Route> route);

    void Send(Ptr<Packet> packet, Ipv6Address saddr, Ipv6Address daddr);

    void Send(Ptr<Packet> packet, Ipv6Address saddr, Ipv6Address daddr, Ptr<Ipv6Route> route);

    void ReceiveIcmp(Ipv4Address icmpSource,
                     uint8_t icmpTtl,
                     uint8_t icmpType,
                     uint8_t icmpCode,
                     uint32_t icmpInfo,
                     Ipv4Address payloadSource,
                     Ipv4Address payloadDestination,
                     const uint8_t payload[8]) override;

    void ReceiveIcmp(Ipv6Address icmpSource,
                     uint8_t icmpTtl,
                     uint8_t icmpType,
                     uint8_t icmpCode,
                     uint32_t icmpInfo,
                     Ipv6Address payloadSource,
                     Ipv6Address payloadDestination,
                     const uint8_t payload[8]) override;


    Ipv4EndPoint* Allocate(Ipv4Address address);

    void SetExclusions(std::set<uint32_t>);

    void startDownState();

    void SetIpv4(Ptr<Ipv4>);

  protected:

    /**
     * Notify all Objects aggregated to this one of a new Object being
     * aggregated.
     *
     * This method is invoked whenever two sets of Objects are aggregated
     * together.  It is invoked exactly once for each Object in both sets.
     * This method can be overridden by subclasses who wish to be notified
     * of aggregation events. These subclasses must chain up to their
     * base class NotifyNewAggregate() method.
     *
     * It is safe to call GetObject() and AggregateObject() from within
     * ththeis method.
     */
    void NotifyNewAggregate() override;

    //void DoInitialize() override;
    /**
     * Initialize() implementation.
     *
     * This method is called only once by Initialize(). If the user
     * calls Initialize() multiple times, DoInitialize() is called only the
     * first time.
     *
     * Subclasses are expected to override this method and chain up
     * to their parent's implementation once they are done. It is
     * safe to call GetObject() and AggregateObject() from within this method.
     */
    void DoDispose() override;

    void SendDownPacket(Ipv4InterfaceAddress);

    void SendInitPacket(Ipv4InterfaceAddress, Ipv4Address);

    void SendTwoWayPacket(Ipv4InterfaceAddress, Ipv4Address);

    void HandleDownResponse(Ptr<Packet>, Ipv4Header, OspfHeader, Ptr<Ipv4Interface>, uint32_t);

    void HandleInitResponse(Ptr<Packet>, Ipv4Header, OspfHeader, Ptr<Ipv4Interface>, uint32_t);

    void HandleTwoWayResponse(Ptr<Packet>, Ipv4Header, OspfHeader, Ptr<Ipv4Interface>, uint32_t);

  private:
    Ptr<Node> m_node;                    //!< The node this stack is associated with
    Ipv4EndPointDemux* m_endPoints;      //!< A list of IPv4 end points.
    Ipv6EndPointDemux* m_endPoints6;     //!< A list of IPv6 end points.

    IpL4Protocol::DownTargetCallback m_downTarget;   //!< Callback to send packets over IPv4
    IpL4Protocol::DownTargetCallback6 m_downTarget6; //!< Callback to send packets over IPv6

    Ptr<Ipv4> m_ipv4;
    std::set<uint32_t> m_interfaceExclusions;
    OspfNeighborTable m_neighbor_table;
    uint32_t m_routerId;
    int m_areaId;
};

}

#endif // OSPF_L4_PROTOCOL_H
