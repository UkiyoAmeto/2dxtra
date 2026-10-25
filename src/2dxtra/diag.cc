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
}
