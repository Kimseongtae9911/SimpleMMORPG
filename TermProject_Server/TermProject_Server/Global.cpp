#include "pch.h"

#include "JobQueue.h"

PacketJobQueue* GPacketJobQueue = nullptr;
std::atomic_int GActiveNpc = 0;

class Global
{
public:
	Global()
	{
		GPacketJobQueue = new PacketJobQueue();
	}

	~Global()
	{
		delete GPacketJobQueue;
	}
} Global;