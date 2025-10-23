#ifndef CACHE_H
#define CACHE_H
#include <unordered_map>
#include <list>
#include <mutex>
#include <memory>
#include <optional>

// 线程安全 LRU 缓存，支持 key-value 并发访问
// Key: 任意可哈希类型，Value: 任意类型（推荐用智能指针）

template <typename Key, typename Value>
class MyLRUCache
{
public:
	using ListIt = typename std::list<std::pair<Key, std::set<Value>>>::iterator;

	MyLRUCache(size_t capacity) : capacity_(capacity) {}

	// 获取缓存，命中则返回 value，miss 返回 std::nullopt
	std::optional<std::set<Value>> get(const Key &key)
	{
		std::lock_guard<std::mutex> lock(mtx_);
		auto it = map_.find(key);
		if (it == map_.end())
			return std::nullopt;
		lru_.splice(lru_.begin(), lru_, it->second); // 将lru_列表中由it->second指向的元素移动到列表的开头位置。
		return it->second->second;
	}

	// 插入/更新缓存
	void put(const Key &key, const Value &value)
	{
		std::lock_guard<std::mutex> lock(mtx_);
		auto it = map_.find(key);
		if (it != map_.end())
		{
			it->second->second.insert(value);
			lru_.splice(lru_.begin(), lru_, it->second);
		}
		else
		{
			lru_.emplace_front(key, std::set<Value>{value});
			map_[key] = lru_.begin();
			if (map_.size() > capacity_)
			{
				auto last = lru_.end();
				--last;
				map_.erase(last->first);
				lru_.pop_back();
			}
		}
	}

	// 删除缓存
	void erase(const Key &key)
	{
		std::lock_guard<std::mutex> lock(mtx_);
		auto it = map_.find(key);
		if (it != map_.end())
		{
			lru_.erase(it->second);
			map_.erase(it);
		}
	}

	// 清空缓存
	void clear()
	{
		std::lock_guard<std::mutex> lock(mtx_);
		lru_.clear();
		map_.clear();
	}

	// 当前缓存条目数
	size_t size() const
	{
		std::lock_guard<std::mutex> lock(mtx_);
		return map_.size();
	}

private:
	size_t capacity_;
	mutable std::mutex mtx_;
	std::list<std::pair<Key, set<Value>>> lru_; // 头部为最近使用
	std::unordered_map<Key, ListIt> map_;
};

#endif // CACHE_H
