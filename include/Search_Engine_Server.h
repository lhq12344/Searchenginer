#ifndef SEARCH_ENGINE_SERVER_H
#define SEARCH_ENGINE_SERVER_H

#include <nlohmann/json.hpp>
#include <wfrest/HttpServer.h>
#include <workflow/MySQLMessage.h>
#include <workflow/MySQLResult.h>
#include <workflow/WFFacilities.h>

class SearchEngineServer
{
public:
	SearchEngineServer(int cnt)
		: _waitGroup(cnt) {};

	~SearchEngineServer() = default;

	void start(unsigned short port);

	void loadModules();

private:
	// 模块化的思维方式写代码
	// 静态资源访问功能
	void loadStaticResourceModule();
	// 关键词搜索功能
	void loadKeywordSearchModule();
	// 网页搜索功能
	void loadWebPageSearchModule();

private:
	WFFacilities::WaitGroup _waitGroup; // 阻塞主线程
	wfrest::HttpServer _httpserver;
};
#endif // !SEARCH_ENGINE_SERVER_H