#pragma once

#include <windows.h>

namespace iidxtra::diag
{
	// Opens "<module dir>\2dxtra.log" in append mode. Safe to call repeatedly.
	auto open(HMODULE module) -> void;

	// Appends one timestamped line and flushes it. No-op if the file is closed.
	auto write(const char* format, ...) -> void;

	// Logs the exception address of any unhandled exception (and continues the
	// search for a real handler) so that a crash can be located from the log.
	auto install_crash_handler() -> void;
}
