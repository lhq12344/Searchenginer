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

// Note: do NOT pass stack references (ip/port) into the async callback.
// The callback will parse consul's response and create its own local ip/port.
void ConsulWordSearchCallback(WFHttpTask *task, const HttpReq *req,
							  HttpResp *resp, SeriesWork *series);
#endif // !SUGGESTSERVER_H