#pragma once
#include <ostream>

namespace mmx5_diagnostic {
// Owned presentation metadata; never part of guest state or replay input.
struct Mode {
    bool netplay=false,coop=false,split=false,ready=false;
    unsigned campaign=0;
    bool operator==(const Mode& other) const {
        return netplay==other.netplay && coop==other.coop && split==other.split &&
            ready==other.ready && campaign==other.campaign;
    }
    const char *play_name() const {return netplay?"netplay":"local";}
    const char *camera_name() const {
        if(!coop)return "single_player";
        return netplay?(split?"netplay_separate":"netplay_unified"):
            (split?"local_split_8_3":"local_unified");
    }
    const char *campaign_name() const {return campaign==0?"x":campaign==1?"zero":"unknown";}
};
inline void mode_json(std::ostream& out,const Mode& mode) {
    out<<"{\"play_mode\": \""<<mode.play_name()<<"\", \"camera_mode\": \""<<mode.camera_name()
       <<"\", \"coop_enabled\": "<<(mode.coop?"true":"false")
       <<", \"configured_players\": "<<(mode.coop?2:1)
       <<", \"partner_initialized\": "<<(mode.ready?"true":"false")
       <<", \"campaign\": \""<<mode.campaign_name()<<"\"}";
}
}
