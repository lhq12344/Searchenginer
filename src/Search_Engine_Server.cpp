#include "../include/Search_Engine_Server.h"
#include "../include/Suggestserver.h"
#include <ppconsul/ppconsul.h>
#include <string>

using namespace wfrest;
using json = nlohmann::json;
using std::cout;
using std::endl;
using std::string;

void SearchEngineServer::start(unsigned short port)
{

	if (_httpserver.track().start(port) == 0)
	{
		_httpserver.list_routes();
		_waitGroup.wait();
		_httpserver.stop();
	}
	else
	{
		printf("SearchEngine Server Start Failed!\n");
	}
}

void SearchEngineServer::loadModules()
{
	loadStaticResourceModule();
	loadKeywordSearchModule();
	loadWebPageSearchModule();
}

// ============= 静态资源模块 =============
void SearchEngineServer::loadStaticResourceModule()
{
	// 以及根路径返回首页
	_httpserver.GET("/", [](const HttpReq *req, HttpResp *resp)
					{ resp->File("../static/index.html"); });
}

// 关键词搜索功能
void SearchEngineServer::loadKeywordSearchModule()
{
	_httpserver.POST("/api/suggest", [](const HttpReq *req, HttpResp *resp,
										SeriesWork *series)
					 {
						if (req->content_type() != APPLICATION_URLENCODED) {
						resp->String("Content-Type must be application/x-www-form-urlencoded");
						return;
						}
						// 从consul中获取服务的ip&port
						std::string url = "http://192.168.149.128:8500/v1/agent/services";
						std::string ip;
						unsigned short port;
						WFHttpTask *consultask = WFTaskFactory::create_http_task(
							url, 0, 3,
							std::bind(ConsulWordSearchCallback, std::placeholders::_1, std::ref(ip),
									std::ref(port), req, resp, series));
						series->push_back(consultask); });
}
