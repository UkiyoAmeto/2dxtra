#pragma once

#include <windows.h>

namespace iidxtra::diag
{
	// Opens "<module dir>\2dxtra.log" in append mode. Safe to call repeatedly.
	auto open(HMODULE module) -> void;

	// Appends one timestamped line and flushes it. No-op if the file is closed.
	auto write(const char* format, ...) -> void;
}
