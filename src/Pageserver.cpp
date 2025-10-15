#include "../include/Pageserver.h"

#include "../include/Utils.h"

void pageSuggestCallback(RespPageSuggest *response, srpc::RPCContext *ctx, HttpResp *resp,
						 ReqPageSuggest *word_suggest_req)
{

	if (response->code() == 0)
	{
		// 将 repeated backwords 字段组织为 JSON 返回给前端
		json j;
		j["code"] = 0;
		j["message"] = response->message();
		j["items"] = json::array();
		for (int i = 0; i < response->itemproto_size(); ++i)
		{
			const auto &item = response->itemproto(i);

			// 手动转换成 JSON 对象
			json item_json = {
				{"title", item.title()},
				{"link", item.link()},
				{"description", item.description()}};

			j["items"].push_back(item_json);
		}

		resp->String(j.dump());
	}
	else
	{
		json j;
		j["code"] = response->code();
		j["message"] = response->message();
	}

	// 清理申请的资源
	delete word_suggest_req;
}

// 访问CONSUL获取服务信息
void ConsulPageSearchCallback(WFHttpTask *task, const HttpReq *req,
							  HttpResp *resp, SeriesWork *series)
{
	auto consul_resp = task->get_resp();
	if (task->get_state() == WFT_STATE_SUCCESS)
	{
		const void *raw_body = nullptr;
		size_t body_size = 0;

		if (consul_resp->get_parsed_body(&raw_body, &body_size))
		{
			// 将 body 转成 std::string
			std::string body(static_cast<const char *>(raw_body), body_size);
			std::string json_candidate = extract_first_json(body);
			try
			{
				// 解析 JSON
				auto j = json::parse(json_candidate);
				// Consul 的返回格式是一个对象，key 是服务名

				std::string ip;
				unsigned short port = 0;
				for (auto &[serviceName, serviceInfo] : j.items())
				{
					if (serviceName == "PageSuggestService1" &&
						serviceInfo.contains("Address") && serviceInfo.contains("Port"))
					{
						ip = serviceInfo["Address"].get<std::string>();
						port = static_cast<unsigned short>(serviceInfo["Port"].get<int>());
						break; // 如果只取一个，找到后跳出
					}
				}
				if (ip.empty() || port == 0)
				{
					std::cerr << "PageSuggestService1 not found in consul response"
							  << std::endl;
					resp->String("FAILED");
					return;
				}

				std::cout << "IP: " << ip << ", Port: " << port << std::endl;
				auto formKV = req->form_kv();
				std::string urlsearchstring = formKV["searchstring"];
				std::string searchstring = urlDecode(urlsearchstring);
				if (searchstring.empty())
				{
					resp->String("FAILED: searchstring is empty");
					return;
				}
				std::cout << "searchstring: " << searchstring.c_str() << std::endl;
				GOOGLE_PROTOBUF_VERIFY_VERSION;
				// 1) 建立 SRPC Client（放到堆或让 task 管理其生命周期）
				auto client =
					std::make_shared<PageSuggestServer::SRPCClient>(ip.c_str(), (int)port);

				// 2) 组装请求
				auto *page_suggest_req = new ReqPageSuggest();
				page_suggest_req->set_searchstring(searchstring);

				// 3) 创建一个 SRPC 任务，并把它挂到当前 HTTP 的 series 上
				auto *task = client->create_PageSuggest_task(
					std::bind(pageSuggestCallback, std::placeholders::_1,
							  std::placeholders::_2, resp, page_suggest_req));
				task->serialize_input(page_suggest_req);

				// 4) 把 SRPC 任务加入本次 HTTP 的序列 —— 关键！
				series->push_back(task);
			}
			catch (const std::exception &e)
			{
				std::cerr << "JSON parse error: " << e.what() << std::endl;
				resp->String("FAILED");
			}
		}
	}
	else
	{
		std::cerr << "HTTP request failed, state=" << task->get_state()
				  << std::endl;
		resp->String("FAILED");
	}
}