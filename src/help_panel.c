#include "help_panel.h"
static const SniperPanelRow flight[]={
    {"ESC   = Exit game",1},
    {"ENTER = Send message",1},
    {"F2    = Toggle statistics",1},
    {"F3    = Toggle name tags",1},
    {"F4    = Toggle map display",1},
    {"F5    = Toggle message display",1},
    {"F6    = Toggle star field",1},
    {"F7    = Attach/Detach",1},
    {"F8    = Toggle engine sounds",1},
    {"F11   = Toggle help ticker",1},
    {"F12   = Commit suicide",1},
    {"",1},
    {"ARROWS = Movement",1},
    {"CTRL   = Fire guns",1},
    {"TAB    = Fire bomb (if available)",1},
    {"HOME   = Toggle stealth",1},
    {"END    = Toggle x-radar",1},
    {"DELETE = Toggle multi-fire",1},
    {"INSERT = Warp (full energy)",1},
    {"SHIFT  = Modifier key:",1},
    {"  -ARROWS = Afterburners, slow rotation",1},
    {"  -Ctrl   = Repeller",1},
    {"  -TAB    = Landmines (still bombs)",1},
    {"  -HOME   = Toggle cloak",1},
    {"  -END    = Toggle ghosting",1},
};
static const SniperPanelRow spectator_rows[]={
    {"ESC   = Exit spectating",1},
    {"ENTER = Send message",1},
    {"F1    = Toggle help",1},
    {"F2    = Toggle statistics",1},
    {"F3    = Toggle name tags",1},
    {"F4    = Toggle map display",1},
    {"F5    = Toggle message display",1},
    {"F6    = Toggle star field",1},
    {"TAB   = Switch spectatee",1},
};
const SniperPanelRow *sniper_help_rows(int spectator,int32_t *count) {
    *count=spectator?(int32_t)(sizeof spectator_rows/sizeof spectator_rows[0]):(int32_t)(sizeof flight/sizeof flight[0]);
    return spectator?spectator_rows:flight;
}

int32_t sniper_help_toggle(int32_t mode,int32_t type){return mode==1||mode==3?0:type==4?3:1;}
