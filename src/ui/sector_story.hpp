#pragma once
#include "content/stages.hpp"
#include <string_view>

namespace vl::story {
enum class Set { relay, salvage, gates, evac, sky };
enum class Voice { lead, partner, neri, archive, aster };
struct Line {
    Voice voice;
    std::string_view text;
};
struct Chapter {
    std::string_view title, reveal;
    Set set;
    std::array<Line,3> lines;
};

inline constexpr std::array<Chapter,sectorCount> chapters{{
    {"Someone is below","PUMP TRANSIT OPEN",Set::gates,{{
        {Voice::lead,"Mule-7 is down. The lift still has power. Listen before we go."},
        {Voice::neri,"This is Neri, shift medic. We're under the mine. The pumps stopped and the lower stairs are filling."},
        {Voice::partner,"Keep them above the waterline. The lift takes us straight to Tideworks."}}}},
    {"The wrong valve","PRESSURE LOG RECOVERED",Set::relay,{{
        {Voice::lead,"The Warden was keeping the floodgate open. That flood had a destination."},
        {Voice::archive,"Override signed by Helix. Divert coolant through the civilian transit shaft. Preserve ore lift operation."},
        {Voice::partner,"The overflow log ends at the research dome. Someone there tried to stop it."}}}},
    {"A second way out","CIVILIAN GALLERY LOCATED",Set::salvage,{{
        {Voice::lead,"The dome staff marked a dry gallery on this map. It runs above the flooded stairs."},
        {Voice::neri,"I can get the families to the gallery. The lift needs heat in the brakes before it'll move."},
        {Voice::partner,"We'll bring the foundry back online. Keep everyone together."}}}},
    {"Heat for the living","CIVILIAN LIFT HEATING",Set::gates,{{
        {Voice::lead,"Crucible is offline. I'm opening the heat loop to the civilian lift."},
        {Voice::neri,"The brake light just turned green. We're moving the children first."},
        {Voice::partner,"Keep that line clear. I'll send our position when we get to Terminus."}}}},
    {"The empty relay","SIGNAL INTERCEPTED IN THE ANNEX",Set::sky,{{
        {Voice::lead,"The uplink sends a test pulse, then the annex eats it. The antenna itself is still good."},
        {Voice::archive,"External traffic quarantined. Maintenance credential required for an emergency transmission."},
        {Voice::partner,"We'll get the credential from the annex. Someone put this block here on purpose."}}}},
    {"The borrowed key","MAINTENANCE KEY ACQUIRED",Set::relay,{{
        {Voice::lead,"Keymaster's code opens the maintenance network. I can see where Helix locked out the rescue routes."},
        {Voice::archive,"Lower manual bypass available. Approach through Blackwater Lock."},
        {Voice::partner,"Neri, stay by the lift. We'll go under the lock and open the bypass from inside."}}}},
    {"A dry minute","SEED VAULT ROUTE OPEN",Set::gates,{{
        {Voice::lead,"Undertow is down. The lower lock will hold long enough to cross."},
        {Voice::neri,"One of the botanists left a field transmitter in the seed vault. Our radios can't reach that deep."},
        {Voice::partner,"We'll take it with us. Save your battery until you hear my call."}}}},
    {"The names on the list","FIELD TRANSMITTER RECOVERED",Set::salvage,{{
        {Voice::lead,"Found the transmitter beside the evacuation roster. There are names from every shift."},
        {Voice::archive,"Secondary heating line disconnected at the Slag Run manifold."},
        {Voice::partner,"Take the roster too. When the ships get here, I want an actual count."}}}},
    {"Warm hands","SHELTER HEAT RESTORED",Set::evac,{{
        {Voice::lead,"Heat is through to the gallery. The manifold's holding."},
        {Voice::neri,"I can feel it through my gloves. We found more people near the old winch bridge."},
        {Voice::partner,"That bridge is over Faultline. Mark where you last heard them."}}}},
    {"The other survivors","TWO SHELTERS IN CONTACT",Set::evac,{{
        {Voice::lead,"That's the winch crew. Their shelter was tucked in behind the ore counterweight."},
        {Voice::neri,"I can hear them now. There are two groups. We'll meet on the dry side of the barrier."},
        {Voice::partner,"The radiator field is directly above us. Let's make sure the pumps have somewhere to send their heat."}}}},
    {"A window in the cold","RADIATOR LOOP ONLINE",Set::sky,{{
        {Voice::lead,"Whiteout's gone. The radiator loop is dumping heat to the surface again."},
        {Voice::archive,"Drowned Grid reports an intact transformer. Local authorization required."},
        {Voice::partner,"We can give the whole route power if we can get to that breaker."}}}},
    {"The lights come back","TRANSIT GRID LIVE",Set::relay,{{
        {Voice::lead,"The transformer has taken the load. I see lights all the way down the transit line."},
        {Voice::neri,"So do we. The winch crew are with us. There's fouled air between here and the barrier."},
        {Voice::partner,"Sporeline holds the ventilation controls. Stay where the air is clear."}}}},
    {"Room to breathe","FRESH AIR AT THE SHELTER",Set::gates,{{
        {Voice::lead,"The intake is running backward. Give it a moment to clear the gallery."},
        {Voice::neri,"The air's getting clearer. The next door says it can't close without coolant."},
        {Voice::partner,"Ash Circuit feeds the pressure doors. We'll give them a clean loop."}}}},
    {"The handoff","PRESSURE DOOR COOLANT STABLE",Set::relay,{{
        {Voice::lead,"The coolant loop is steady. We still need a manual release on the far side of the pressure door."},
        {Voice::archive,"Mechanical access accessible from the Gravemill loader platform."},
        {Voice::partner,"I'll get that far-side release. Tell Neri to wait for both door lights."}}}},
    {"The quiet order","COMMAND LOG COPIED",Set::salvage,{{
        {Voice::lead,"The release is clear. There's a recording in the loader terminal from before the blackout."},
        {Voice::archive,"Emergency priority rejected. Helix classifies civilians outside designated ore operations as a load to shed."},
        {Voice::partner,"Copy the whole log. We'll send it with the distress call from Antenna Spine."}}}},
    {"A line to orbit","RESCUE SHIPS RESPONDING",Set::sky,{{
        {Voice::lead,"The mast is pointed home. Transmitting the roster and landing coordinates."},
        {Voice::aster,"This is rescue cutter Aster. We have your signal. Hold the landing pressure seal until we dock."},
        {Voice::partner,"Aster, we still have to cross Pressure Hull. Keep the channel open."}}}},
    {"Hold the seal","LANDING BARRIER SEALED",Set::gates,{{
        {Voice::lead,"Both seal lights are green. The water is falling on the landing side."},
        {Voice::neri,"We're at the dry platform. There are people here we can't carry up a ladder."},
        {Voice::partner,"Use the level route through Glass Garden. We'll clear it before you move."}}}},
    {"The weight of a seed","CONVOY AT THE SMELTER",Set::evac,{{
        {Voice::lead,"The garden route is clear. One of the crew asked if we could take the seed case."},
        {Voice::neri,"We can carry that. It holds every food crop we managed to grow down here."},
        {Voice::partner,"Keep it close. The smelter doors are the last thing between us and the cutter."}}}},
    {"One light left","CIVILIANS BOARDING ASTER",Set::evac,{{
        {Voice::lead,"The Kiln Titan is down. The families are crossing the last gangway."},
        {Voice::aster,"Aster has the first group aboard. Helix is trying to close the outer bay from the command core."},
        {Voice::partner,"I'll head to the core and give you a clear launch. Neri, count everybody on."}}}},
    {"Vega-9 answers","RESCUE COMPLETE",Set::sky,{{
        {Voice::lead,"Sovereign is offline. Outer bay open. Aster, Vega-9 is yours to approach and depart."},
        {Voice::aster,"All names on the roster accounted for. The seed case is here too. You can come home."},
        {Voice::partner,"Leaving the relay on. If anybody comes back, there'll be a light to follow."}}}}
}};

enum class Angle { establish, arrival, close, detail, reaction, aftermath };
struct Cut { Angle angle; float at; int dialogue; };
inline constexpr std::array cuts{
    Cut{Angle::establish,0,0}, Cut{Angle::arrival,4.4f,0},
    Cut{Angle::close,10.2f,1}, Cut{Angle::detail,15.9f,1},
    Cut{Angle::reaction,21.1f,2}, Cut{Angle::aftermath,26.6f,2}
};
inline constexpr float duration(int index) { return index==sectorCount-1?36.f:33.2f; }
inline constexpr int shotAt(float time) {
    int shot=0;
    while (shot+1<static_cast<int>(cuts.size()) && time>=cuts[shot+1].at) ++shot;
    return shot;
}
enum class Action { drain, relay, recover, unlock, evacuate, launch };
struct Outcome { Action action; float strength; };
inline constexpr std::array<Outcome,sectorCount> actions{{
    {Action::unlock,.62f},{Action::drain,.55f},{Action::recover,.42f},{Action::unlock,.78f},
    {Action::relay,.37f},{Action::relay,.67f},{Action::drain,.78f},{Action::recover,.71f},
    {Action::evacuate,.51f},{Action::evacuate,.72f},{Action::relay,.54f},{Action::relay,.80f},
    {Action::unlock,.91f},{Action::relay,.66f},{Action::recover,.96f},{Action::launch,.58f},
    {Action::drain,1.f},{Action::evacuate,.89f},{Action::evacuate,1.f},{Action::launch,1.f}
}};
inline constexpr std::string_view track(int index) {
    return index==sectorCount-1?"chapter-home":index>=14?"chapter-signal":index>=5?"chapter-depths":"chapter-ember";
}
inline constexpr std::string_view scenery(Set set) {
    if (set==Set::sky) return "cinematic/sky";
    if (set==Set::evac || set==Set::gates) return "cinematic/concourse";
    return "cinematic/relay";
}
}
