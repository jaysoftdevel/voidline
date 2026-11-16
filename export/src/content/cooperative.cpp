// Generated offline by VOIDLINE Model Studio v0.0.17. Replace this original source manually.
#include "content/stages.hpp"
#include <utility>
namespace vl {
const std::array<Stage,cooperativeSectorCount> cooperativeStages=[] {
    std::array<Stage,cooperativeSectorCount> campaign{};
    {
        Stage s{};
        s.name="Split Signal";
        s.art="terminus";
        s.location="Dual-route coordination facility";
        s.threat="HIGH";
        s.accent="#7adfff";
        s.cold="#76b9ff";
        s.briefing="Deploy together. Player 1 climbs to the upper switch route, Player 2 advances below; hold relays to open each other's doors.";
        s.boss="Relay Warden";
        s.biome=static_cast<Biome>(4);
        s.worldWidth=5410.0f;
        s.exit={5298.0f,512.0f};
        s.insertion={100.0f,512.0f};
        s.ground={
          {0.0f,512.0f,5410.0f,100.0f}};
        s.ledges={
          {196.0f,451.0f,103.0f,17.0f},{292.0f,393.0f,107.0f,17.0f},{400.0f,326.0f,600.0f,19.0f},{1000.0f,326.0f,532.0f,19.0f},
          {1600.0f,326.0f,532.0f,19.0f},{2200.0f,326.0f,532.0f,19.0f},{2800.0f,326.0f,532.0f,19.0f},{3400.0f,326.0f,532.0f,19.0f},
          {4000.0f,326.0f,532.0f,19.0f},{4600.0f,326.0f,342.0f,19.0f},{5110.0f,238.0f,140.0f,16.0f},{4930.0f,204.0f,110.0f,16.0f},
          {447.0f,326.0f,144.0f,17.0f},{565.0f,240.0f,120.0f,16.0f},{935.0f,181.0f,116.0f,17.0f},{1535.0f,240.0f,120.0f,16.0f},
          {2160.0f,453.0f,99.0f,17.0f},{2258.0f,395.0f,99.0f,17.0f},{2356.0f,326.0f,103.0f,17.0f},{2387.0f,326.0f,144.0f,17.0f},
          {2505.0f,240.0f,120.0f,16.0f},{2875.0f,181.0f,116.0f,17.0f},{3475.0f,240.0f,120.0f,16.0f},{4100.0f,453.0f,99.0f,17.0f},
          {4198.0f,395.0f,99.0f,17.0f},{4296.0f,326.0f,103.0f,17.0f}};
        s.cover={
          {620.0f,469.0f,44.0f,43.0f},{1390.0f,469.0f,38.0f,43.0f},{2360.0f,469.0f,38.0f,43.0f},{3330.0f,469.0f,38.0f,43.0f},
          {4910.0f,469.0f,45.0f,43.0f}};
        s.beacons={
          {2115.0f,512.0f},{4055.0f,512.0f},{4970.0f,512.0f}};
        s.stairs={
          {{107.0f,512.0f},{225.0f,451.0f}},{{245.0f,451.0f},{324.0f,393.0f}},{{351.0f,393.0f},{429.0f,326.0f}}};
        s.lifts={
          {300.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,0.0f},{1620.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,0.0f},{2940.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,0.0f},{4260.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,0.0f}};
        s.vents={};
        s.enemies={
          {static_cast<EnemyKind>(1),946.0f,512.0f,85.0f,false},{static_cast<EnemyKind>(0),1060.0f,266.0f,0.0f,false},{static_cast<EnemyKind>(6),1938.0f,512.0f,55.0f,false},{static_cast<EnemyKind>(0),2030.0f,266.0f,0.0f,false},
          {static_cast<EnemyKind>(1),2886.0f,512.0f,85.0f,false},{static_cast<EnemyKind>(0),3000.0f,266.0f,0.0f,false},{static_cast<EnemyKind>(6),3878.0f,512.0f,55.0f,true},{static_cast<EnemyKind>(8),3970.0f,266.0f,0.0f,false},
          {static_cast<EnemyKind>(4),5065.0f,512.0f,140.0f,false}};
        s.supplies={
          {static_cast<SupplyKind>(0),160.0f,512.0f},{static_cast<SupplyKind>(2),455.0f,326.0f},{static_cast<SupplyKind>(0),1130.0f,512.0f},{static_cast<SupplyKind>(1),1120.0f,326.0f},
          {static_cast<SupplyKind>(2),2100.0f,512.0f},{static_cast<SupplyKind>(0),3070.0f,512.0f},{static_cast<SupplyKind>(1),3060.0f,326.0f},{static_cast<SupplyKind>(2),4040.0f,512.0f},
          {static_cast<SupplyKind>(2),4865.0f,512.0f}};
        s.crates={
          {static_cast<CrateKind>(0),{1600.0f,512.0f}},{static_cast<CrateKind>(0),{2570.0f,512.0f}},{static_cast<CrateKind>(1),{3540.0f,512.0f}}};
        s.relays={
          {{790.0f,307.0f,52.0f,205.0f},{515.0f,326.0f},0,static_cast<RelayControl>(0)},{{1760.0f,169.0f,52.0f,157.0f},{1485.0f,512.0f},1,static_cast<RelayControl>(1)},{{2730.0f,307.0f,52.0f,205.0f},{2455.0f,326.0f},0,static_cast<RelayControl>(0)},{{3700.0f,169.0f,52.0f,157.0f},{3425.0f,512.0f},1,static_cast<RelayControl>(0)}};
        s.structurePolygons={};
        campaign[0]=std::move(s);
    }
    {
        Stage s{};
        s.name="Pressure Exchange";
        s.art="tideworks";
        s.location="Dual-route coordination facility";
        s.threat="HIGH";
        s.accent="#64f4ce";
        s.cold="#4a9ab8";
        s.briefing="Start together, split at the access lift and alternate the pressure switches as the other operative passes a locked door.";
        s.boss="Flood Sentinel";
        s.biome=static_cast<Biome>(1);
        s.worldWidth=6570.0f;
        s.exit={6458.0f,512.0f};
        s.insertion={100.0f,512.0f};
        s.ground={
          {0.0f,512.0f,6570.0f,100.0f}};
        s.ledges={
          {196.0f,451.0f,103.0f,17.0f},{292.0f,393.0f,107.0f,17.0f},{400.0f,326.0f,600.0f,19.0f},{1000.0f,326.0f,532.0f,19.0f},
          {1600.0f,326.0f,532.0f,19.0f},{2200.0f,326.0f,532.0f,19.0f},{2800.0f,326.0f,532.0f,19.0f},{3400.0f,326.0f,532.0f,19.0f},
          {4000.0f,326.0f,532.0f,19.0f},{4600.0f,326.0f,532.0f,19.0f},{5200.0f,326.0f,532.0f,19.0f},{5800.0f,326.0f,302.0f,19.0f},
          {6270.0f,238.0f,140.0f,16.0f},{6090.0f,204.0f,110.0f,16.0f},{447.0f,326.0f,144.0f,17.0f},{565.0f,240.0f,120.0f,16.0f},
          {1573.0f,240.0f,120.0f,16.0f},{1943.0f,181.0f,116.0f,17.0f},{2198.0f,453.0f,99.0f,17.0f},{2296.0f,395.0f,99.0f,17.0f},
          {2394.0f,326.0f,103.0f,17.0f},{2463.0f,326.0f,144.0f,17.0f},{2581.0f,240.0f,120.0f,16.0f},{3589.0f,240.0f,120.0f,16.0f},
          {3959.0f,181.0f,116.0f,17.0f},{4214.0f,453.0f,99.0f,17.0f},{4312.0f,395.0f,99.0f,17.0f},{4410.0f,326.0f,103.0f,17.0f},
          {4479.0f,326.0f,144.0f,17.0f},{4597.0f,240.0f,120.0f,16.0f}};
        s.cover={
          {620.0f,469.0f,44.0f,43.0f},{1428.0f,469.0f,38.0f,43.0f},{2436.0f,469.0f,38.0f,43.0f},{3444.0f,469.0f,38.0f,43.0f},
          {4452.0f,469.0f,38.0f,43.0f},{6070.0f,469.0f,45.0f,43.0f}};
        s.beacons={
          {2153.0f,512.0f},{4169.0f,512.0f},{6130.0f,512.0f}};
        s.stairs={
          {{107.0f,512.0f},{225.0f,451.0f}},{{245.0f,451.0f},{324.0f,393.0f}},{{351.0f,393.0f},{429.0f,326.0f}}};
        s.lifts={
          {300.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,0.400000006f},{1620.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,0.400000006f},{2940.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,0.400000006f},{4260.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,0.400000006f},
          {5580.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,0.400000006f}};
        s.vents={};
        s.enemies={
          {static_cast<EnemyKind>(1),946.0f,512.0f,85.0f,false},{static_cast<EnemyKind>(0),1060.0f,266.0f,0.0f,false},{static_cast<EnemyKind>(6),1976.0f,512.0f,55.0f,false},{static_cast<EnemyKind>(0),2068.0f,266.0f,0.0f,false},
          {static_cast<EnemyKind>(1),2962.0f,512.0f,85.0f,false},{static_cast<EnemyKind>(0),3076.0f,266.0f,0.0f,false},{static_cast<EnemyKind>(6),3992.0f,512.0f,55.0f,true},{static_cast<EnemyKind>(8),4084.0f,266.0f,0.0f,false},
          {static_cast<EnemyKind>(7),4978.0f,512.0f,85.0f,true},{static_cast<EnemyKind>(8),5092.0f,266.0f,0.0f,true},{static_cast<EnemyKind>(4),6225.0f,512.0f,140.0f,false}};
        s.supplies={
          {static_cast<SupplyKind>(0),160.0f,512.0f},{static_cast<SupplyKind>(2),455.0f,326.0f},{static_cast<SupplyKind>(0),1130.0f,512.0f},{static_cast<SupplyKind>(1),1120.0f,326.0f},
          {static_cast<SupplyKind>(2),2138.0f,512.0f},{static_cast<SupplyKind>(0),3146.0f,512.0f},{static_cast<SupplyKind>(1),3136.0f,326.0f},{static_cast<SupplyKind>(2),4154.0f,512.0f},
          {static_cast<SupplyKind>(0),5162.0f,512.0f},{static_cast<SupplyKind>(1),5152.0f,326.0f},{static_cast<SupplyKind>(2),6025.0f,512.0f}};
        s.crates={
          {static_cast<CrateKind>(0),{1638.0f,512.0f}},{static_cast<CrateKind>(0),{2646.0f,512.0f}},{static_cast<CrateKind>(1),{3654.0f,512.0f}},{static_cast<CrateKind>(0),{4662.0f,512.0f}}};
        s.relays={
          {{790.0f,307.0f,52.0f,205.0f},{515.0f,326.0f},0,static_cast<RelayControl>(0)},{{1798.0f,169.0f,52.0f,157.0f},{1523.0f,512.0f},1,static_cast<RelayControl>(1)},{{2806.0f,307.0f,52.0f,205.0f},{2531.0f,326.0f},0,static_cast<RelayControl>(0)},{{3814.0f,169.0f,52.0f,157.0f},{3539.0f,512.0f},1,static_cast<RelayControl>(0)},
          {{4822.0f,307.0f,52.0f,205.0f},{4547.0f,326.0f},0,static_cast<RelayControl>(1)}};
        s.structurePolygons={};
        campaign[1]=std::move(s);
    }
    {
        Stage s{};
        s.name="Twin Conduits";
        s.art="descent";
        s.location="Dual-route coordination facility";
        s.threat="SEVERE";
        s.accent="#e0b0ff";
        s.cold="#8596f9";
        s.briefing="Separate from the shared insertion bay: one operative routes power overhead while the other clears floor conduits.";
        s.boss="Circuit Regent";
        s.biome=static_cast<Biome>(0);
        s.worldWidth=7806.0f;
        s.exit={7694.0f,512.0f};
        s.insertion={100.0f,512.0f};
        s.ground={
          {0.0f,512.0f,7806.0f,100.0f}};
        s.ledges={
          {196.0f,451.0f,103.0f,17.0f},{292.0f,393.0f,107.0f,17.0f},{400.0f,326.0f,600.0f,19.0f},{1000.0f,326.0f,532.0f,19.0f},
          {1600.0f,326.0f,532.0f,19.0f},{2200.0f,326.0f,532.0f,19.0f},{2800.0f,326.0f,532.0f,19.0f},{3400.0f,326.0f,532.0f,19.0f},
          {4000.0f,326.0f,532.0f,19.0f},{4600.0f,326.0f,532.0f,19.0f},{5200.0f,326.0f,532.0f,19.0f},{5800.0f,326.0f,532.0f,19.0f},
          {6400.0f,326.0f,532.0f,19.0f},{7000.0f,326.0f,338.0f,19.0f},{7506.0f,238.0f,140.0f,16.0f},{7326.0f,204.0f,110.0f,16.0f},
          {447.0f,326.0f,144.0f,17.0f},{565.0f,240.0f,120.0f,16.0f},{935.0f,181.0f,116.0f,17.0f},{1611.0f,240.0f,120.0f,16.0f},
          {2236.0f,453.0f,99.0f,17.0f},{2334.0f,395.0f,99.0f,17.0f},{2432.0f,326.0f,103.0f,17.0f},{2539.0f,326.0f,144.0f,17.0f},
          {2657.0f,240.0f,120.0f,16.0f},{3027.0f,181.0f,116.0f,17.0f},{3703.0f,240.0f,120.0f,16.0f},{4328.0f,453.0f,99.0f,17.0f},
          {4426.0f,395.0f,99.0f,17.0f},{4524.0f,326.0f,103.0f,17.0f},{4631.0f,326.0f,144.0f,17.0f},{4749.0f,240.0f,120.0f,16.0f},
          {5119.0f,181.0f,116.0f,17.0f},{5795.0f,240.0f,120.0f,16.0f},{6420.0f,453.0f,99.0f,17.0f},{6518.0f,395.0f,99.0f,17.0f},
          {6616.0f,326.0f,103.0f,17.0f}};
        s.cover={
          {620.0f,469.0f,44.0f,43.0f},{1466.0f,469.0f,38.0f,43.0f},{2512.0f,469.0f,38.0f,43.0f},{3558.0f,469.0f,38.0f,43.0f},
          {4604.0f,469.0f,38.0f,43.0f},{5650.0f,469.0f,38.0f,43.0f},{7306.0f,469.0f,45.0f,43.0f}};
        s.beacons={
          {2191.0f,512.0f},{4283.0f,512.0f},{6375.0f,512.0f},{7366.0f,512.0f}};
        s.stairs={
          {{107.0f,512.0f},{225.0f,451.0f}},{{245.0f,451.0f},{324.0f,393.0f}},{{351.0f,393.0f},{429.0f,326.0f}}};
        s.lifts={
          {300.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,0.800000012f},{1620.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,0.800000012f},{2940.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,0.800000012f},{4260.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,0.800000012f},
          {5580.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,0.800000012f},{6900.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,0.800000012f},{1185.0f,423.0f,108.0f,34.0f,-95.0f,3.89999986f,0.0f},{3277.0f,423.0f,108.0f,34.0f,-95.0f,3.89999986f,1.10000002f},
          {5369.0f,423.0f,108.0f,34.0f,-95.0f,3.89999986f,2.20000005f}};
        s.vents={
          {2296.0f,512.0f,0.400000006f},{5434.0f,512.0f,1.60000002f}};
        s.enemies={
          {static_cast<EnemyKind>(1),946.0f,512.0f,85.0f,false},{static_cast<EnemyKind>(3),1210.0f,512.0f,65.0f,false},{static_cast<EnemyKind>(0),1060.0f,266.0f,0.0f,false},{static_cast<EnemyKind>(6),2014.0f,512.0f,55.0f,false},
          {static_cast<EnemyKind>(5),2256.0f,512.0f,65.0f,false},{static_cast<EnemyKind>(0),2106.0f,266.0f,0.0f,false},{static_cast<EnemyKind>(1),3038.0f,512.0f,85.0f,false},{static_cast<EnemyKind>(5),3302.0f,512.0f,65.0f,false},
          {static_cast<EnemyKind>(0),3152.0f,266.0f,0.0f,false},{static_cast<EnemyKind>(6),4106.0f,512.0f,55.0f,true},{static_cast<EnemyKind>(3),4348.0f,512.0f,65.0f,false},{static_cast<EnemyKind>(8),4198.0f,266.0f,0.0f,false},
          {static_cast<EnemyKind>(7),5130.0f,512.0f,85.0f,true},{static_cast<EnemyKind>(5),5394.0f,512.0f,65.0f,true},{static_cast<EnemyKind>(8),5244.0f,266.0f,0.0f,true},{static_cast<EnemyKind>(6),6198.0f,512.0f,55.0f,true},
          {static_cast<EnemyKind>(5),6440.0f,512.0f,65.0f,true},{static_cast<EnemyKind>(8),6290.0f,266.0f,0.0f,true},{static_cast<EnemyKind>(4),7461.0f,512.0f,140.0f,false}};
        s.supplies={
          {static_cast<SupplyKind>(0),160.0f,512.0f},{static_cast<SupplyKind>(2),455.0f,326.0f},{static_cast<SupplyKind>(0),1130.0f,512.0f},{static_cast<SupplyKind>(1),1120.0f,326.0f},
          {static_cast<SupplyKind>(2),2176.0f,512.0f},{static_cast<SupplyKind>(0),3222.0f,512.0f},{static_cast<SupplyKind>(1),3212.0f,326.0f},{static_cast<SupplyKind>(2),4268.0f,512.0f},
          {static_cast<SupplyKind>(0),5314.0f,512.0f},{static_cast<SupplyKind>(1),5304.0f,326.0f},{static_cast<SupplyKind>(2),6360.0f,512.0f},{static_cast<SupplyKind>(2),7261.0f,512.0f}};
        s.crates={
          {static_cast<CrateKind>(0),{1676.0f,512.0f}},{static_cast<CrateKind>(0),{2722.0f,512.0f}},{static_cast<CrateKind>(1),{3768.0f,512.0f}},{static_cast<CrateKind>(0),{4814.0f,512.0f}},
          {static_cast<CrateKind>(0),{5860.0f,512.0f}}};
        s.relays={
          {{790.0f,307.0f,52.0f,205.0f},{515.0f,326.0f},0,static_cast<RelayControl>(0)},{{1836.0f,169.0f,52.0f,157.0f},{1561.0f,512.0f},1,static_cast<RelayControl>(1)},{{2882.0f,307.0f,52.0f,205.0f},{2607.0f,326.0f},0,static_cast<RelayControl>(0)},{{3928.0f,169.0f,52.0f,157.0f},{3653.0f,512.0f},1,static_cast<RelayControl>(0)},
          {{4974.0f,307.0f,52.0f,205.0f},{4699.0f,326.0f},0,static_cast<RelayControl>(1)},{{6020.0f,169.0f,52.0f,157.0f},{5745.0f,512.0f},1,static_cast<RelayControl>(0)}};
        s.structurePolygons={};
        campaign[2]=std::move(s);
    }
    {
        Stage s{};
        s.name="Resonance Bridge";
        s.art="foundry";
        s.location="Dual-route coordination facility";
        s.threat="SEVERE";
        s.accent="#ffbc75";
        s.cold="#e4a95c";
        s.briefing="Climb from the shared starting deck into separate gantries. Trade switch and gate duties at every relay.";
        s.boss="Gantry Marshal";
        s.biome=static_cast<Biome>(3);
        s.worldWidth=9118.0f;
        s.exit={9006.0f,512.0f};
        s.insertion={100.0f,512.0f};
        s.ground={
          {0.0f,512.0f,9118.0f,100.0f}};
        s.ledges={
          {196.0f,451.0f,103.0f,17.0f},{292.0f,393.0f,107.0f,17.0f},{400.0f,326.0f,600.0f,19.0f},{1000.0f,326.0f,532.0f,19.0f},
          {1600.0f,326.0f,532.0f,19.0f},{2200.0f,326.0f,532.0f,19.0f},{2800.0f,326.0f,532.0f,19.0f},{3400.0f,326.0f,532.0f,19.0f},
          {4000.0f,326.0f,532.0f,19.0f},{4600.0f,326.0f,532.0f,19.0f},{5200.0f,326.0f,532.0f,19.0f},{5800.0f,326.0f,532.0f,19.0f},
          {6400.0f,326.0f,532.0f,19.0f},{7000.0f,326.0f,532.0f,19.0f},{7600.0f,326.0f,532.0f,19.0f},{8200.0f,326.0f,450.0f,19.0f},
          {8818.0f,238.0f,140.0f,16.0f},{8638.0f,204.0f,110.0f,16.0f},{447.0f,326.0f,144.0f,17.0f},{565.0f,240.0f,120.0f,16.0f},
          {1649.0f,240.0f,120.0f,16.0f},{2019.0f,181.0f,116.0f,17.0f},{2274.0f,453.0f,99.0f,17.0f},{2372.0f,395.0f,99.0f,17.0f},
          {2470.0f,326.0f,103.0f,17.0f},{2615.0f,326.0f,144.0f,17.0f},{2733.0f,240.0f,120.0f,16.0f},{3817.0f,240.0f,120.0f,16.0f},
          {4187.0f,181.0f,116.0f,17.0f},{4442.0f,453.0f,99.0f,17.0f},{4540.0f,395.0f,99.0f,17.0f},{4638.0f,326.0f,103.0f,17.0f},
          {4783.0f,326.0f,144.0f,17.0f},{4901.0f,240.0f,120.0f,16.0f},{5985.0f,240.0f,120.0f,16.0f},{6355.0f,181.0f,116.0f,17.0f},
          {6610.0f,453.0f,99.0f,17.0f},{6708.0f,395.0f,99.0f,17.0f},{6806.0f,326.0f,103.0f,17.0f},{6951.0f,326.0f,144.0f,17.0f},
          {7069.0f,240.0f,120.0f,16.0f}};
        s.cover={
          {620.0f,469.0f,44.0f,43.0f},{1504.0f,469.0f,38.0f,43.0f},{2588.0f,469.0f,38.0f,43.0f},{3672.0f,469.0f,38.0f,43.0f},
          {4756.0f,469.0f,38.0f,43.0f},{5840.0f,469.0f,38.0f,43.0f},{6924.0f,469.0f,38.0f,43.0f},{8618.0f,469.0f,45.0f,43.0f}};
        s.beacons={
          {2229.0f,512.0f},{4397.0f,512.0f},{6565.0f,512.0f},{8678.0f,512.0f}};
        s.stairs={
          {{107.0f,512.0f},{225.0f,451.0f}},{{245.0f,451.0f},{324.0f,393.0f}},{{351.0f,393.0f},{429.0f,326.0f}}};
        s.lifts={
          {300.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,1.20000005f},{1620.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,1.20000005f},{2940.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,1.20000005f},{4260.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,1.20000005f},
          {5580.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,1.20000005f},{6900.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,1.20000005f},{8220.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,1.20000005f},{1185.0f,423.0f,108.0f,34.0f,-95.0f,4.04999971f,0.0f},
          {3353.0f,423.0f,108.0f,34.0f,-95.0f,4.04999971f,1.10000002f},{5521.0f,423.0f,108.0f,34.0f,-95.0f,4.04999971f,2.20000005f},{7689.0f,423.0f,108.0f,34.0f,-95.0f,4.04999971f,3.30000019f}};
        s.vents={
          {2334.0f,512.0f,0.400000006f},{5586.0f,512.0f,1.60000002f}};
        s.enemies={
          {static_cast<EnemyKind>(1),946.0f,512.0f,85.0f,false},{static_cast<EnemyKind>(3),1210.0f,512.0f,65.0f,false},{static_cast<EnemyKind>(0),1060.0f,266.0f,0.0f,false},{static_cast<EnemyKind>(6),2052.0f,512.0f,55.0f,false},
          {static_cast<EnemyKind>(5),2294.0f,512.0f,65.0f,false},{static_cast<EnemyKind>(0),2144.0f,266.0f,0.0f,false},{static_cast<EnemyKind>(1),3114.0f,512.0f,85.0f,false},{static_cast<EnemyKind>(5),3378.0f,512.0f,65.0f,false},
          {static_cast<EnemyKind>(0),3228.0f,266.0f,0.0f,false},{static_cast<EnemyKind>(6),4220.0f,512.0f,55.0f,true},{static_cast<EnemyKind>(3),4462.0f,512.0f,65.0f,false},{static_cast<EnemyKind>(8),4312.0f,266.0f,0.0f,false},
          {static_cast<EnemyKind>(7),5282.0f,512.0f,85.0f,true},{static_cast<EnemyKind>(5),5546.0f,512.0f,65.0f,true},{static_cast<EnemyKind>(8),5396.0f,266.0f,0.0f,true},{static_cast<EnemyKind>(6),6388.0f,512.0f,55.0f,true},
          {static_cast<EnemyKind>(5),6630.0f,512.0f,65.0f,true},{static_cast<EnemyKind>(8),6480.0f,266.0f,0.0f,true},{static_cast<EnemyKind>(7),7450.0f,512.0f,85.0f,true},{static_cast<EnemyKind>(3),7714.0f,512.0f,65.0f,true},
          {static_cast<EnemyKind>(8),7564.0f,266.0f,0.0f,true},{static_cast<EnemyKind>(4),8773.0f,512.0f,140.0f,false}};
        s.supplies={
          {static_cast<SupplyKind>(0),160.0f,512.0f},{static_cast<SupplyKind>(2),455.0f,326.0f},{static_cast<SupplyKind>(0),1130.0f,512.0f},{static_cast<SupplyKind>(1),1120.0f,326.0f},
          {static_cast<SupplyKind>(2),2214.0f,512.0f},{static_cast<SupplyKind>(0),3298.0f,512.0f},{static_cast<SupplyKind>(1),3288.0f,326.0f},{static_cast<SupplyKind>(2),4382.0f,512.0f},
          {static_cast<SupplyKind>(0),5466.0f,512.0f},{static_cast<SupplyKind>(1),5456.0f,326.0f},{static_cast<SupplyKind>(2),6550.0f,512.0f},{static_cast<SupplyKind>(0),7634.0f,512.0f},
          {static_cast<SupplyKind>(1),7624.0f,326.0f},{static_cast<SupplyKind>(2),8573.0f,512.0f}};
        s.crates={
          {static_cast<CrateKind>(0),{1714.0f,512.0f}},{static_cast<CrateKind>(0),{2798.0f,512.0f}},{static_cast<CrateKind>(1),{3882.0f,512.0f}},{static_cast<CrateKind>(0),{4966.0f,512.0f}},
          {static_cast<CrateKind>(0),{6050.0f,512.0f}},{static_cast<CrateKind>(1),{7134.0f,512.0f}}};
        s.relays={
          {{790.0f,307.0f,52.0f,205.0f},{515.0f,326.0f},0,static_cast<RelayControl>(0)},{{1874.0f,169.0f,52.0f,157.0f},{1599.0f,512.0f},1,static_cast<RelayControl>(1)},{{2958.0f,307.0f,52.0f,205.0f},{2683.0f,326.0f},0,static_cast<RelayControl>(0)},{{4042.0f,169.0f,52.0f,157.0f},{3767.0f,512.0f},1,static_cast<RelayControl>(0)},
          {{5126.0f,307.0f,52.0f,205.0f},{4851.0f,326.0f},0,static_cast<RelayControl>(1)},{{6210.0f,169.0f,52.0f,157.0f},{5935.0f,512.0f},1,static_cast<RelayControl>(0)},{{7294.0f,307.0f,52.0f,205.0f},{7019.0f,326.0f},0,static_cast<RelayControl>(0)}};
        s.structurePolygons={};
        campaign[3]=std::move(s);
    }
    {
        Stage s{};
        s.name="Reactor Duplex";
        s.art="tideworks";
        s.location="Dual-route coordination facility";
        s.threat="CRITICAL";
        s.accent="#f8856b";
        s.cold="#9b89dd";
        s.briefing="Start side by side, then coordinate upper coolant controls with floor pressure doors while fighting the guard patrols.";
        s.boss="Core Guardian";
        s.biome=static_cast<Biome>(1);
        s.worldWidth=10506.0f;
        s.exit={10394.0f,512.0f};
        s.insertion={100.0f,512.0f};
        s.ground={
          {0.0f,512.0f,10506.0f,100.0f}};
        s.ledges={
          {196.0f,451.0f,103.0f,17.0f},{292.0f,393.0f,107.0f,17.0f},{400.0f,326.0f,600.0f,19.0f},{1000.0f,326.0f,532.0f,19.0f},
          {1600.0f,326.0f,532.0f,19.0f},{2200.0f,326.0f,532.0f,19.0f},{2800.0f,326.0f,532.0f,19.0f},{3400.0f,326.0f,532.0f,19.0f},
          {4000.0f,326.0f,532.0f,19.0f},{4600.0f,326.0f,532.0f,19.0f},{5200.0f,326.0f,532.0f,19.0f},{5800.0f,326.0f,532.0f,19.0f},
          {6400.0f,326.0f,532.0f,19.0f},{7000.0f,326.0f,532.0f,19.0f},{7600.0f,326.0f,532.0f,19.0f},{8200.0f,326.0f,532.0f,19.0f},
          {8800.0f,326.0f,532.0f,19.0f},{9400.0f,326.0f,532.0f,19.0f},{10000.0f,326.0f,38.0f,19.0f},{10206.0f,238.0f,140.0f,16.0f},
          {10026.0f,204.0f,110.0f,16.0f},{447.0f,326.0f,144.0f,17.0f},{565.0f,240.0f,120.0f,16.0f},{935.0f,181.0f,116.0f,17.0f},
          {1687.0f,240.0f,120.0f,16.0f},{2312.0f,453.0f,99.0f,17.0f},{2410.0f,395.0f,99.0f,17.0f},{2508.0f,326.0f,103.0f,17.0f},
          {2691.0f,326.0f,144.0f,17.0f},{2809.0f,240.0f,120.0f,16.0f},{3179.0f,181.0f,116.0f,17.0f},{3931.0f,240.0f,120.0f,16.0f},
          {4556.0f,453.0f,99.0f,17.0f},{4654.0f,395.0f,99.0f,17.0f},{4752.0f,326.0f,103.0f,17.0f},{4935.0f,326.0f,144.0f,17.0f},
          {5053.0f,240.0f,120.0f,16.0f},{5423.0f,181.0f,116.0f,17.0f},{6175.0f,240.0f,120.0f,16.0f},{6800.0f,453.0f,99.0f,17.0f},
          {6898.0f,395.0f,99.0f,17.0f},{6996.0f,326.0f,103.0f,17.0f},{7179.0f,326.0f,144.0f,17.0f},{7297.0f,240.0f,120.0f,16.0f},
          {7667.0f,181.0f,116.0f,17.0f},{8419.0f,240.0f,120.0f,16.0f},{9044.0f,453.0f,99.0f,17.0f},{9142.0f,395.0f,99.0f,17.0f},
          {9240.0f,326.0f,103.0f,17.0f}};
        s.cover={
          {620.0f,469.0f,44.0f,43.0f},{1542.0f,469.0f,38.0f,43.0f},{2664.0f,469.0f,38.0f,43.0f},{3786.0f,469.0f,38.0f,43.0f},
          {4908.0f,469.0f,38.0f,43.0f},{6030.0f,469.0f,38.0f,43.0f},{7152.0f,469.0f,38.0f,43.0f},{8274.0f,469.0f,38.0f,43.0f},
          {10006.0f,469.0f,45.0f,43.0f}};
        s.beacons={
          {2267.0f,512.0f},{4511.0f,512.0f},{6755.0f,512.0f},{8999.0f,512.0f},
          {10066.0f,512.0f}};
        s.stairs={
          {{107.0f,512.0f},{225.0f,451.0f}},{{245.0f,451.0f},{324.0f,393.0f}},{{351.0f,393.0f},{429.0f,326.0f}}};
        s.lifts={
          {300.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,1.60000002f},{1620.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,1.60000002f},{2940.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,1.60000002f},{4260.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,1.60000002f},
          {5580.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,1.60000002f},{6900.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,1.60000002f},{8220.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,1.60000002f},{9540.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,1.60000002f},
          {1185.0f,423.0f,108.0f,34.0f,-95.0f,4.19999981f,0.0f},{3429.0f,423.0f,108.0f,34.0f,-95.0f,4.19999981f,1.10000002f},{5673.0f,423.0f,108.0f,34.0f,-95.0f,4.19999981f,2.20000005f},{7917.0f,423.0f,108.0f,34.0f,-95.0f,4.19999981f,3.30000019f}};
        s.vents={
          {2372.0f,512.0f,0.400000006f},{5738.0f,512.0f,1.60000002f},{9104.0f,512.0f,2.79999995f}};
        s.enemies={
          {static_cast<EnemyKind>(1),946.0f,512.0f,85.0f,false},{static_cast<EnemyKind>(3),1210.0f,512.0f,65.0f,false},{static_cast<EnemyKind>(0),1060.0f,266.0f,0.0f,false},{static_cast<EnemyKind>(6),2090.0f,512.0f,55.0f,false},
          {static_cast<EnemyKind>(5),2332.0f,512.0f,65.0f,false},{static_cast<EnemyKind>(0),2182.0f,266.0f,0.0f,false},{static_cast<EnemyKind>(1),3190.0f,512.0f,85.0f,false},{static_cast<EnemyKind>(5),3454.0f,512.0f,65.0f,false},
          {static_cast<EnemyKind>(0),3304.0f,266.0f,0.0f,false},{static_cast<EnemyKind>(6),4334.0f,512.0f,55.0f,true},{static_cast<EnemyKind>(3),4576.0f,512.0f,65.0f,false},{static_cast<EnemyKind>(8),4426.0f,266.0f,0.0f,false},
          {static_cast<EnemyKind>(7),5434.0f,512.0f,85.0f,true},{static_cast<EnemyKind>(5),5698.0f,512.0f,65.0f,true},{static_cast<EnemyKind>(8),5548.0f,266.0f,0.0f,true},{static_cast<EnemyKind>(6),6578.0f,512.0f,55.0f,true},
          {static_cast<EnemyKind>(5),6820.0f,512.0f,65.0f,true},{static_cast<EnemyKind>(8),6670.0f,266.0f,0.0f,true},{static_cast<EnemyKind>(7),7678.0f,512.0f,85.0f,true},{static_cast<EnemyKind>(3),7942.0f,512.0f,65.0f,true},
          {static_cast<EnemyKind>(8),7792.0f,266.0f,0.0f,true},{static_cast<EnemyKind>(6),8822.0f,512.0f,55.0f,true},{static_cast<EnemyKind>(5),9064.0f,512.0f,65.0f,true},{static_cast<EnemyKind>(8),8914.0f,266.0f,0.0f,true},
          {static_cast<EnemyKind>(4),10161.0f,512.0f,140.0f,true}};
        s.supplies={
          {static_cast<SupplyKind>(0),160.0f,512.0f},{static_cast<SupplyKind>(2),455.0f,326.0f},{static_cast<SupplyKind>(0),1130.0f,512.0f},{static_cast<SupplyKind>(1),1120.0f,326.0f},
          {static_cast<SupplyKind>(2),2252.0f,512.0f},{static_cast<SupplyKind>(0),3374.0f,512.0f},{static_cast<SupplyKind>(1),3364.0f,326.0f},{static_cast<SupplyKind>(2),4496.0f,512.0f},
          {static_cast<SupplyKind>(0),5618.0f,512.0f},{static_cast<SupplyKind>(1),5608.0f,326.0f},{static_cast<SupplyKind>(2),6740.0f,512.0f},{static_cast<SupplyKind>(0),7862.0f,512.0f},
          {static_cast<SupplyKind>(1),7852.0f,326.0f},{static_cast<SupplyKind>(2),8984.0f,512.0f},{static_cast<SupplyKind>(2),9961.0f,512.0f}};
        s.crates={
          {static_cast<CrateKind>(0),{1752.0f,512.0f}},{static_cast<CrateKind>(0),{2874.0f,512.0f}},{static_cast<CrateKind>(1),{3996.0f,512.0f}},{static_cast<CrateKind>(0),{5118.0f,512.0f}},
          {static_cast<CrateKind>(0),{6240.0f,512.0f}},{static_cast<CrateKind>(1),{7362.0f,512.0f}},{static_cast<CrateKind>(0),{8484.0f,512.0f}}};
        s.relays={
          {{790.0f,307.0f,52.0f,205.0f},{515.0f,326.0f},0,static_cast<RelayControl>(0)},{{1912.0f,169.0f,52.0f,157.0f},{1637.0f,512.0f},1,static_cast<RelayControl>(1)},{{3034.0f,307.0f,52.0f,205.0f},{2759.0f,326.0f},0,static_cast<RelayControl>(0)},{{4156.0f,169.0f,52.0f,157.0f},{3881.0f,512.0f},1,static_cast<RelayControl>(0)},
          {{5278.0f,307.0f,52.0f,205.0f},{5003.0f,326.0f},0,static_cast<RelayControl>(1)},{{6400.0f,169.0f,52.0f,157.0f},{6125.0f,512.0f},1,static_cast<RelayControl>(0)},{{7522.0f,307.0f,52.0f,205.0f},{7247.0f,326.0f},0,static_cast<RelayControl>(0)},{{8644.0f,169.0f,52.0f,157.0f},{8369.0f,512.0f},1,static_cast<RelayControl>(1)}};
        s.structurePolygons={};
        campaign[4]=std::move(s);
    }
    {
        Stage s{};
        s.name="The Last Relay";
        s.art="terminus";
        s.location="Dual-route coordination facility";
        s.threat="CRITICAL";
        s.accent="#da91ff";
        s.cold="#8ae7ff";
        s.briefing="Deploy as a pair and split routes: each operative must hold alternating uplink switches to guide the other forward.";
        s.boss="Helix Overseer";
        s.biome=static_cast<Biome>(4);
        s.worldWidth=11970.0f;
        s.exit={11858.0f,512.0f};
        s.insertion={100.0f,512.0f};
        s.ground={
          {0.0f,512.0f,11970.0f,100.0f}};
        s.ledges={
          {196.0f,451.0f,103.0f,17.0f},{292.0f,393.0f,107.0f,17.0f},{400.0f,326.0f,600.0f,19.0f},{1000.0f,326.0f,532.0f,19.0f},
          {1600.0f,326.0f,532.0f,19.0f},{2200.0f,326.0f,532.0f,19.0f},{2800.0f,326.0f,532.0f,19.0f},{3400.0f,326.0f,532.0f,19.0f},
          {4000.0f,326.0f,532.0f,19.0f},{4600.0f,326.0f,532.0f,19.0f},{5200.0f,326.0f,532.0f,19.0f},{5800.0f,326.0f,532.0f,19.0f},
          {6400.0f,326.0f,532.0f,19.0f},{7000.0f,326.0f,532.0f,19.0f},{7600.0f,326.0f,532.0f,19.0f},{8200.0f,326.0f,532.0f,19.0f},
          {8800.0f,326.0f,532.0f,19.0f},{9400.0f,326.0f,532.0f,19.0f},{10000.0f,326.0f,532.0f,19.0f},{10600.0f,326.0f,532.0f,19.0f},
          {11200.0f,326.0f,302.0f,19.0f},{11670.0f,238.0f,140.0f,16.0f},{11490.0f,204.0f,110.0f,16.0f},{447.0f,326.0f,144.0f,17.0f},
          {565.0f,240.0f,120.0f,16.0f},{1725.0f,240.0f,120.0f,16.0f},{2095.0f,181.0f,116.0f,17.0f},{2350.0f,453.0f,99.0f,17.0f},
          {2448.0f,395.0f,99.0f,17.0f},{2546.0f,326.0f,103.0f,17.0f},{2767.0f,326.0f,144.0f,17.0f},{2885.0f,240.0f,120.0f,16.0f},
          {4045.0f,240.0f,120.0f,16.0f},{4415.0f,181.0f,116.0f,17.0f},{4670.0f,453.0f,99.0f,17.0f},{4768.0f,395.0f,99.0f,17.0f},
          {4866.0f,326.0f,103.0f,17.0f},{5087.0f,326.0f,144.0f,17.0f},{5205.0f,240.0f,120.0f,16.0f},{6365.0f,240.0f,120.0f,16.0f},
          {6735.0f,181.0f,116.0f,17.0f},{6990.0f,453.0f,99.0f,17.0f},{7088.0f,395.0f,99.0f,17.0f},{7186.0f,326.0f,103.0f,17.0f},
          {7407.0f,326.0f,144.0f,17.0f},{7525.0f,240.0f,120.0f,16.0f},{8685.0f,240.0f,120.0f,16.0f},{9055.0f,181.0f,116.0f,17.0f},
          {9310.0f,453.0f,99.0f,17.0f},{9408.0f,395.0f,99.0f,17.0f},{9506.0f,326.0f,103.0f,17.0f},{9727.0f,326.0f,144.0f,17.0f},
          {9845.0f,240.0f,120.0f,16.0f}};
        s.cover={
          {620.0f,469.0f,44.0f,43.0f},{1580.0f,469.0f,38.0f,43.0f},{2740.0f,469.0f,38.0f,43.0f},{3900.0f,469.0f,38.0f,43.0f},
          {5060.0f,469.0f,38.0f,43.0f},{6220.0f,469.0f,38.0f,43.0f},{7380.0f,469.0f,38.0f,43.0f},{8540.0f,469.0f,38.0f,43.0f},
          {9700.0f,469.0f,38.0f,43.0f},{11470.0f,469.0f,45.0f,43.0f}};
        s.beacons={
          {2305.0f,512.0f},{4625.0f,512.0f},{6945.0f,512.0f},{9265.0f,512.0f},
          {11530.0f,512.0f}};
        s.stairs={
          {{107.0f,512.0f},{225.0f,451.0f}},{{245.0f,451.0f},{324.0f,393.0f}},{{351.0f,393.0f},{429.0f,326.0f}}};
        s.lifts={
          {300.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,2.0f},{1620.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,2.0f},{2940.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,2.0f},{4260.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,2.0f},
          {5580.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,2.0f},{6900.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,2.0f},{8220.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,2.0f},{9540.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,2.0f},
          {10860.0f,485.0f,93.0f,0.0f,-167.0f,5.0999999f,2.0f},{1185.0f,423.0f,108.0f,34.0f,-95.0f,4.3499999f,0.0f},{3505.0f,423.0f,108.0f,34.0f,-95.0f,4.3499999f,1.10000002f},{5825.0f,423.0f,108.0f,34.0f,-95.0f,4.3499999f,2.20000005f},
          {8145.0f,423.0f,108.0f,34.0f,-95.0f,4.3499999f,3.30000019f},{10465.0f,423.0f,108.0f,34.0f,-95.0f,4.3499999f,4.4000001f}};
        s.vents={
          {2410.0f,512.0f,0.400000006f},{5890.0f,512.0f,1.60000002f},{9370.0f,512.0f,2.79999995f}};
        s.enemies={
          {static_cast<EnemyKind>(1),946.0f,512.0f,85.0f,false},{static_cast<EnemyKind>(3),1210.0f,512.0f,65.0f,false},{static_cast<EnemyKind>(0),1060.0f,266.0f,0.0f,false},{static_cast<EnemyKind>(6),2128.0f,512.0f,55.0f,false},
          {static_cast<EnemyKind>(5),2370.0f,512.0f,65.0f,false},{static_cast<EnemyKind>(0),2220.0f,266.0f,0.0f,false},{static_cast<EnemyKind>(1),3266.0f,512.0f,85.0f,false},{static_cast<EnemyKind>(5),3530.0f,512.0f,65.0f,false},
          {static_cast<EnemyKind>(0),3380.0f,266.0f,0.0f,false},{static_cast<EnemyKind>(6),4448.0f,512.0f,55.0f,true},{static_cast<EnemyKind>(3),4690.0f,512.0f,65.0f,false},{static_cast<EnemyKind>(8),4540.0f,266.0f,0.0f,false},
          {static_cast<EnemyKind>(7),5586.0f,512.0f,85.0f,true},{static_cast<EnemyKind>(5),5850.0f,512.0f,65.0f,true},{static_cast<EnemyKind>(8),5700.0f,266.0f,0.0f,true},{static_cast<EnemyKind>(6),6768.0f,512.0f,55.0f,true},
          {static_cast<EnemyKind>(5),7010.0f,512.0f,65.0f,true},{static_cast<EnemyKind>(8),6860.0f,266.0f,0.0f,true},{static_cast<EnemyKind>(7),7906.0f,512.0f,85.0f,true},{static_cast<EnemyKind>(3),8170.0f,512.0f,65.0f,true},
          {static_cast<EnemyKind>(8),8020.0f,266.0f,0.0f,true},{static_cast<EnemyKind>(6),9088.0f,512.0f,55.0f,true},{static_cast<EnemyKind>(5),9330.0f,512.0f,65.0f,true},{static_cast<EnemyKind>(8),9180.0f,266.0f,0.0f,true},
          {static_cast<EnemyKind>(7),10226.0f,512.0f,85.0f,true},{static_cast<EnemyKind>(5),10490.0f,512.0f,65.0f,true},{static_cast<EnemyKind>(8),10340.0f,266.0f,0.0f,true},{static_cast<EnemyKind>(4),11625.0f,512.0f,140.0f,true}};
        s.supplies={
          {static_cast<SupplyKind>(0),160.0f,512.0f},{static_cast<SupplyKind>(2),455.0f,326.0f},{static_cast<SupplyKind>(0),1130.0f,512.0f},{static_cast<SupplyKind>(1),1120.0f,326.0f},
          {static_cast<SupplyKind>(2),2290.0f,512.0f},{static_cast<SupplyKind>(0),3450.0f,512.0f},{static_cast<SupplyKind>(1),3440.0f,326.0f},{static_cast<SupplyKind>(2),4610.0f,512.0f},
          {static_cast<SupplyKind>(0),5770.0f,512.0f},{static_cast<SupplyKind>(1),5760.0f,326.0f},{static_cast<SupplyKind>(2),6930.0f,512.0f},{static_cast<SupplyKind>(0),8090.0f,512.0f},
          {static_cast<SupplyKind>(1),8080.0f,326.0f},{static_cast<SupplyKind>(2),9250.0f,512.0f},{static_cast<SupplyKind>(0),10410.0f,512.0f},{static_cast<SupplyKind>(1),10400.0f,326.0f},
          {static_cast<SupplyKind>(2),11425.0f,512.0f}};
        s.crates={
          {static_cast<CrateKind>(0),{1790.0f,512.0f}},{static_cast<CrateKind>(0),{2950.0f,512.0f}},{static_cast<CrateKind>(1),{4110.0f,512.0f}},{static_cast<CrateKind>(0),{5270.0f,512.0f}},
          {static_cast<CrateKind>(0),{6430.0f,512.0f}},{static_cast<CrateKind>(1),{7590.0f,512.0f}},{static_cast<CrateKind>(0),{8750.0f,512.0f}},{static_cast<CrateKind>(0),{9910.0f,512.0f}}};
        s.relays={
          {{790.0f,307.0f,52.0f,205.0f},{515.0f,326.0f},0,static_cast<RelayControl>(0)},{{1950.0f,169.0f,52.0f,157.0f},{1675.0f,512.0f},1,static_cast<RelayControl>(1)},{{3110.0f,307.0f,52.0f,205.0f},{2835.0f,326.0f},0,static_cast<RelayControl>(0)},{{4270.0f,169.0f,52.0f,157.0f},{3995.0f,512.0f},1,static_cast<RelayControl>(0)},
          {{5430.0f,307.0f,52.0f,205.0f},{5155.0f,326.0f},0,static_cast<RelayControl>(1)},{{6590.0f,169.0f,52.0f,157.0f},{6315.0f,512.0f},1,static_cast<RelayControl>(0)},{{7750.0f,307.0f,52.0f,205.0f},{7475.0f,326.0f},0,static_cast<RelayControl>(0)},{{8910.0f,169.0f,52.0f,157.0f},{8635.0f,512.0f},1,static_cast<RelayControl>(1)},
          {{10070.0f,307.0f,52.0f,205.0f},{9795.0f,326.0f},0,static_cast<RelayControl>(0)}};
        s.structurePolygons={};
        campaign[5]=std::move(s);
    }
    return campaign;
}();
}
