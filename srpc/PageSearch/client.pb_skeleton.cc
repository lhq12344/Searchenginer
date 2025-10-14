#include "pagesearch.srpc.h"
#include "workflow/WFFacilities.h"

using namespace srpc;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
	wait_group.done();
}

static void pagesuggest_done(RespPageSuggest *response, srpc::RPCContext *context)
{
}

int main()
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	const char *ip = "127.0.0.1";
	unsigned short port = 1412;

	PageSuggestServer::SRPCClient client(ip, port);

	// example for RPC method call
	ReqPageSuggest pagesuggest_req;
	//pagesuggest_req.set_message("Hello, srpc!");
	client.PageSuggest(&pagesuggest_req, pagesuggest_done);

	wait_group.wait();
	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}
