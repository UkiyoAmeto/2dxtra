#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <string>

#include "diag.h"

namespace iidxtra::diag
{
	namespace
	{
		std::mutex mutex;
		std::FILE* file = nullptr;
	}

	auto open(HMODULE module) -> void
	{
		auto const guard = std::lock_guard { mutex };

		if (file != nullptr)
			return;

		wchar_t path[MAX_PATH] = {};
		GetModuleFileNameW(module, path, MAX_PATH);

		auto const text = std::wstring { path };
		auto const slash = text.find_last_of(L"\\/");
		auto const log_path = (slash == std::wstring::npos ? std::wstring {}: text.substr(0, slash + 1)) + L"2dxtra.log";

		file = _wfopen(log_path.c_str(), L"a");
	}

	auto write(const char* format, ...) -> void
	{
		auto const guard = std::lock_guard { mutex };

		if (file == nullptr)
			return;

		SYSTEMTIME time {};
		GetLocalTime(&time);

		std::fprintf(file, "[%04u-%02u-%02u %02u:%02u:%02u] ",
			time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);

		va_list args;
		va_start(args, format);
		std::vfprintf(file, format, args);
		va_end(args);

		std::fputc('\n', file);
		std::fflush(file);
	}

	auto describe_address(const std::uintptr_t address, std::string& module_name) -> std::uintptr_t
	{
		wchar_t path[MAX_PATH] = {};
		HMODULE module = nullptr;

		if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
			| GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCWSTR>(address), &module) || module == nullptr)
		{
			module_name = "?";
			return address;
		}

		GetModuleFileNameW(module, path, MAX_PATH);

		auto const text = std::wstring { path };
		auto const slash = text.find_last_of(L"\\/");
		auto const name = (slash == std::wstring::npos ? text: text.substr(slash + 1));

		module_name.assign(name.begin(), name.end());
		return address - reinterpret_cast<std::uintptr_t>(module);
	}

	auto on_crash(EXCEPTION_POINTERS* info) -> LONG
	{
		auto const* const record = info->ExceptionRecord;
		auto const address = reinterpret_cast<std::uintptr_t>(record->ExceptionAddress);

		auto module_name = std::string {};
		auto const offset = describe_address(address, module_name);

		write("[crash] code %08lX at %s+0x%llX (flags %08lX, params %llu)",
			static_cast<unsigned long>(record->ExceptionCode), module_name.c_str(),
			static_cast<unsigned long long>(offset),
			static_cast<unsigned long>(record->ExceptionFlags),
			static_cast<unsigned long long>(record->NumberParameters));

		for (DWORD i = 0; i < record->NumberParameters && i < 4; i++)
			write("[crash]   arg %lu = %p",
				static_cast<unsigned long>(i),
				reinterpret_cast<void*>(record->ExceptionInformation[i]));

		auto const* const context = info->ContextRecord;

		if (context != nullptr)
			write("[crash]   rip %p rsp %p rbp %p",
				reinterpret_cast<void*>(context->Rip),
				reinterpret_cast<void*>(context->Rsp),
				reinterpret_cast<void*>(context->Rbp));

		return EXCEPTION_CONTINUE_SEARCH;
	}

	auto install_crash_handler() -> void
		{ AddVectoredExceptionHandler(1, on_crash); }
}
