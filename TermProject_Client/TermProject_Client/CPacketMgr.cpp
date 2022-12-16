#include "pch.h"
#include "CPacketMgr.h"
#include "CScene.h"

CPacketMgr::CPacketMgr(const char* ip, shared_ptr<CScene> scene)
{
	m_scene = scene;
	wcout.imbue(locale("korean"));
	sf::Socket::Status status = m_sock.connect(ip, PORT_NUM);
	m_sock.setBlocking(false);

	if (status != sf::Socket::Done) {
		wcout << L"서버와 연결할 수 없습니다.\n";
		while (true);
	}
}

CPacketMgr::~CPacketMgr()
{
}

void CPacketMgr::SendPacket(void* packet)
{
	unsigned char* p = reinterpret_cast<unsigned char*>(packet);
	size_t sent = 0;
	m_sock.send(packet, p[0], sent);
}

void CPacketMgr::RecvPacket()
{
	char net_buf[BUF_SIZE];
	size_t	received;

	auto recv_result = m_sock.receive(net_buf, BUF_SIZE, received);
	if (recv_result == sf::Socket::Error)
	{
		wcout << L"Recv 에러!";
		while (true);
	}

	if (recv_result != sf::Socket::NotReady)
		if (received > 0)
			PacketReasemble(net_buf, received);
}

void CPacketMgr::PacketReasemble(char* net_buf, size_t io_byte)
{
	char* ptr = net_buf;
	static size_t in_packet_size = 0;
	static size_t saved_packet_size = 0;
	static char packet_buffer[BUF_SIZE];

	while (0 != io_byte) {
		if (0 == in_packet_size) in_packet_size = ptr[0];
		if (io_byte + saved_packet_size >= in_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			ptr += in_packet_size - saved_packet_size;
			io_byte -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
			saved_packet_size += io_byte;
			io_byte = 0;
		}
	}
}

void CPacketMgr::ProcessPacket(char* ptr)
{
	static bool first_time = true;
	switch (ptr[1])
	{
	case SC_LOGIN_INFO:
	{
		m_scene->ProcessLoginInfoPacket(ptr);
		break;
	}
	case SC_LOGIN_FAIL:
		break;
	case SC_ADD_OBJECT:
	{
		m_scene->ProcessAddObjectPacket(ptr);
		break;
	}
	case SC_MOVE_OBJECT:
	{
		m_scene->ProcessMoveObjectPacket(ptr);
		break;
	}

	case SC_REMOVE_OBJECT:
	{
		m_scene->ProcessRemoveObjectPacket(ptr);
		break;
	}
	case SC_CHAT:
	{
		m_scene->ProcessChatPacket(ptr);
		break;
	}
	default:
		printf("Unknown PACKET type [%d]\n", ptr[1]);
	}
}
