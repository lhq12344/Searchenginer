#include "../include/Search_Engine_Server.h"

int main()
{
	SearchEngineServer server(1);
	server.loadModules();
	server.start(5679);
	return 0;
}