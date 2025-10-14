整个搜索引擎分为两大部分：关键词推荐和网页查询服务。
一.关键词推荐
	1.离线部分：创建汉字和英文词典和词典索引存放在data文件夹中
				细节：要得到词频并且去掉垃圾词
	2.在线部分：开启服务器后先读取字典和索引文件储存到内存
				字典存储类型为vector<pair<string,int>>
				字典索引类型为unorder_map<string,set<int>>
			   wf作为api网关，将客户端的请求发送给对应的微服务（关键词和网页）
			   	关键词微服务接收到用户请求后，使用分词将请求分词后在对应索引里查找
				将所有查询结果汇总去重
				接下来使用最短编辑距离算法找到最大相似度的n个结果并且返回json

				srpc微服务指令	：srpc_generator protobuf wordsearch.proto . 
								protoc -I. --cpp_out=. pagesearch.proto 2>&1 | sed -n '1,200p'
二.网页查询
	1.离线部分：创建网页搜索库和网页偏移库
				细节：网页清洗
					网页去重（simhasher）
	2.在线部分：

	========================
	前端静态页面与 API 规范
	========================

	静态页面：static/index.html
	直接在浏览器打开或由服务端映射为静态资源目录。

	1) 关键词推荐 API
		 方法:POST /api/suggest?q=<string>&n=<int>
		 返回:
		 {
			 "code": 0,
			 "msg": "ok",
			 "data": {
				 "query": "原始查询词",
				 "suggestions": [
					 { "word": "建议词", "freq": 123, "score": 0.87 }
				 ]
			 }
		 }
		 失败:
		 { "code": 非0, "msg": "错误信息" }

	2) 网页查询 API
		 方法: POST /api/search?q=<string>&page=<int>&size=<int>
		 返回:
		 {
			 "code": 0,
			 "msg": "ok",
			 "data": {
				 "query": "原始查询词",
				 "total": 100,
				 "page": 1,
				 "size": 10,
				 "items": [
					 { "title": "标题", "url": "https://...", "snippet": "摘要", "score": 0.92 }
				 ]
			 }
		 }
		 失败:
		 { "code": 非0, "msg": "错误信息" }

	返回 JSON 格式统一：UTF-8，无 BOM，Content-Type: application/json; charset=utf-8。




