#pragma once

class OVER_EXP {
public:
	WSAOVERLAPPED m_over;
	WSABUF m_Wsabuf;
	char m_send_buf[BUF_SIZE];
	OP_TYPE m_comp_type;
	int m_ai_target_obj;

	OVER_EXP()
	{
		m_Wsabuf.len = BUF_SIZE;
		m_Wsabuf.buf = m_send_buf;
		m_comp_type = OP_TYPE::OP_RECV;
		ZeroMemory(&m_over, sizeof(m_over));
	}
	OVER_EXP(char* packet)
	{
		m_Wsabuf.len = packet[0];
		m_Wsabuf.buf = m_send_buf;
		ZeroMemory(&m_over, sizeof(m_over));
		m_comp_type = OP_TYPE::OP_SEND;
		memcpy(m_send_buf, packet, packet[0]);
	}
};
