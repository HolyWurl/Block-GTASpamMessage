#include "pch.h"
#include "sigscan.h"

ptr_manage::ptr_manage(void* hand) {
	m_ptr = hand;
}
ptr_manage::ptr_manage(std::uintptr_t hand) {
	m_ptr = reinterpret_cast<void*>(hand);
}

ptr_manage ptr_manage::add(int offset) {
	return ptr_manage(as<std::uintptr_t>() + offset);
}

sModule::sModule(HMODULE hMod) : m_begin(hMod), m_end(nullptr), m_size(0) {
	auto dosHeader = ptr_manage(m_begin).as<IMAGE_DOS_HEADER*>();
	auto ntHeader = ptr_manage(m_begin).add(dosHeader->e_lfanew).as<IMAGE_NT_HEADERS*>();
	m_size = ntHeader->OptionalHeader.SizeOfImage;
	m_end = ptr_manage(m_begin.add(m_size));
}

sModule::sModule(std::string name) : sModule(GetModuleHandleA(name.c_str())) { }

ptr_manage sModule::get_begin() {
	return m_begin;
}

ptr_manage sModule::get_end() {
	return m_end;
}

ptr_manage sModule::get_export(std::string proc_name) {
	return ptr_manage(GetProcAddress(m_begin.as<HMODULE>(), proc_name.c_str()));
}


find_pattern::find_pattern(const char* pattern) {
	auto toUpper = [](char c) -> char {
		return c >= 'a' && c <= 'z' ? static_cast<char>(c + ('A' - 'a')) : static_cast<char>(c);
	};
	auto isHex = [&](char c) -> bool {
		switch (toUpper(c)) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			return true;
		default:
			return false;
		}
	};
	do {
		if (*pattern == ' ')
			continue;
		if (*pattern == '?') {
			Element e = Element({}, true);
			m_elements.push_back(e);
			continue;
		}
		if (*(pattern + 1) && isHex(*pattern) && isHex(*(pattern + 1))) {
			char str[3] = { *pattern, *(pattern + 1), '\0' };
			auto data = std::strtol(str, nullptr, 16);

			Element e = Element(static_cast<std::uint8_t>(data), false);
			m_elements.push_back(e);
		}
	} while (*(pattern++));
}

ptr_manage find_pattern::scan(sModule region) {
	auto compareMemory = [](std::uint8_t* data, Element* elem, std::size_t num) -> bool {
		for (std::size_t i = 0; i < num; ++i) {
			if (!elem[i].m_wildcard)
				if (data[i] != elem[i].m_data)
					return false;
		}
		return true;
	};
	for (std::uintptr_t i = region.get_begin().as<std::uintptr_t>(), end = region.get_end().as<std::uintptr_t>(); i != end; ++i) {
		if (compareMemory(reinterpret_cast<std::uint8_t*>(i), m_elements.data(), m_elements.size()))
			return ptr_manage(i);
	}
	return nullptr;
}

pattern_hisnt::pattern_hisnt(std::string name, find_pattern pattern, std::function<void(ptr_manage)> callback) : m_name(std::move(name)), m_pattern(std::move(pattern)), m_callback(std::move(callback)) { }

void pattern_batch::add(std::string name, find_pattern pattern, std::function<void(ptr_manage)> callback) {
	pattern_list.emplace_back(name, pattern, callback);
}

void pattern_batch::run() {
	const sModule module = { GetModuleHandle(nullptr) };
	for (auto& hisnt : pattern_list) {
		if (auto result = hisnt.m_pattern.scan(module)) {
			if (!hisnt.m_callback)
				continue;
			if (!result.as<uintptr_t>())
				continue;
			std::invoke(std::move(hisnt.m_callback), result);
			if (!std::strcmp(hisnt.m_name.c_str(), ""))
				continue;
		}
	}
	pattern_list.clear();
}