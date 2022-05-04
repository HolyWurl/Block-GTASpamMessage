#pragma once

class ptr_manage {
public:
	ptr_manage(void* hand = 0);
	ptr_manage(std::uintptr_t hand = 0);

	template <typename T>
	inline std::enable_if_t<std::is_pointer<T>::value, T> as() {
		return static_cast<T>(m_ptr);
	}

	template <typename T>
	inline std::enable_if_t<std::is_lvalue_reference<T>::value, T> as() {
		return *static_cast<std::add_pointer_t<std::remove_reference_t<T>>>(m_ptr);
	}

	template <typename T>
	inline std::enable_if_t<std::is_same<T, std::uintptr_t>::value, T> as() {
		return reinterpret_cast<T>(m_ptr);
	}

	ptr_manage add(int offset);

	inline operator bool() { return m_ptr != nullptr; }
private:
	void* m_ptr;
};

class sModule {
public:
	sModule(HMODULE hMod);
	sModule(std::string name);

	ptr_manage get_begin();
	ptr_manage get_end();
	ptr_manage get_export(std::string proc_name);

private:
	ptr_manage m_begin;
	ptr_manage m_end;
	size_t m_size{};
	std::string m_name{};
};

class find_pattern {
public:
	struct Element {
		std::uint8_t m_data{};
		bool m_wildcard{};
		Element(uint8_t data, bool wildcard) : m_data(data), m_wildcard(wildcard) { }
	};
	find_pattern(const char* pattern);
	ptr_manage scan(sModule region = sModule(nullptr));

private:
	const char* m_pat;
	std::vector<Element> m_elements;
};

struct pattern_hisnt {
	std::string m_name;
	find_pattern m_pattern;
	std::function<void(ptr_manage)> m_callback;

	pattern_hisnt(std::string name, find_pattern pattern, std::function<void(ptr_manage)> callback);
};

class pattern_batch {
public:
	void add(std::string name, find_pattern pattern, std::function<void(ptr_manage)> callback);
	void run();
private:
	std::vector<pattern_hisnt> pattern_list;
};