#ifndef SERVER_HANDLERS_JSON_UTILS_H
#define SERVER_HANDLERS_JSON_UTILS_H

#include <string>

// 从字符串中提取第一个完整的 JSON 值（以 '{' 或 '[' 开始），
// 会匹配闭合括号并跳过字符串内的转义。返回空字符串表示未找到完整 JSON。
std::string extract_first_json(const std::string &s);

#endif // SERVER_HANDLERS_JSON_UTILS_H
