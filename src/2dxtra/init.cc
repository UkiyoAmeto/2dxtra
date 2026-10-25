#include <filesystem>
#include <meta.h>
#include <cstring>
#include "diag.h"
#include <MinHook.h>
#include "chart_set.h"
#include "score_set.h"
#include "database.h"
#include "settings.h"
#include "hooks/input_hook.h"
#include "hooks/network_hook.h"
#include "hooks/chart_load_hook.h"
#include "hooks/chart_analyze_hook.h"
#include "hooks/dan_select_hook.h"
#include "hooks/music_select_hook.h"
#include "hooks/stage_result_hook.h"
#include "hooks/mselect_genre_hook.h"
#include "hooks/mdata_load_hook.h"
#include "hooks/boot_screen_hook.h"
#include "hooks/renderer_hook.h"
#include "hooks/reset_state_hook.h"
#include "hooks/result_title_hook.h"
#include "hooks/score_invalidator_hook.h"
#include "hooks/card_out_hook.h"
#include "hooks/play_field_load_hook.h"
#include "hooks/judge_timing_hook.h"
#include "hooks/fast_slow_hook.h"
#include "hooks/attract_randomizer_hook.h"
#include "features/autoplay.h"
#include "features/unrandomizer.h"
#include "features/autoretry.h"
#include "features/chart_speed.h"
#include "features/hi_speed_reset.h"

namespace iidxtra
{
	auto init(LPVOID param) -> DWORD
	{
		auto const module = static_cast<HMODULE>(param);

		diag::write("2dxtra %s init thread started", VERSION_STRING);
		diag::write("  profile %s  base %p  max_entries %#zx",
			bm2dx::addr->GAME_VERSION.c_str(), bm2dx::base, bm2dx::max_entries());
		diag::write("  state %p  config %p  input %p  scores %p/%p  rivals %p/%p",
			static_cast<void*>(bm2dx::state), static_cast<void*>(bm2dx::config),
			static_cast<void*>(bm2dx::input_manager),
			static_cast<void*>(bm2dx::scores[0]), static_cast<void*>(bm2dx::scores[1]),
			static_cast<void*>(bm2dx::rival_scores[0]), static_cast<void*>(bm2dx::rival_scores[1]));

		// Cheap self-checks that prove the data offsets against the live game:
		// the input manager must point at a vtable whose slot 3 is INPUT_POLL_FN,
		// and the app config must expose a plausible frame rate.
		{
			auto vtable_ok = false;

			if (bm2dx::input_manager != nullptr)
			{
				auto const vft = *reinterpret_cast<std::uint8_t**>(bm2dx::input_manager);
				auto const image_end = bm2dx::base + bm2dx::addr->DLL_IMAGE_SIZE;

				if (vft >= bm2dx::base && vft < image_end)
				{
					auto const slot3 = *reinterpret_cast<std::uint8_t**>(vft + 3 * sizeof(void*));
					vtable_ok = slot3 == bm2dx::addr->INPUT_POLL_FN;
				}
			}

			diag::write("probe: input manager vtable %s", vtable_ok ? "ok" : "MISMATCH");
		}

		if (bm2dx::config != nullptr)
		{
			diag::write("probe: config target_fps %d monitor_check_fps %.2f",
				bm2dx::config->target_fps, static_cast<double>(bm2dx::config->monitor_check_fps));
		}

		if (bm2dx::state != nullptr)
		{
			diag::write("probe: state game_type %d play_style %d p1 %d p2 %d",
				bm2dx::state->game_type, bm2dx::state->play_style,
				bm2dx::state->p1_active, bm2dx::state->p2_active);
		}

		MH_Initialize();

		// Open the chart database next to the module.
		wchar_t module_path[MAX_PATH] = {};
		GetModuleFileNameW(module, module_path, MAX_PATH);

		auto const cwd = std::filesystem::path { module_path }.remove_filename();
		auto const db_path = (cwd / "2dxtra.sqlite").string();
		auto const db = database::open(db_path.c_str());

		chart_set::init(db);

		// Allocate memory.
		diag::write("score_set::init");
		diag::write("  score isolation %s", score_set::init() ? "enabled" : "DISABLED");

		// Initialize hooks.
		diag::write("install chart_analyze_hook");
		chart_analyze_hook::install(db);
		diag::write("install input_hook");
		input_hook::install_hook();
		diag::write("install network_hook");
		network_hook::install_hook();
		diag::write("install chart_load_hook");
		chart_load_hook::install_hook();
		diag::write("install dan_select_hook");
		dan_select_hook::install_hook();
		diag::write("install music_select_hook");
		music_select_hook::install_hook();
		diag::write("install stage_result_hook");
		stage_result_hook::install_hook();
		diag::write("install mselect_genre_hook");
		mselect_genre_hook::install_hook();
		diag::write("install mdata_load_hook");
		mdata_load_hook::install_hook();
		diag::write("install boot_screen_hook");
        boot_screen_hook::install_hook();
		diag::write("install reset_state_hook");
        reset_state_hook::install_hook();
		diag::write("install result_title_hook");
        result_title_hook::install_hook();
		diag::write("install score_invalidator_hook");
		score_invalidator_hook::install_hook();
		diag::write("install card_out_hook");
        card_out_hook::install_hook();
		diag::write("install autoretry");
        autoretry::install_hook();
		diag::write("install play_field_load_hook");
        play_field_load_hook::install_hook();
		diag::write("install judge_timing_hook");
        judge_timing_hook::install_hook();
		diag::write("install fast_slow_hook");
		fast_slow_hook::install_hook();
		diag::write("install autoplay");
		autoplay::install_hook();
		diag::write("install unrandomizer");
		unrandomizer::install_hook();
		diag::write("install attract_randomizer_hook");
		attract_randomizer_hook::install_hook();
		diag::write("install chart_speed");
		chart_speed::install_hook();
		diag::write("install hi_speed_reset");
        hi_speed_reset::install_hook();

		settings::init(db);

		// Enable all hooks.
		diag::write("enabling hooks");
		MH_EnableHook(MH_ALL_HOOKS);
		diag::write("init complete");

#ifndef NDEBUG
		// Wait for detach signal.
		while (true)
		{
			if (GetAsyncKeyState(VK_LSHIFT) && GetAsyncKeyState(VK_F10))
			{
				// Revert to default before unloading.
				chart_set::revert();
				break;
			}

			Sleep(10);
		}

		// Free stock score storage.
		score_set::uninit();

		// Hooks that require extra setup.
        renderer_hook::uninstall_hook();
		card_out_hook::uninstall_hook();

		database::close(db);

		MH_Uninitialize();
		FreeLibraryAndExitThread(static_cast<HMODULE>(param), 0);
#endif

		return 0;
	}
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		iidxtra::diag::open(module);

		if (!bm2dx::resolve())
		{
			iidxtra::diag::write("no offsets profile matched this bm2dx.dll - refusing to load");
			return FALSE;
		}

		iidxtra::diag::write("loaded against %s (2dxtra %s)", bm2dx::addr->GAME_VERSION.c_str(), VERSION_STRING);

		{
			char model[24] = {};
			std::memcpy(model, bm2dx::addr->GAME_MODEL, sizeof(model) - 1);
			auto const model_ok = std::strcmp(model, "***:*:*:*:**********") == 0;
			iidxtra::diag::write("probe: game model \"%s\" %s", model, model_ok ? "ok" : "MISMATCH");
		}

		bm2dx::set_soft_rev('E');

		CreateThread(nullptr, 0, iidxtra::init, module, 0, nullptr);
	}

	return TRUE;
}
