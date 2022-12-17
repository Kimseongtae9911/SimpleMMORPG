#include "pch.h"
#include "CEffect.h"

CEffect::CEffect(string filename, float tba, int fa)
{
}

CEffect::~CEffect()
{
}

void CEffect::Update(float ElapsdTime)
{
	m_spriteLeft = m_spriteLeft + m_frame_action * m_action_time * ElapsdTime;

	int FrameStart = static_cast<int>(m_spriteLeft) % m_frame_action;

	m_sprite.setTextureRect(sf::IntRect(FrameStart * 32, 0, TILE_WIDTH, TILE_WIDTH));
}

CAttackEffect::CAttackEffect(string filename, float tba, int fa)
{
	m_texture.loadFromFile(filename);
	m_sprite.setTexture(m_texture);
	m_sprite.setTextureRect({ 0, 0, TILE_WIDTH, TILE_WIDTH });

	m_time_by_action = tba;
	m_action_time = 1.0f / tba;
	m_frame_action = fa;

	m_spriteLeft = 0;
	m_enable = false;
}

void CAttackEffect::Render(sf::RenderWindow& RW, int x, int y, int left, int top, DIR dir)
{
	float rx = (x - left) * 32.f + 8;
	float ry = (y - top) * 32.f + 8;

	m_sprite.setPosition(sf::Vector2f(rx - TILE_WIDTH, ry));
	RW.draw(m_sprite);
	m_sprite.setPosition(sf::Vector2f(rx + TILE_WIDTH, ry ));
	RW.draw(m_sprite);
	m_sprite.setPosition(sf::Vector2f(rx, ry - TILE_WIDTH));
	RW.draw(m_sprite);
	m_sprite.setPosition(sf::Vector2f(rx, ry + TILE_WIDTH));
	RW.draw(m_sprite);

	if (static_cast<int>(m_spriteLeft) % m_frame_action == m_frame_action - 1) {
		m_enable = false;
		m_spriteLeft = 0;
	}
}

CSkillEffect1::CSkillEffect1(string filename, float tba, int fa)
{
	m_texture.loadFromFile(filename);
	m_sprite.setTexture(m_texture);
	m_sprite.setTextureRect({ 0, 0, TILE_WIDTH, TILE_WIDTH });

	m_time_by_action = tba;
	m_action_time = 1.0f / tba;
	m_frame_action = fa;

	m_spriteLeft = 0;
	m_enable = false;
}

void CSkillEffect1::Render(sf::RenderWindow& RW, int x, int y, int left, int top, DIR dir)
{
	float rx = (x - left) * 32.f + 8;
	float ry = (y - top) * 32.f + 8;
	m_sprite.setPosition(sf::Vector2f(rx, ry));
	RW.draw(m_sprite);
}

CSkillEffect2::CSkillEffect2(string filename, float tba, int fa)
{
	m_texture.loadFromFile(filename);
	m_sprite.setTexture(m_texture);
	m_sprite.setTextureRect({ 0, 0, TILE_WIDTH, TILE_WIDTH });

	m_time_by_action = tba;
	m_action_time = 1.0f / tba;
	m_frame_action = fa;

	m_spriteLeft = 0;
	m_enable = false;
}

void CSkillEffect2::Render(sf::RenderWindow& RW, int x, int y, int left, int top, DIR dir)
{
	for (int i = 1; i < 5; ++i) {
		switch (dir) {
		case DIR::DOWN: {
			float rx = (x - left) * 32.f + 8;
			float ry = (y - top + i) * 32.f + 8;
			m_sprite.setPosition(sf::Vector2f(rx, ry));
			RW.draw(m_sprite);
			break;
		}	
		case DIR::UP: {
			float rx = (x - left) * 32.f + 8;
			float ry = (y - top - i) * 32.f + 8;
			m_sprite.setPosition(sf::Vector2f(rx, ry));
			RW.draw(m_sprite);
			break;
		}
		case DIR::LEFT: {
			float rx = (x - left - i) * 32.f + 8;
			float ry = (y - top) * 32.f + 8;
			m_sprite.setPosition(sf::Vector2f(rx, ry));
			RW.draw(m_sprite);
			break;
		}
		case DIR::RIGHT: {
			float rx = (x - left + i) * 32.f + 8;
			float ry = (y - top) * 32.f + 8;
			m_sprite.setPosition(sf::Vector2f(rx, ry));
			RW.draw(m_sprite);
			break;
		}
		}
	}

	if (static_cast<int>(m_spriteLeft) % m_frame_action == m_frame_action - 1 ||
		(static_cast<int>(m_spriteLeft) % m_frame_action == 0 && m_spriteLeft >= 0.f + FLT_EPSILON)) {
		m_enable = false;
		m_spriteLeft = 0;
	}
}

CSkillEffect3::CSkillEffect3(string filename, float tba, int fa)
{
	m_texture.loadFromFile(filename);
	m_sprite.setTexture(m_texture);
	m_sprite.setTextureRect({ 0, 0, TILE_WIDTH, TILE_WIDTH });

	m_time_by_action = tba;
	m_action_time = 1.0f / tba;
	m_frame_action = fa;

	m_spriteLeft = 0.f;
	m_enable = false;
}

void CSkillEffect3::Render(sf::RenderWindow& RW, int x, int y, int left, int top, DIR dir)
{
	for (int i = -1; i < 4; ++i) {
		for (int j = -2; j < 3; ++j) {
			float rx = (x - left + i) * 32.f + 8;
			float ry = (y - top + j) * 32.f + 8;
			m_sprite.setPosition(sf::Vector2f(rx - TILE_WIDTH, ry));
			RW.draw(m_sprite);
		}
	}

	if (static_cast<int>(m_spriteLeft) % m_frame_action == m_frame_action - 1 || 
		(static_cast<int>(m_spriteLeft) % m_frame_action == 0 && m_spriteLeft >= 0.f + FLT_EPSILON)) {
		m_enable = false;
		m_spriteLeft = 0.f;
	}
}