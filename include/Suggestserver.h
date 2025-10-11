#ifndef SUGGESTSERVER_H
#define SUGGESTSERVER_H

#include <nlohmann/json.hpp>
#include <wfrest/HttpServer.h>
#include <workflow/MySQLMessage.h>
#include <workflow/MySQLResult.h>
#include <workflow/WFFacilities.h>
#include "../srpc/WordSearch/wordsearch.srpc.h"
#include "../include/json_utils.h"

using namespace wfrest;
using namespace protocol;
using json = nlohmann::json;

void ConsulWordSearchCallback(WFHttpTask *task, std::string &ip,
							  unsigned short &port, const HttpReq *req,
							  HttpResp *resp, SeriesWork *series);
#endif // !SUGGESTSERVER_H