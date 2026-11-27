#pragma once
#include <array>
#include <cstdint>
#include "mmx5_diagnostic_mode.h"
#ifdef MMX5_DIAGNOSTIC_WRITER_TEST
#define RECOMP_DIAGNOSTIC_NATIVE_INPUT_WRITER_TEST
#endif
#include <recomp_diagnostics/native_input_writer.hpp>
namespace mmx5_diagnostic {
using Row = std::array<int64_t,18>;
using State = std::array<uint8_t,0x1C0+0x158+0x158>;
inline uint32_t hash(const State& state,unsigned size) {
    uint32_t h=2166136261u;
    for(unsigned i=0;i<size;++i)h=(h^state[i])*16777619u;
    return h;
}
struct RecordingPolicy {
    using Row=mmx5_diagnostic::Row;
    using State=mmx5_diagnostic::State;
    using Mode=mmx5_diagnostic::Mode;
    static constexpr unsigned HashColumn=17;
    static uint32_t hash(const State& state,unsigned size) {return mmx5_diagnostic::hash(state,size);}
    static Row mode_row(unsigned sequence,unsigned frame,const Mode& mode) {
        return {sequence,frame,mode.netplay,mode.coop,mode.split,mode.ready,mode.campaign};
    }
    static const char* mode_header() {
        return "native_input,game_frame,play_mode,camera_mode,configured_players,partner_initialized,campaign\n";
    }
    static void write_mode(std::ostream& output,const Row& row) {
        Mode mode{bool(row[2]),bool(row[3]),bool(row[4]),bool(row[5]),unsigned(row[6])};
        output<<row[0]<<','<<row[1]<<','<<mode.play_name()<<','<<mode.camera_name()<<','
              <<(mode.coop?2:1)<<','<<mode.ready<<','<<mode.campaign_name()<<'\n';
    }
};
template<size_t Capacity=4096>
using Writer=recomp::diagnostics::NativeInputWriter<Capacity,RecordingPolicy>;
}
