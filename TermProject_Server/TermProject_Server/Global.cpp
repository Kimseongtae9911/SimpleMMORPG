#include "pch.h"

#include "JobQueue.h"

PacketJobQueue* GPacketJobQueue = nullptr;

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