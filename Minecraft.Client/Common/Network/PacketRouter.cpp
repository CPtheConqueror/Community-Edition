#include "stdafx.h"
#include "PacketRouter.h"

PacketRouter g_PacketRouter;

PacketRouter::PacketRouter()
{
	m_p2pManager = NULL;
	m_p2pEnabled = false;
	m_p2pRoutedCount = 0;
	m_hostRoutedCount = 0;
}

void PacketRouter::Initialize(IP2PConnectionManager* p2pManager)
{
	m_p2pManager = p2pManager;
	m_p2pEnabled = (p2pManager != NULL);
}

void PacketRouter::Shutdown()
{
	m_p2pManager = NULL;
	m_p2pEnabled = false;
	ResetStats();
}

ERouteDecision PacketRouter::GetRouteDecision(shared_ptr<Packet> packet, INetworkPlayer* targetPlayer)
{
	// If P2P is disabled or no manager, always use host
	if (!m_p2pEnabled || m_p2pManager == NULL || targetPlayer == NULL)
		return ROUTE_VIA_HOST;

	// Local players don't need P2P
	if (targetPlayer->IsLocal())
		return ROUTE_VIA_HOST;

	int packetId = packet->getId();
	EPacketRoutingMode routingMode = Packet::GetRoutingMode(packetId);

	switch (routingMode)
	{
	case ROUTING_HOST_ONLY:
		return ROUTE_VIA_HOST;

	case ROUTING_PREFER_DIRECT:
		// Use P2P if we have a direct connection to this player
		if (m_p2pManager->IsDirectConnectionAvailable(targetPlayer))
			return ROUTE_VIA_P2P;
		return ROUTE_VIA_HOST;

	case ROUTING_DIRECT_ONLY:
		if (m_p2pManager->IsDirectConnectionAvailable(targetPlayer))
			return ROUTE_VIA_P2P;
		return ROUTE_VIA_HOST;  // Fallback to host if no direct link

	case ROUTING_BROADCAST:
		return ROUTE_VIA_HOST;  // Broadcasts always go through host

	default:
		return ROUTE_VIA_HOST;
	}
}

bool PacketRouter::RoutePacket(shared_ptr<Packet> packet, INetworkPlayer* targetPlayer,
	const void* serializedData, int dataSize)
{
	ERouteDecision decision = GetRouteDecision(packet, targetPlayer);

	if (decision == ROUTE_VIA_P2P && m_p2pManager != NULL)
	{
		EP2PChannel channel = GetChannelForPacket(packet->getId());
		m_p2pManager->SendDirect(targetPlayer, serializedData, dataSize, channel);
		m_p2pRoutedCount++;
		return true;
	}

	m_hostRoutedCount++;
	return false;  // Caller should use existing host path
}

EP2PChannel PacketRouter::GetChannelForPacket(int packetId)
{
	// Movement packets
	if ((packetId >= 10 && packetId <= 13) ||		// MovePlayerPacket variants
		(packetId >= 30 && packetId <= 34) ||		// MoveEntityPacket variants
		(packetId >= 162 && packetId <= 165))		// MoveEntityPacketSmall variants
	{
		return P2P_CHANNEL_MOVEMENT;
	}

	// Animation packets
	if (packetId == 18 || packetId == 35)			// AnimatePacket, RotateHeadPacket
	{
		return P2P_CHANNEL_ANIMATION;
	}

	// Effects/sounds
	if (packetId == 62)								// LevelSoundPacket
	{
		return P2P_CHANNEL_EFFECTS;
	}

	// Default
	return P2P_CHANNEL_MOVEMENT;
}
