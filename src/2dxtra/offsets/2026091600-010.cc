// IIDX 34 (bm2dx010.dll) -- build 2026091600-010.
//
// Recovered from the 33rd-gen build 2026081900-010 by masked code-signature
// transfer, MSVC RTTI vtable alignment and byte-pattern matching; every entry
// was checked against its 33rd-gen counterpart instruction by instruction.
// See docs/2026091600-010.md in the fork for the evidence trail.
//
// NOTE: IIDX 34 inserted three virtuals into the common scene base class, so
// every scene vtable slot >= 13 moved by +3 (see STAGE_RESULT_FN,
// DAN_SELECT_CTOR, MDATA_LOAD_FN). Do not reuse 33rd-gen slot numbers.
versions.push_back({
    .GAME_VERSION             = "2026091600-010",

    .DLL_CODE_SIZE            = 0x00c64200, // optional header SizeOfCode
    .DLL_ENTRYPOINT           = 0x00b02b3c, // optional header AddressOfEntryPoint
    .DLL_IMAGE_SIZE           = 0x0c5ff000, // optional header SizeOfImage

    // offsets: code
    .SOFT_REV_PATCH           = base + 0x0934fb9, // call replaced by `mov byte ptr [rdi+5],rev`; 33rd-gen 0x94e6f9
    .INPUT_POLL_FN            = base + 0x0a67d70, // IO::InputManagerIIDX vftable slot 3 (vftable 0xd9ff20)
    .ARENA_PHASE_PATCH        = base + 0x058f760, // arena phase clamp helper
    .MUSIC_SELECT_CTOR        = base + 0x05f6e40, // builds the music list; stores [this+4]=1, [this+6]=0x100
    .DAN_SELECT_CTOR          = base + 0x08b9cc0, // CDanSelectScene vtable slot 16 (was 13 in the 33rd gen)
    .SCENE_DTOR               = base + 0x09513d0, // scene teardown (mid-hook target; 129-byte function)
    .STAGE_RESULT_FN          = base + 0x0914080, // CStageResultScene vtable slot 16 (was 13)
    .RESET_STATE_FN           = base + 0x059d220, // memset(PLAYER_BLOCK + player*0x3e66db8, 0, 0x3e66db8)
    .MSELECT_GENRE_C          = base + 0x05f5c40, // renders the hovered entry's genre and title
    .MSELECT_GENRE_B          = base + 0x05f5cd1, // `cmp music_entry::texture_genre,0` (7 bytes, patched to jmp)
    .MSELECT_GENRE_A          = base + 0x05f5dad, // call to the wide genre text renderer
    .MSELECT_GENRE_TEXT_FN    = base + 0x05df070, // wide/UTF-16 text renderer called from MSELECT_GENRE_A
    .INIT_TEXT_RENDER_FN      = base + 0x05de0e0, // boot debug text renderer
    .BOOT_MODEL_TEXT_CALL     = base + 0x092648b, // call that draws the boot model string
    .TITLE_MODEL_TEXT_CALL    = base + 0x091d68c, // call that draws the title-screen model string
    .TEXT_INIT_FN             = base + 0x032e480, // initialises a text property block
    .TEXT_RENDER_FN           = base + 0x05de460, // ASCII text renderer wrapper
    .RESULT_ARTIST_FN         = base + 0x0907139, // call that draws the artist line on the result screen
    .LOAD_CHART_FN_B          = base + 0x08141b0, // reads one .1 chart into the scratch buffer
    .LOAD_CHART_FN_A          = base + 0x08142d0, // outer loader; knows which player is loading
    .CHART_ANALYZE_FN         = base + 0x0813040, // boot-time chart load + analyze pass
    .CHART_ANALYZE_RESULT     = base + 0x0813267, // instruction after the chart fread (rax=bytes, rdi=buffer)
    .CHART_CALC_RADAR_FN      = base + 0x0813b60, // computes the six radar values of a difficulty
    .GET_APP_CONFIG           = base + 0x0922810, // mov rax,[0xb4e3360]; add rax,0x7c8; ret
    .GET_MUSIC_DATA           = base + 0x0939100, // lea rax,[0xb60c870]; ret
    .LOAD_AUDIO_FN            = base + 0x0a7e700, // loads a sound file into the sound bank
    .GET_SOUND_ENTRY_FN       = base + 0x0a7e2a0, // returns the sound bank entry for a sound index
    .XRPC_APPLY_FN            = base + 0x0aa42e0, // wrapper around avs2-ea3 xrpc_apply
    .REG_DISPATCH_FN          = base + 0x09013f0, // builds and fires the music.reg request
    .REG_PATCH_ADDR           = base + 0x090218e, // call inside it that queues the request
    .SCORE_INVALID_FN         = base + 0x081ab30, // returns whether this play may save
    .DAN_SAVE_FN              = base + 0x05a5880, // queues the dan course result
    .EAAPPLI_SAVE_FN          = base + 0x05aa5a0, // queues the eamuse app result
    .MDATA_LOAD_FN            = base + 0x091af40, // CMonitorCheckScene vtable slot 18 (was 15)
    .IS_BTN_DOWN_FN_A         = base + 0x080b17d, // cmp autoplay flag inside the `button pressed` helper
    .IS_BTN_DOWN_FN_B         = base + 0x080b21d, // cmp autoplay flag inside the `button held` helper
    .AUTO_BEAM_PATCH          = base + 0x08f0b7b, // cmp autoplay flag in the lane beam renderer
    .AUTO_BEAM_FN             = base + 0x08f0920, // lane beam renderer
    .CARD_OUT_VFUNC           = base + 0x0d46608, // slot 0 of the CCardOutScene vftable (the slot itself, not the function)
    .RENDERER_PATCH           = base + 0x05b8dd2, // call nop'd while the swap chain vtable is swapped
    .APPLY_RANDOM_FN          = base + 0x080fb60, // applies the random modifier
    .DARK_MODE_PATCH          = base + 0x08ed2fb, // short `je` over the black frame tint assignment
    .MEASURE_PATCH            = base + 0x08103c9, // branch over the measure bar draw
    .BPM_BAR_PATCH            = base + 0x08ec66a, // start of the bpm gradient draw
    .BPM_BAR_PATCH_JMP        =        0x00000ba, // still 0xba: `playm_bpm_gra_bar` block is byte-identical
    .PLAY_FIELD_LOAD          = base + 0x0808990, // fills the play field note table
    .RETRY_CHECK_A            = base + 0x08fac68, // call testing the EFFECT bit
    .RETRY_CHECK_B            = base + 0x08fac7a, // call testing the VEFX bit
    .GHOST_TARGET_FN          = base + 0x08f14d9, // call that renders the '+????' ghost text
    .GRAPH_TARGET_FN          = base + 0x0a203b5, // call that renders the 'TARGET:+????' text
    .GRAPH_CONDITION          = base + 0x0a2036d, // gate on whether the TARGET text is drawn
    .FAIL_ANIMATION_FN        = base + 0x08fb47b, // call that starts the stage failed animation
    .FAIL_PLAY_SFX_FN         = base + 0x08fb4aa, // call that plays the stage failed sound effect
    .FAIL_DURATION_JMP        = base + 0x08fae20, // `jl` deciding whether the failed animation still runs
    .WNDPROC_FN               = base + 0x0923d20, // PropClassWindowProc
    .TIMING_HOOK_FN           = base + 0x080db00, // jump-table dispatcher that installs the judge windows
    .ATTRACT_SELECT_FN        = base + 0x088dd20, // picks the next attract demo chart
    .IS_SEPARATE_SCRATCH_FN   = base + 0x07f48f0, // option getter reads [block+0x68] (unchanged in 34)
    .SPRITE_DRAW_FN           = base + 0x0334ed0, // named sprite draw helper
    .JUDGE_APPLY_FN           = base + 0x080d480, // applies a note judgment
    .JUDGE_DISPLAY_FN         = base + 0x08f4bf0, // updates judge, combo and FAST/SLOW state
    .JUDGE_DRAW_FS_KEYS_FN    = base + 0x08f43c0, // draws FAST/SLOW for keys
    .JUDGE_DRAW_FS_SC_FN      = base + 0x08f46b0, // draws the separate scratch FAST/SLOW indicator
    .JUDGE_DISPLAY_INIT_FN    = base + 0x08f4980, // initialises one player's judgment display state
    .JUDGE_PRESS_RETURN       = base + 0x080e0b1, // return address of general timing calculation
    .JUDGE_RELEASE_RETURN     = base + 0x080d05a, // return address of CN release timing calculation
    .HI_SPEED_RESET_PATCH     = base + 0x08f4f13, // `xor byte ptr [rsi+28h], 1`
    .HI_SPEED_RESET_CONTINUE  = base + 0x08f5023, // mov rcx,rsi; call lane/green-number predicate
    .HI_SPEED_ADJUST_CALL     = base + 0x08f6716, // start+turntable adjustment call inside the double-START handler
    .GET_PLAYER_OPTIONS       = base + 0x0871910, // mov rax,[0xb3e9f28]; ret
    .IS_DOUBLE_PLAY           = base + 0x081a710, // native SP/DP selector used by option accessors
    .GET_HI_SPEED_MODE        = base + 0x08848d0, // selected option block +0x1c
    .GET_SAVED_HI_SPEED       = base + 0x0882970, // option speed in native scroll-speed units
    .SET_HI_SPEED             = base + 0x0815b90, // native clamped, interpolated scroll-speed setter

    // offsets: data
    .GAME_MODEL               = base + 0x1041fd0, // mutable ea3 model string; rev writer stores to +8
    .GAME_STATE               = base + 0x0b60b3a0, // bm2dx::state_t (p1/p2 active at +0x10/+0x14)
    .INPUT_MANAGER            = base + 0x0bcce280, // IO::InputManagerIIDX singleton
    .PLAYER_BLOCK             = base + 0x0328c068, // base of player 0's game data block (RESET_STATE_FN clears it)
    .SCORES_P1                = base + 0x32badd4, // player 0 score table (block array 0x107c0f0 + 0x223ece4)
    .SCORES_P2                = base + 0x36a158c, // = SCORES_P1 + per-player block stride 0x3e66db8
    .RIVAL_SCORES_P1          = base + 0x3b35d78, // player 0 rival score tables (block array + 0x2bdbc88)
    .RIVAL_SCORES_P2          = base + 0x3f1cb30, // = RIVAL_SCORES_P1 + 0x3e66db8
    .MAX_ENTRIES              =        0x00088b8, // music_data.bin index table: 35000 entries (`occupied_entries` in the blob)
    .D3D9_DEVICE              = nullptr,          // resolved at runtime, see D3D9_MANAGER_PTR
    .AUTO_PLAY                = base + 0x0b3cabbc, // autoplay flag the game itself tests
    .RANDOM_DATA              = base + 0x0b125990, // object holding the post-random lane order
    .PLAY_STATE               = base + 0x0b123870, // play score/note counters and the pacemaker target
    .PLAY_SESSION             = base + 0x0b4dda70, // gameplay session block
    .DEAD_STATE               = base + 0x0b4dd620, // per-player alive flags
    .HI_SPEED_STATE           = base + 0x0b3e3b90, // native scroll-speed singleton

    // IIDX 34 allocates the renderer/app manager on the heap, so the device
    // pointer is no longer at a fixed address. The manager pointer lives in a
    // static and the IDirect3DDevice9* is its +0xe0 field.
    .D3D9_MANAGER_PTR         = base + 0x0b4e3360,
});
