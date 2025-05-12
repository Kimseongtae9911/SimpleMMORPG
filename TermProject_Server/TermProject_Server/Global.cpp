#include "pch.h"

#include "JobQueue.h"

PacketJobQueue* GPacketJobQueue = new PacketJobQueue();
std::atomic_int GActiveNpc = 0;