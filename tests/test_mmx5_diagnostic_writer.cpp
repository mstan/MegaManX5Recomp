#define MMX5_DIAGNOSTIC_WRITER_TEST
#include "mmx5_diagnostic_writer.h"
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <vector>
using namespace mmx5_diagnostic;
#define CHECK(x) do {if(!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}} while(0)
static std::atomic<bool> entered{false},released{false};
static void gate() {
    entered.store(true);
    while(!released.load())std::this_thread::sleep_for(std::chrono::milliseconds(1));
}
static std::string contents(const std::filesystem::path& path) {
    std::ifstream in(path);return std::string(std::istreambuf_iterator<char>(in),{});
}
static void reference(std::ostream& out,Row row,const State& state) {
    row[17]=hash(state,state.size());out<<"N ";
    for(unsigned i=0;i<row.size();++i)out<<(i?",":"")<<row[i];
    out<<'\n';
}
int main(int argc,char **argv) {
    auto root=std::filesystem::u8path(argc>1?argv[1]:"writer-qa")/
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    std::filesystem::create_directories(root);
    Row row{};State state{};
    for(unsigned i=0;i<state.size();++i)state[i]=uint8_t(i);
    // Slow filesystem: producers return at capacity, drain exactly the owned
    // accepted prefix, and never borrow mutable frame memory.
    Writer<8> blocked;blocked.test_gate(gate);blocked.open(root/"blocked.csv","header\n");
    struct ReleaseOnExit {~ReleaseOnExit() {released.store(true);}} release_gate;
    CHECK(blocked && blocked.is_open());
    auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
    while(!entered.load() && std::chrono::steady_clock::now()<deadline)std::this_thread::yield();
    CHECK(entered.load());
    std::ostringstream expected;expected<<"header\n";
    for(unsigned i=0;i<7;++i) {
        row[0]=i;state[0]=uint8_t(i);
        CHECK(blocked.native(row,state,state.size()));reference(expected,row,state);
    }
    CHECK(!blocked.controller(7,1,65535));
    CHECK(!released.load()); // All submissions returned while disk was gated.
    CHECK(std::string(blocked.error_text()).find("queue full")!=std::string::npos);
    state.fill(0xEE);row.fill(-99);released.store(true);blocked.close();
    CHECK(contents(root/"blocked.csv")==expected.str());
    // Exercise ring wrap and mixed input order, including a final controller
    // read after the last native poll and a network-load annotation.
    Writer<32> writer;writer.open(root/"ordered.csv","header\n",root/"modes.csv");
    expected.str("");expected.clear();expected<<"header\n";
    for(unsigned i=0;i<300;++i) {
        row.fill(i);row[13]=-1234;state[0]=uint8_t(i);
        CHECK(writer.controller(i,1,uint16_t(i)));expected<<"C "<<i<<" 1 "<<i<<'\n';
        CHECK(writer.native(row,state,state.size()));reference(expected,row,state);
        if(i%60==0) {
            unsigned mode=i/60;
            CHECK(writer.mode(i,i+100,{mode>=3,mode!=0,mode==2 || mode==4,mode!=0,mode&1u}));
        }
        if(i==150) {writer.state_load();expected<<"# network state load\n";}
        if(i==151) {writer.development(i,9,0,1,4);expected<<"D 151 9 0 1 4\n";}
        if(i%8==7)writer.flush();
        CHECK(writer);
    }
    CHECK(writer.controller(300,0,42));expected<<"C 300 0 42\n";
    // No more game frames or explicit flush: the worker must flush on wall
    // time even when the producer is paused or hung.
    deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
    while(contents(root/"ordered.csv")!=expected.str() && std::chrono::steady_clock::now()<deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    CHECK(contents(root/"ordered.csv")==expected.str());
    writer.close();CHECK(writer);
    auto modes=contents(root/"modes.csv");
    CHECK(modes.find("0,100,local,single_player,1,0,x\n")!=std::string::npos);
    CHECK(modes.find("60,160,local,local_unified,2,1,zero\n")!=std::string::npos);
    CHECK(modes.find("120,220,local,local_split_8_3,2,1,x\n")!=std::string::npos);
    CHECK(modes.find("180,280,netplay,netplay_unified,2,1,zero\n")!=std::string::npos);
    CHECK(modes.find("240,340,netplay,netplay_separate,2,1,x\n")!=std::string::npos);
    CHECK(contents(root/"ordered.csv")==expected.str());
    // Failed destinations are reported; a later reset can start a new capture.
    writer.open(root/"missing"/"capture.csv","header\n");CHECK(!writer && !writer.is_open());
    writer.open(root/"bad-modes-input.csv","header\n",root/"missing"/"modes.csv");
    CHECK(!writer && !writer.is_open());
    writer.open(root/"recovered.csv","header\n");CHECK(writer && writer.is_open());
    CHECK(writer.native(row,state,state.size()));
    writer.close();CHECK(writer);
    CHECK(contents(root/"recovered.csv").find("N ")!=std::string::npos);
    writer.open(root/"invalid.csv","header\n");
    CHECK(!writer.native(row,state,state.size()+1));writer.close();CHECK(!writer);
    if(argc>2 && std::string(argv[2])=="benchmark") {
        // Compare the old stream/hash loop with the new producer under the
        // same batches. Barriers and shutdown are excluded from producer
        // latency, but included in the separately reported elapsed total.
        constexpr unsigned frames=10000,batch=100;
        double total_ms[2]{};std::vector<double> latencies[2];
        for(unsigned mode=0;mode<2;++mode) {
            auto path=root/(mode?"async.csv":"sync.csv");
            std::ofstream sync;Writer<> async;
            if(mode)async.open(path,"");else sync.open(path);
            auto total_start=std::chrono::steady_clock::now();
            for(unsigned i=0;i<frames;++i) {
                row.fill(i);row[13]=-1234;state[0]=uint8_t(i);
                auto before=std::chrono::steady_clock::now();
                if(mode) {
                    CHECK(async.controller(i,0,65535));CHECK(async.controller(i,1,32767));
                    CHECK(async.native(row,state,state.size()));
                }else {
                    sync<<"C "<<i<<" 0 65535\nC "<<i<<" 1 32767\n";
                    reference(sync,row,state);
                    if(i%60==0)sync.flush();
                }
                latencies[mode].push_back(std::chrono::duration<double,std::micro>(
                    std::chrono::steady_clock::now()-before).count());
                if(mode && i%batch==batch-1)async.flush();
            }
            if(mode) {async.close();CHECK(async);}else sync.close();
            total_ms[mode]=std::chrono::duration<double,std::milli>(
                std::chrono::steady_clock::now()-total_start).count();
        }
        CHECK(contents(root/"async.csv")==contents(root/"sync.csv"));
        for(unsigned mode=0;mode<2;++mode) {
            auto& times=latencies[mode];std::sort(times.begin(),times.end());
            double sum=0;for(auto t:times)sum+=t;
            printf("%s: mean %.3f us p50 %.3f us p95 %.3f us p99 %.3f us total %.1f ms\n",
                mode?"background producer":"old synchronous",sum/times.size(),
                times[times.size()/2],times[times.size()*95/100],times[times.size()*99/100],total_ms[mode]);
        }
        printf("byte-identical output: %llu bytes, %u frames\n",
            (unsigned long long)std::filesystem::file_size(root/"async.csv"),frames);
    }
    printf("bounded diagnostic writer checks passed\n");return 0;
}
