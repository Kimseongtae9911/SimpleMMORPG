#include "pch.h"
#include "JobQueue.h"
#include "CClient.h"

class PacketJobQueue::Impl {
public:
    concurrency::concurrent_priority_queue<CClient*, CClientTimeComparer> m_jobQueue;
};

PacketJobQueue::PacketJobQueue() {
    std::cout << "JobQueue Init" << std::endl;
    m_impl = new Impl();
}

PacketJobQueue::~PacketJobQueue() {
    delete m_impl;
}

void PacketJobQueue::AddSessionQueue(CClient* object)
{
    if (object->TryMarkInQueue())
    {
        object->SetRegisterTime();
        m_jobQueue.push(object);
    }
}

void PacketJobQueue::ProcessJob()
{
    while (true)
    {
        CClient* client = nullptr;
        while (m_jobQueue.try_pop(client))
        {           
            if (client->IsDisconnected())
            {
                //CUserMgr::GetInstance()->ClientReset(client->GetID());
                continue;
            }

            client->GetJobQueue().ProcessJob();
            client->GetJobQueue().JobQueueSize() > 0 ? m_jobQueue.push(client) : client->UnmarkInQueue();

        }
    }
}
