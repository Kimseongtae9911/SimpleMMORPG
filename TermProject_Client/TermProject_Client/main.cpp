#define SFML_STATIC 1

#include "pch.h"
#include "CFramework.h"

sf::Font g_font;

int main()
{
	if (false == g_font.loadFromFile("cour.ttf")) {
		cout << "Font Loading Error!\n";
		exit(-1);
	}

	CFramework* client = new CFramework("127.0.0.1");

	client->Process();

	delete client;

	return 0;
}