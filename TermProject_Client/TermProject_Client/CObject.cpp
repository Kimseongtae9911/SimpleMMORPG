#include "pch.h"
#include "CObject.h"

extern sf::Font g_font;

CObject::CObject(sf::Texture& t, int x, int y, int x2, int y2)
{
	m_showing = false;
	m_sprite.setTexture(t);
	m_sprite.setTextureRect(sf::IntRect(x, y, x2, y2));
	//set_name("-1");
	m_mess_end_time = chrono::system_clock::now();
}

void CMoveObject::draw(sf::RenderWindow& RW, int left, int top)
{
	if (false == m_showing)
		return;
	float rx = (m_x - left) * 32.f + 8;
	float ry = (m_y - top) * 32.f + 8;
	m_sprite.setPosition(rx, ry);
	RW.draw(m_sprite);
	auto size = m_name.getGlobalBounds();

	if (m_mess_end_time < chrono::system_clock::now()) {
		m_name.setPosition(rx + 16.f - size.width / 2, ry - 10);
		RW.draw(m_name);
	}
	else {
		m_chat.setPosition(rx + 16.f - size.width / 2, ry - 10);
		RW.draw(m_chat);
	}
}

void CMoveObject::set_name(const char str[])
{
	m_name.setFont(g_font);
	m_name.setString(str);
	m_name.setCharacterSize(15);
	if (m_id < MAX_USER)
		m_name.setFillColor(sf::Color(255, 255, 255));
	else
		m_name.setFillColor(sf::Color(255, 255, 0));
	//m_name.setStyle(sf::Text::Bold);
}

void CMoveObject::set_chat(const char str[])
{
	m_chat.setFont(g_font);
	m_chat.setString(str);
	m_chat.setFillColor(sf::Color(255, 255, 255));
	m_chat.setStyle(sf::Text::Bold);
	m_mess_end_time = chrono::system_clock::now() + chrono::seconds(3);
}

CPlayer::CPlayer(sf::Texture& t, int x, int y, int x2, int y2)
{
	m_showing = false;
	m_sprite.setTexture(t);
	m_sprite.setTextureRect(sf::IntRect(x, y, x2, y2));
	//set_name("-1");
	m_mess_end_time = chrono::system_clock::now();
}

CNpc::CNpc(sf::Texture& t, int x, int y, int x2, int y2)
{
}
