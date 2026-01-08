#pragma once

namespace Container
{
	// Copy List To Vector
	template<typename T>
	std::vector<T> ListToVector(const std::list<T>& lst) {
		return std::vector<T>(lst.begin(), lst.end());
	}

	// Copy Vector To List
	template<typename T>
	std::list<T> VectorToList(const std::vector<T>& vec) {
		return std::list<T>(vec.begin(), vec.end());
	}

	// Inversion Vector
	template<typename T>
	void ReverseVector(std::vector<T>& vec) {
		std::reverse(vec.begin(), vec.end());
	}

	// Inversion List
	template<typename T>
	void ReverseList(std::list<T>& lst) {
		lst.reverse();
	}

	// Copy Map To Vector
	template <typename K, typename V, typename Hash = std::hash<K>, typename KeyEqual = std::equal_to<K>>
	std::vector<K> MapFirstToVector(const std::unordered_map<K, V, Hash, KeyEqual>& map) {
		std::vector<K> result;
		result.reserve(map.size());

		for (const auto& pair : map) {
			result.push_back(pair.first);
		}

		return result;
	}

	// Copy Map To Vector
	template <typename K, typename V, typename Hash = std::hash<K>, typename KeyEqual = std::equal_to<K>>
	std::list<K> MapFirstToList(const std::unordered_map<K, V, Hash, KeyEqual>& map) {
		std::list<K> result;

		for (const auto& pair : map) {
			result.push_back(pair.first);
		}

		return result;
	}

	// Copy Map To Vector
	template <typename K, typename V, typename Hash = std::hash<K>, typename KeyEqual = std::equal_to<K>>
	std::vector<V> MapSecondToVector(const std::unordered_map<K, V, Hash, KeyEqual>& map) {
		std::vector<V> result;
		result.reserve(map.size());

		for (const auto& pair : map)
			result.push_back(pair.second);

		return result;
	}

	// Copy Map To List
	template <typename K, typename V, typename Hash = std::hash<K>, typename KeyEqual = std::equal_to<K>>
	std::list<V> MapSecondToList(const std::unordered_map<K, V, Hash, KeyEqual>& map) {
		std::list<V> result;

		for (const auto& pair : map)
			result.push_back(pair.second);

		return result;
	}
}