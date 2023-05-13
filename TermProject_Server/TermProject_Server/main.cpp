#include "pch.h"
#include "CServer.h"

int main()
{
	CServer server;
	if (false == server.Initialize()) {
		cout << "Server Initialize Fail" << endl;
	}

	server.Run();

	if (false == server.Release()) {
		cout << "Server Release Fail" << endl;
	}
}