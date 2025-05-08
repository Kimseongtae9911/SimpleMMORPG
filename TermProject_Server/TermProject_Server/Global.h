#pragma once

extern class PacketJobQueue* GPacketJobQueue;

#ifdef CHECK_NPC_ACTIVE
extern std::atomic_int GActiveNpc;
#endif