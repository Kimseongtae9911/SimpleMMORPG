#include "pch.h"
#include "JobQueue.h"
#include "CClient.h"

void PacketJobQueue::AddSessionQueue(CClient* object)
{
    if (object->TryMarkInQueue())
        m_jobQueue.push(object);
}

void PacketJobQueue::ProcessJob()
{
    while (true)
    {
        CClient* client = nullptr;
        while (m_jobQueue.try_pop(client))
        {
            client->UnmarkInQueue();

            if (client->IsDisconnected())
            {
                //CUserMgr::GetInstance()->ClientReset(client->GetID());
                continue;
            }

            client->GetJobQueue().ProcessJob();
        }
    }
}
