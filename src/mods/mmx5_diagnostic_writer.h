/* Bounded background capture, following snesrecomp's diagnostic writers
 * (a6be9756285f26ccc74539c811d4d23d57d8c33d). Only owned frame-boundary
 * copies cross the queue; the worker never reads emulated memory or APIs.
 * One simulation-thread producer. Close/flush require that producer to stop.
 * Normal close drains; a full queue stops capture rather than dropping rows
 * or blocking gameplay. The existing mmx5-input-v2 format is unchanged. */
#pragma once
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include "mmx5_diagnostic_mode.h"

namespace mmx5_diagnostic {
using Row = std::array<int64_t,18>;
using State = std::array<uint8_t,0x1C0+0x158+0x158>;
inline uint32_t hash(const State& state,unsigned size) {
    uint32_t h=2166136261u;
    for(unsigned i=0;i<size;++i)h=(h^state[i])*16777619u;
    return h;
}

template<size_t Capacity=4096> class Writer {
    enum class Kind { Native, Controller, StateLoad, Development, Mode, Flush };
    enum Error { None, Full, Io, Start, Invalid };
    struct Event {
        Row row{};
        State state{};
        unsigned size=0;
        Kind kind=Kind::Native;
    };
    static_assert(Capacity>1,"queue must hold at least one event");
    std::unique_ptr<Event[]> queue;
    std::ofstream file;
    std::ofstream modes;
    bool modes_enabled=false;
    std::thread thread;
    std::atomic<size_t> read{0},write{0};
    std::atomic<unsigned> flushes{0};
    unsigned requested_flushes=0;
    std::atomic<int> error{None};
    std::atomic<bool> stop{false};
    bool opened=false;
#ifdef MMX5_DIAGNOSTIC_WRITER_TEST
    void (*gate)()=nullptr;
#endif
    void fail(Error value) {int expected=None;error.compare_exchange_strong(expected,value);}
    Event *reserve() {
        if(!opened || error.load(std::memory_order_relaxed))return nullptr;
        size_t head=write.load(std::memory_order_relaxed);
        if((head+1)%Capacity==read.load(std::memory_order_acquire)) {fail(Full);return nullptr;}
        return &queue[head];
    }
    void publish() {
        write.store((write.load(std::memory_order_relaxed)+1)%Capacity,std::memory_order_release);
    }
    void run() {
#ifdef MMX5_DIAGNOSTIC_WRITER_TEST
        if(gate)gate();
#endif
        auto last_flush=std::chrono::steady_clock::now();
        bool dirty=false;
        for(;;) {
            size_t tail=read.load(std::memory_order_relaxed);
            if(tail!=write.load(std::memory_order_acquire)) {
                const auto& event=queue[tail];
                switch(event.kind) {
                case Kind::Native:
                    file<<"N ";
                    for(unsigned i=0;i<event.row.size();++i)
                        file<<(i?",":"")<<(i==17?int64_t(hash(event.state,event.size)):event.row[i]);
                    file<<'\n';break;
                case Kind::Controller:
                    file<<"C "<<event.row[0]<<' '<<event.row[1]<<' '<<event.row[2]<<'\n';break;
                case Kind::StateLoad:file<<"# network state load\n";break;
                case Kind::Development:
                    file<<"D "<<event.row[0]<<' '<<event.row[1]<<' '<<event.row[2]
                        <<' '<<event.row[3]<<' '<<event.row[4]<<'\n';break;
                case Kind::Mode: {
                    Mode mode{bool(event.row[2]),bool(event.row[3]),bool(event.row[4]),
                              bool(event.row[5]),unsigned(event.row[6])};
                    modes<<event.row[0]<<','<<event.row[1]<<','<<mode.play_name()<<','
                         <<mode.camera_name()<<','<<(mode.coop?2:1)<<','<<mode.ready<<','
                         <<mode.campaign_name()<<'\n';
                    break;
                }
                case Kind::Flush:
                    file.flush();if(modes_enabled)modes.flush();
                    flushes.fetch_add(1,std::memory_order_release);break;
                }
                // Do not release a slot until hashing/formatting has finished.
                read.store((tail+1)%Capacity,std::memory_order_release);
                dirty=true;
                if(!file || (modes_enabled && !modes)) {fail(Io);break;}
            }else {
                if(stop.load(std::memory_order_acquire) || error.load())break;
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
            auto now=std::chrono::steady_clock::now();
            // Wall time also flushes a hung/paused simulation's final input.
            if(dirty && now-last_flush>=std::chrono::milliseconds(100)) {
                file.flush();if(modes_enabled)modes.flush();last_flush=now;dirty=false;
                if(!file || (modes_enabled && !modes)) {fail(Io);break;}
            }
        }
        file.close();if(!file)fail(Io);
        if(modes_enabled) {modes.close();if(!modes)fail(Io);}
    }
public:
    Writer()=default;
    Writer(const Writer&)=delete;
    Writer& operator=(const Writer&)=delete;
    ~Writer() {close();}
#ifdef MMX5_DIAGNOSTIC_WRITER_TEST
    void test_gate(void (*callback)()) {gate=callback;}
#endif
    void open(const std::filesystem::path& path,const std::string& header,
              const std::filesystem::path& mode_path={}) {
        close();clear();
        // Session setup is outside steady-state recording. Make the header
        // observable immediately and report path/permissions errors now.
        file.open(path,std::ios::out|std::ios::trunc);
        file<<header;file.flush();
        if(!file) {fail(Io);file.close();return;}
        if(!mode_path.empty()) {
            modes.open(mode_path,std::ios::out|std::ios::trunc);
            modes<<"native_input,game_frame,play_mode,camera_mode,configured_players,partner_initialized,campaign\n";
            modes.flush();
            if(!modes) {fail(Io);modes.close();file.close();return;}
            modes_enabled=true;
        }
        try {
            queue=std::make_unique<Event[]>(Capacity);
            opened=true;thread=std::thread([this] {run();});
        }catch(...) {opened=false;fail(Start);file.close();if(modes_enabled)modes.close();queue.reset();}
    }
    bool native(const Row& row,const State& state,unsigned size) {
        if(size>state.size()) {fail(Invalid);return false;}
        auto *event=reserve();if(!event)return false;
        event->kind=Kind::Native;event->row=row;event->state=state;event->size=size;
        publish();return true;
    }
    bool controller(unsigned sequence,unsigned seat,uint16_t buttons) {
        auto *event=reserve();if(!event)return false;
        event->kind=Kind::Controller;event->row[0]=sequence;event->row[1]=seat;event->row[2]=buttons;
        publish();return true;
    }
    void state_load() {auto *event=reserve();if(event) {event->kind=Kind::StateLoad;publish();}}
    void development(unsigned sequence,unsigned command,unsigned seat,unsigned hp,unsigned ammo) {
        auto *event=reserve();if(!event)return;
        event->kind=Kind::Development;event->row[0]=sequence;event->row[1]=command;
        event->row[2]=seat;event->row[3]=hp;event->row[4]=ammo;publish();
    }
    bool mode(unsigned sequence,unsigned frame,const Mode& mode) {
        if(!modes_enabled) {fail(Invalid);return false;}
        auto *event=reserve();if(!event)return false;
        event->kind=Kind::Mode;
        event->row={sequence,frame,mode.netplay,mode.coop,mode.split,mode.ready,mode.campaign};
        publish();return true;
    }
    // Explicit barrier for tests/tools, never called by the recording loop.
    void flush() {
        auto *event=reserve();if(!event)return;
        unsigned target=++requested_flushes;event->kind=Kind::Flush;publish();
        while(flushes.load(std::memory_order_acquire)<target && !error.load())
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    void close() {
        stop.store(true,std::memory_order_release);
        if(thread.joinable())thread.join();
        else {
            if(file.is_open()) {file.close();if(!file)fail(Io);}
            if(modes.is_open()) {modes.close();if(!modes)fail(Io);}
        }
        opened=false;queue.reset();
    }
    void clear() {
        // Caller must close first; a worker can never outlive its queue.
        if(opened)return;
        file.clear();modes.clear();modes_enabled=false;error.store(None);stop.store(false);
        read.store(0);write.store(0);flushes.store(0);requested_flushes=0;
    }
    bool is_open() const {return opened;}
    explicit operator bool() const {return error.load(std::memory_order_relaxed)==None;}
    const char *error_text() const {
        switch(error.load(std::memory_order_relaxed)) {
        case Full:return "background diagnostic queue full; capture stopped";
        case Io:return "diagnostic file write failed";
        case Start:return "cannot start diagnostic background writer";
        case Invalid:return "invalid diagnostic state size";
        default:return nullptr;
        }
    }
};
}
