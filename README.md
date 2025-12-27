# Searchenginer

<p align="center">
  <strong>轻量级中文/英文搜索引擎演示项目</strong>
  <br />
  关键词联想 + 网页检索 + 微服务网关 + 离线索引构建
</p>

<p align="center">
  <img alt="language" src="https://img.shields.io/badge/language-C%2B%2B17-blue" />
  <img alt="build" src="https://img.shields.io/badge/build-make-success" />
  <img alt="server" src="https://img.shields.io/badge/server-wfrest%2Fworkflow-9cf" />
</p>

---

## ✨ 项目简介

Searchenginer 是一个面向教学与实践的搜索引擎原型，包含 **关键词推荐** 与 **网页搜索** 两大模块。项目使用 C++17 与 wfrest/workflow 构建服务端，离线生成词典、索引与网页库，在线通过微服务完成候选词检索与网页查询，并结合 **最短编辑距离**、**TF‑IDF + 余弦相似度** 等算法实现相似度排序。

> 适合用于：搜索引擎原理课程实践、分布式系统演示、后端工程能力训练。

---

## 🧰 工具链（Tech Stack）

- **语言与标准**：C++17
- **构建工具**：Makefile
- **HTTP 框架**：wfrest（基于 workflow）
- **序列化**：Protocol Buffers + SRPC
- **基础依赖**：OpenSSL、pthread、zlib
- **服务发现**：Consul（通过 HTTP 查询服务实例）
- **数据结构与算法**：
  - 分词/停用词处理
  - 最短编辑距离（Levenshtein）
  - 向量空间模型（TF‑IDF + 余弦相似度）
  - SimHash（网页去重）

---

## ✅ 功能说明

### 1. 关键词推荐（Suggest）
- **离线阶段**
  - 构建中英文词典与词频表
  - 生成倒排索引（字符 → 词表）
  - 过滤噪声词/停用词

- **在线阶段**
  - 网关接收请求后拆分关键词
  - 根据索引取候选集并去重
  - 通过最短编辑距离排序，返回 Top‑N 结果（JSON）

### 2. 网页搜索（Search）
- **离线阶段**
  - 抓取/清洗网页内容
  - 生成网页库 + 偏移库
  - SimHash 去重

- **在线阶段**
  - 对输入分词并计算权重
  - 取倒排索引交集获取候选网页
  - 计算 TF‑IDF 向量相似度进行排序

---

## 🏗️ 系统架构

```
          ┌────────────┐
          │   Client   │
          └─────┬──────┘
                │ HTTP
        ┌───────▼────────┐
        │  API Gateway   │  wfrest
        └───────┬────────┘
                │ Consul 服务发现
    ┌───────────▼───────────┐
    │   Suggest Microservice│
    └───────────┬───────────┘
                │
    ┌───────────▼───────────┐
    │   Search Microservice │
    └───────────────────────┘
```

---

## 🔌 API 说明（示例）

> 网关路由位于 `src/Search_Engine_Server.cpp`

| 方法 | 路径 | 说明 | Content‑Type |
|------|------|------|--------------|
| POST | `/api/suggest` | 关键词联想 | `application/x-www-form-urlencoded` |
| POST | `/api/search`  | 网页检索   | `application/x-www-form-urlencoded` |

---

## 📦 项目结构

```
Searchenginer/
├─ src/                 # 核心实现
│  ├─ Search_Engine_Server.cpp
│  ├─ Suggestserver.cpp
│  ├─ Pageserver.cpp
│  └─ Utils.cpp
├─ include/             # 头文件、数据结构、字典等
├─ srpc/                # SRPC 与 protobuf 生成文件
├─ static/              # 前端静态页面
├─ build/               # 构建产物
└─ README.md
```

---

## 🚀 快速开始

### 1. 编译

```bash
make
```

### 2. 运行

```bash
make run
```

### 3. 访问

- 首页：`http://localhost:PORT/`
- API：`/api/suggest`、`/api/search`

> `PORT` 由 `Search_Engine_Server` 启动时指定。

---

## 🔍 核心实现亮点

- **高性能索引结构**：`vector<pair<string,int>>` 词典 + `unordered_map<string, set<int>>` 索引
- **编辑距离排序**：以最短编辑距离衡量相似度，保障联想结果质量
- **向量空间模型**：使用 TF‑IDF + 余弦相似度排序网页结果
- **网页去重**：使用 SimHash 处理近似网页

---

## 📜 参考 & 致谢

- [wfrest](https://github.com/wfrest/wfrest)
- [workflow](https://github.com/sogou/workflow)
- [Protocol Buffers](https://developers.google.com/protocol-buffers)

---

## 📄 License

本项目仅用于教学与学习目的，可自由参考与扩展。
