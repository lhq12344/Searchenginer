# Searchenginer 搜索引擎系统

[![Language](https://img.shields.io/badge/language-C%2B%2B-blue)](#) [![Build](https://img.shields.io/badge/build-make-green)](#) [![License](https://img.shields.io/badge/license-MIT-lightgrey)](#)

> 一个面向中文与英文的搜索引擎实验项目，涵盖关键词推荐与网页搜索两大链路，并引入 **FAISS 向量检索**以增强语义检索能力。

---

## ✨ 项目亮点

- **端到端搜索系统**：从离线构建词典/索引到在线检索服务一体化实现。
- **关键词推荐**：基于最短编辑距离与分词索引，提供高相似度纠错与联想。
- **网页检索**：TF‑IDF + 余弦相似度的向量空间模型进行相关性排序。
- **网页去重**：利用 SimHash 进行文本相似度判定。
- **缓存优化**：实现 LRU 缓存，降低磁盘读取开销。
- **倒排索引升级**：探索 LSM Tree 结构，改善 I/O 与批量写入性能。
- **FAISS 引入**：结合 FastText 生成向量，进行语义相似度检索。

---

## 🧰 工具链

| 类型 | 技术栈 |
| --- | --- |
| 语言 | C++ |
| 构建 | Make / Makefile |
| 分词 | Jieba（中文分词） |
| 相似度 | SimHash / 最短编辑距离 |
| 向量化 | TF‑IDF / FastText |
| 数据库 | FAISS（向量检索） |
| RPC 框架 | srpc |

---

## ✅ 功能说明

### 1. 关键词推荐（自动补全 + 拼写纠错）
- 离线构建 **词典 + 索引**（字典 + 字典索引）。
- 在线服务读取索引至内存，使用分词定位候选词。
- 基于最短编辑距离，从候选中选取相似度最高的 Top N 结果。

### 2. 网页搜索（关键词检索）
- 离线创建网页库与偏移库，并进行清洗与去重。
- 在线检索：
  1. 对查询进行分词并计算权重。
  2. 在倒排索引中求交集。
  3. 使用 TF‑IDF + 余弦相似度排序。

### 3. 语义检索（FAISS）
- 使用 FastText 模型对文档向量化，并在服务启动时将向量加载入 FAISS。
- 用户输入句子 → 分词 → FastText 向量化 → L2 归一化 → FAISS TopK 搜索。
- 语义检索为补充检索方式，可与关键词检索结合提升召回率。

---

## 🔧 功能实现概览

### 关键词推荐核心流程
1. 离线生成词典与索引。
2. 在线服务启动后加载索引入内存。
3. 使用分词器拆分输入词组。
4. 汇总候选词并去重。
5. 最短编辑距离排序，输出 Top N。

### 网页搜索核心流程
1. 网页清洗 + SimHash 去重。
2. 构建倒排索引与网页偏移库。
3. 查询分词，计算权重。
4. 查找交集并排序。
5. 返回结果摘要。

### FAISS 语义检索流程
1. 训练并加载 FastText 模型。
2. 对网页文档进行向量化与归一化。
3. 将向量写入 FAISS 索引结构。
4. 查询阶段向量化后进行 TopK 语义检索。

---

## 📂 文件夹结构说明

```
Searchenginer/
├── Make_Page/        # 网页抓取、清洗与离线处理
├── Make_dir/         # 词典构建与索引生成
├── bin/              # 生成的可执行文件
├── build/            # 构建中间产物
├── include/          # 头文件
├── log/              # 日志文件
├── scripts/          # 脚本工具
├── src/              # 核心代码
├── static/           # 静态资源
├── srpc/             # RPC 配置与相关文件
├── README.txt        # 原始项目说明文档
└── README.md         # Github 项目展示说明（本文件）
```

---

## 🚀 快速开始（示例）

```bash
# 编译
make

# 启动服务（根据项目脚本调整）
./bin/your_server_binary
```

---

## 📌 项目特色与优化方向

- **语义检索提升**：FAISS 可进一步结合 TF‑IDF 或更强的向量模型提高准确率。
- **索引结构优化**：LSM Tree 适合批量写入和冷热数据分层。
- **缓存优化**：LRU 减少磁盘随机访问，提高响应性能。

---

## 📖 参考灵感

- [Elasticsearch](https://github.com/elastic/elasticsearch)
- [Faiss](https://github.com/facebookresearch/faiss)
- [Lucene](https://github.com/apache/lucene)

---

## 📄 License

本项目仅用于学习与研究用途。
