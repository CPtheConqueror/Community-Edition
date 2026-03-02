#pragma once
#include "..\..\..\Minecraft.World\Packet.h"
#include "P2PConnectionManager.h"
#include "NetworkPlayerInterface.h"

// Packet routing decision
enum ERouteDecision
{
	ROUTE_VIA_HOST = 0,		// Send through host TCP (existing path)
	ROUTE_VIA_P2P,			// Send directly via P2P UDP
	ROUTE_VIA_BOTH			// Send via both paths (redundancy for critical packets)
};

class PacketRouter
{
public:
	PacketRouter();

	// Initialize with P2P manager reference
	void Initialize(IP2PConnectionManager* p2pManager);
	void Shutdown();

	// Determine how to route a packet to a specific player
	ERouteDecision GetRouteDecision(shared_ptr<Packet> packet, INetworkPlayer* targetPlayer);

	// Route a serialized packet - returns true if sent via P2P
	bool RoutePacket(shared_ptr<Packet> packet, INetworkPlayer* targetPlayer,
		const void* serializedData, int dataSize);

	// Enable/disable P2P routing (global toggle)
	void SetP2PEnabled(bool enabled) { m_p2pEnabled = enabled; }
	bool IsP2PEnabled() const { return m_p2pEnabled; }

	// Map packet IDs to P2P channels
	static EP2PChannel GetChannelForPacket(int packetId);

	// Stats
	int GetP2PRoutedCount() const { return m_p2pRoutedCount; }
	int GetHostRoutedCount() const { return m_hostRoutedCount; }
	void ResetStats() { m_p2pRoutedCount = 0; m_hostRoutedCount = 0; }

private:
	IP2PConnectionManager* m_p2pManager;
	bool m_p2pEnabled;
	int m_p2pRoutedCount;
	int m_hostRoutedCount;
};

extern PacketRouter g_PacketRouter;
