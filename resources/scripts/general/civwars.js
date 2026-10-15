// Civilization Wars prototype: resources, terrain dependent production and construction by infantry.
// All values of the prototype are collected here for easy tuning.
var CIVWARS =
{
    ruleId : "GAMERULE_CIVILIZATION_WARS",
    // player variable written by the host of a network match: 1 for a seat of a human, 2 for a seat of an ai
    seatTypeVariable : "COW_SEAT_TYPE",

    // ------------------------------------------------------------------------------------------------
    // resources
    // ------------------------------------------------------------------------------------------------
    food : "FOOD",
    metal : "METAL",
    oil : "OIL",
    resources : ["FOOD", "METAL", "OIL"],
    resourceNames : [qsTr("Food"), qsTr("Metal"), qsTr("Oil")],
    resourceShortNames : [qsTr("F"), qsTr("M"), qsTr("O")],
    // Turns to produce a unit by its base costs: [costs up to, turns]. A unit with 1 turn can move in the turn after
    // it was ordered like in a normal match, each further turn keeps it one more turn in its factory.
    productionTurns : [[2999, 1], [8999, 2], [17999, 3], [9999999, 4]],
    // turns of single units, which replace the ones by their costs e.g. { "MECH" : 1, "BOMBER" : 5 }
    productionTurnsUnits : {},
    // turns left and turns in total of a unit which is still produced
    buildLeftVariable : "CIV_BUILD_LEFT",
    buildTurnsVariable : "CIV_BUILD_TURNS",
    // resources each player owns at the start of a match
    startResources : [0, 30, 200],
    variablePrefix : "CIV_",
    initVariable : "CIV_INITIALIZED",

    // ------------------------------------------------------------------------------------------------
    // production of the buildings each turn
    // ------------------------------------------------------------------------------------------------
    foodPerPlains : 1,
    metalPerForest : 1,
    // a mine creates its base yield plus Metal for each mountain and each iron deposit next to it
    mineBaseYield : 1,
    metalPerMountain : 1,
    metalPerIron : 4,
    // an oil refinery creates its base yield plus Oil for each oil deposit next to it
    oilBaseYield : 1,
    oilPerDeposit : 4,
    // Each building stores resources on its own: [Food, Metal, Oil] it can hold.
    // The buildings which create resources fill their own store and stop when it's full.
    // The stock a player can spend is what the logistical centers hold: trucks bring the resources there.
    storage : {"HQ" :               [50,  100, 100],
               "CIV_CITY" :         [100, 50,  50],
               "CIV_WAREHOUSE" :    [50,  150, 150],
               "CIV_FARMHOUSE" :    [30,  0,   0],
               "CIV_LUMBER_MILL" :  [0,   30,  0],
               "CIV_MINE" :         [0,   30,  0],
               "CIV_OIL_REFINERY" : [0,   0,   30],},
    // logistical centers: only resources in these buildings can be spend
    centerIds : ["HQ", "CIV_CITY", "CIV_WAREHOUSE"],
    // A logistical center takes the resources of the mines, farms, mills and refineries of its owner next to it each turn.
    autoCollect : true,
    // A building which produces units uses the resources of the logistical centers of its owner next to it, to manufacture
    // the unit and for its first ammo and fuel. Without a center next to it, it has no resources at all.
    // If this is false, each building uses the resources of all centers of its owner.
    localProduction : true,
    // centers the stock of the player is limited to right now, see localProduction
    scopeCenters : null,
    storeVariablePrefix : "CIV_STORE_",
    // the truck carries resources between the buildings of its owner
    truck : "CIV_TRUCK",
    // buildings which can produce the truck
    truckFactories : ["FACTORY"],
    // resources a truck carries in total
    truckCapacity : 40,
    cargoVariablePrefix : "CIV_CARGO_",
    // id of the menu entry which loads or unloads all resources
    allResources : "ALL",
    // each city and headquarters creates a bit of everything: [Food, Metal, Oil]. Its funds are part of fundsIncome.
    cityProduction : [2, 2, 2],
    // terrains which count for the production of a building next to them.
    // A field with a building or road on it doesn't count as its terrain anymore.
    plainsIds : ["PLAINS", "PLAINS_DESTROYED", "PLAINS_PLASMA"],
    forestIds : ["FOREST", "FOREST1", "FOREST2", "FOREST3", "DESERT_FOREST", "DESERT_FOREST1", "SNOW_FOREST", "SNOW_FOREST1", "SNOW_FOREST2", "WASTE_FOREST"],
    mountainIds : ["MOUNTAIN", "DESERT_ROCK", "SNOW_MOUNTAIN", "WASTE_MOUNTAIN"],
    // Resource deposits are a property of a field and shown as a small icon on its terrain.
    // A deposit counts for a mine or refinery on its field or next to it, as long as no other building stands on it.
    iron : "IRON",
    oilDeposit : "OIL",
    depositVariable : "CIV_DEPOSIT",
    // A map without any deposit gets random ones at the start of the match: [id, land fields for each deposit]
    randomDeposits : [["IRON", 30], ["OIL", 40]],
    // chance in percent that a random iron deposit is put on a mountain
    ironOnMountainChance : 70,
    // terrains for random deposits
    depositTerrains : ["PLAINS", "PLAINS_DESTROYED", "FOREST", "FOREST1", "FOREST2", "FOREST3", "DESERT", "SNOW", "WASTE"],
    depositsDoneVariable : "CIV_DEPOSITS_DONE",

    // ------------------------------------------------------------------------------------------------
    // population
    // ------------------------------------------------------------------------------------------------
    // cities and headquarters have a population. It eats Food from the store of its building each turn and grows when
    // enough Food is left. Each unit a human player produces takes population from the nearest city which has some.
    populationIds : ["HQ", "CIV_CITY"],
    startPopulation : 3,
    maxPopulation : 10,
    foodPerPopulation : 1,
    // Food which has to be left after feeding the population to grow by one: base + perPopulation * population
    growthFoodBase : 4,
    growthFoodPerPopulation : 2,
    unitPopulation : 1,
    populationVariable : "CIV_POPULATION",
    populationInitVariable : "CIV_POPULATION_INIT",

    // ------------------------------------------------------------------------------------------------
    // radius of the buildings
    // ------------------------------------------------------------------------------------------------
    // Each building works the fields around it. A mine, farm, mill or refinery uses the terrain in its radius.
    // A logistical center collects the stores of the mines, farms, mills and refineries in its radius and supplies
    // the buildings which produce units in its radius. The fields in the radius of the buildings of a player are the
    // territory of this player: its border is always shown in the colour of the player.
    radius : {"HQ" : 3, "CIV_CITY" : 3, "CIV_WAREHOUSE" : 3,
              "CIV_FARMHOUSE" : 2, "CIV_LUMBER_MILL" : 2, "CIV_MINE" : 2, "CIV_OIL_REFINERY" : 2,
              "FACTORY" : 2, "AIRPORT" : 2, "HARBOUR" : 2,},

    // ------------------------------------------------------------------------------------------------
    // trucks
    // ------------------------------------------------------------------------------------------------
    // trucks each human player owns at the start, next to the first logistical center
    startTrucks : 2,
    autoVariable : "CIV_AUTO",
    autoDayVariable : "CIV_AUTO_DAY",
    // a truck on its own drives to a center once it's at least this full, in percent
    autoDeliverLoad : 50,

    // ------------------------------------------------------------------------------------------------
    // supply: Metal is the ammunition and Oil the fuel of the units
    // ------------------------------------------------------------------------------------------------
    // The units use their ammo and fuel like always. The stock of the player is only used when a unit
    // gets its ammo and fuel: when it's produced and whenever it gets resupplied.
    metalPerAmmo : 1,
    oilPerFuel : 1,
    // weapons with one of these endings don't need Metal for their ammo: machine guns
    freeAmmoWeaponEndings : ["_MG"],
    // units with these movement types don't need Oil for their fuel: soldiers carry rations
    oilFreeMovementTypes : ["MOVE_FEET", "MOVE_MECH"],
    // The ai doesn't know about the economy. If this is false its units are supplied like without this mode,
    // if it's true the ai uses its stock like a human and runs out of ammo and fuel without refineries and mines.
    aiPaysSupply : false,
    // Metal needed to manufacture a unit. It's paid once when the unit is produced, on top of the Metal for its ammo,
    // and a unit can't be produced without it. Resupplies never need it.
    industrialMetal : {"INFANTRY" : 0,          // soldiers
                       "LIGHT" : 10,            // light vehicles and transports
                       "ARTILLERY" : 15,        // artillery and light armour
                       "TANK" : 20,             // tanks and medium armour
                       "HEAVY" : 30,            // heavy armour
                       "HELICOPTER" : 15,
                       "AIRCRAFT" : 25,
                       "NAVAL_LIGHT" : 20,
                       "NAVAL_LARGE" : 35,},
    // category of all units which aren't part of industrialMetalUnits, by the type of the unit
    industrialMetalDefaults : [[GameEnums.UnitType_Infantry, "INFANTRY"],
                               [GameEnums.UnitType_Ground, "TANK"],
                               [GameEnums.UnitType_Hovercraft, "TANK"],
                               [GameEnums.UnitType_Air, "AIRCRAFT"],
                               [GameEnums.UnitType_Naval, "NAVAL_LARGE"]],
    industrialMetalUnits : {"CIV_TRUCK" : "LIGHT", "RECON" : "LIGHT", "APC" : "LIGHT", "FLARE" : "LIGHT",
                            "ZCOUNIT_INTEL_TRUCK" : "LIGHT", "ZCOUNIT_LOGIC_TRUCK" : "LIGHT", "ZCOUNIT_SMUGGLER" : "LIGHT",
                            "ZCOUNIT_HOT_TANK" : "LIGHT",
                            "ARTILLERY" : "ARTILLERY", "FLAK" : "ARTILLERY", "ANTITANKCANNON" : "ARTILLERY",
                            "MISSILE" : "ARTILLERY", "ROCKETTHROWER" : "ARTILLERY", "ARTILLERYCRAFT" : "ARTILLERY",
                            "HOVERFLAK" : "ARTILLERY", "ZCOUNIT_REPAIR_TANK" : "ARTILLERY", "ZCOUNIT_CHAPERON" : "ARTILLERY",
                            "HEAVY_TANK" : "HEAVY", "NEOTANK" : "HEAVY", "MEGATANK" : "HEAVY", "HEAVY_HOVERCRAFT" : "HEAVY",
                            "PIPERUNNER" : "HEAVY", "ZCOUNIT_SIEGE_CANNON" : "HEAVY", "ZCOUNIT_CRYSTAL_TANK" : "HEAVY",
                            "ZCOUNIT_NEOSPIDER_TANK" : "HEAVY", "ZCOUNIT_ROYAL_GUARD" : "HEAVY", "ZCOUNIT_TANK_HUNTER" : "HEAVY",
                            "ZCOUNIT_IRON_SHIELD_GENERATOR" : "HEAVY",
                            "K_HELI" : "HELICOPTER", "T_HELI" : "HELICOPTER",
                            "GUNBOAT" : "NAVAL_LIGHT", "CANNONBOAT" : "NAVAL_LIGHT", "BLACK_BOAT" : "NAVAL_LIGHT",
                            "TORPEDOBOAT" : "NAVAL_LIGHT", "LANDER" : "NAVAL_LIGHT", "FRIGATE" : "NAVAL_LIGHT",
                            "DESTROYER" : "NAVAL_LIGHT", "WATERMINE" : "LIGHT",},
    // An aircraft on one of these buildings of its owner or an ally is landed: it doesn't use its daily fuel.
    // It still gets resupplied there with the Oil and Metal of its owner like every other unit.
    landingBuildings : ["AIRPORT", "TEMPORARY_AIRPORT"],

    // ------------------------------------------------------------------------------------------------
    // buildings
    // ------------------------------------------------------------------------------------------------
    city : "CIV_CITY",
    farmhouse : "CIV_FARMHOUSE",
    mine : "CIV_MINE",
    lumberMill : "CIV_LUMBER_MILL",
    oilRefinery : "CIV_OIL_REFINERY",
    warehouse : "CIV_WAREHOUSE",
    buildingIds : ["CIV_CITY", "CIV_FARMHOUSE", "CIV_MINE", "CIV_LUMBER_MILL", "CIV_OIL_REFINERY", "CIV_WAREHOUSE", "CIV_SITE"],
    // A building which needs more than one turn of work is a building site until it's finished. The site looks like an
    // empty silo and stores the work and its progress, so the work goes on when the infantry or another one comes back.
    site : "CIV_SITE",
    // an empty silo and a silo with a missile: infantry builds the silo and then a missile on it
    silo : "SILO",
    loadedSilo : "SILO_ROCKET",
    missile : "MISSILE",
    // range of the blast of a missile around its target, 2 in a normal match
    missileRadius : 1,
    normalMissileRadius : 2,
    // The look of a building belongs to its type and not to its owner: each type uses the capital of one army.
    // The owner is only shown by the colour of the building.
    buildingSprites : {"CIV_CITY" : "hq+os",
                       "CIV_FARMHOUSE" : "hq+yc",
                       "CIV_MINE" : "hq+bm",
                       "CIV_LUMBER_MILL" : "hq+ge",
                       "CIV_OIL_REFINERY" : "hq+bh",},
    // funds income of the buildings
    fundsIncome : {"CIV_CITY" : 1000,
                   "CIV_FARMHOUSE" : 0,
                   "CIV_MINE" : 0,
                   "CIV_LUMBER_MILL" : 0,
                   "CIV_OIL_REFINERY" : 0,
                   "CIV_WAREHOUSE" : 1000,},
    // all units move over the new buildings like over a town
    movementAlias : "TOWN",

    // ------------------------------------------------------------------------------------------------
    // work of the infantry
    // ------------------------------------------------------------------------------------------------
    road : "STREET",
    clearForest : "CLEAR_FOREST",
    // [id, name, funds, metal, turns, icon]
    // The costs are paid once when the work begins. Nothing is paid back if the work is given up.
    // A turn of work creates as many work points as the infantry has hp, like a capture, and a work of n turns
    // needs n * workPointsPerTurn points. So a damaged infantry needs longer and a work of one turn is done at once.
    workOptions : [["STREET",           qsTr("Build Road"),         500,  2,  1, "STREET"],
                   ["CIV_FARMHOUSE",    qsTr("Build Farmhouse"),    2000, 10, 2, "CIV_FARMHOUSE"],
                   ["CIV_LUMBER_MILL",  qsTr("Build Lumber Mill"),  2000, 10, 2, "CIV_LUMBER_MILL"],
                   ["CIV_MINE",         qsTr("Build Mine"),         3000, 15, 2, "CIV_MINE"],
                   ["CIV_OIL_REFINERY", qsTr("Build Oil Refinery"), 5000, 20, 3, "CIV_OIL_REFINERY"],
                   ["CIV_WAREHOUSE",    qsTr("Build Warehouse"),    3000, 15, 2, "CIV_WAREHOUSE"],
                   ["FACTORY",          qsTr("Build Base"),         5000, 30, 3, "FACTORY"],
                   ["AIRPORT",          qsTr("Build Airport"),      8000, 40, 4, "AIRPORT"],
                   ["HARBOUR",          qsTr("Build Port"),         8000, 40, 4, "HARBOUR"],
                   ["SILO",             qsTr("Build Silo"),         4000, 25, 3, "SILO"],
                   ["MISSILE",          qsTr("Build Missile"),      6000, 20, 2, "SILO_ROCKET"],
                   ["CLEAR_FOREST",     qsTr("Clear Forest"),       0,    0,  0, "FOREST"]],
    workPointsPerTurn : 10,
    // id of the menu entry which continues the work of the infantry
    continueWork : "CONTINUE_WORK",
    projectVariable : "CIV_PROJECT",
    projectPointsVariable : "CIV_PROJECT_POINTS",
    projectPlayerVariable : "CIV_PROJECT_PLAYER",
    projectDayVariable : "CIV_PROJECT_DAY",
    // id of the menu entry of a silo which finishes its missile
    finishMissile : "FINISH_MISSILE",
    // metal gained by clearing a forest
    clearForestMetal : 5,
    // terrains on which roads and buildings can be build
    buildTerrains : ["PLAINS", "PLAINS_DESTROYED"],
    // a port needs at least one of these terrains next to it, so its ships can leave it
    port : "HARBOUR",
    portWaterIds : ["SEA", "LAKE", "ROUGH_SEA", "REAF", "FOG"],

    // ------------------------------------------------------------------------------------------------
    // functions
    // ------------------------------------------------------------------------------------------------
    isActive : function(map)
    {
        if (map === null || typeof map === "undefined")
        {
            return false;
        }
        // the rule selection of the game lists all rules, the ones which are switched off have the value 0
        var rule = map.getGameRules().getGameRule(CIVWARS.ruleId);
        return (typeof rule !== "undefined" && rule !== null && rule.getRuleValue(0) !== 0);
    },
    // Sets up the match: the stock, trucks and build lists of all players and the deposits of the map.
    // It's called by the first action of a match, the same way on all computers of a network match.
    // Nothing else may set up anything: the info bar and the menus only read, else the computers would differ.
    initGame : function(map)
    {
        if (!CIVWARS.isActive(map))
        {
            return;
        }
        var count = map.getPlayerCount();
        for (var i = 0; i < count; i++)
        {
            CIVWARS.initPlayer(map.getPlayer(i));
        }
    },
    initPlayer : function(player)
    {
        var variables = player.getVariables();
        var init = variables.createVariable(CIVWARS.initVariable);
        if (init.readDataBool() === false)
        {
            init.writeDataBool(true);
            // the resources of the start are put into the logistical centers of the player
            for (var i = 0; i < CIVWARS.resources.length; i++)
            {
                CIVWARS.changeCenterStock(player, CIVWARS.resources[i], CIVWARS.startResources[i]);
            }
            // the truck is part of this mode
            player.changeBuildlist(CIVWARS.truck, false);
            CIVWARS.initMap(player.getMap());
            CIVWARS.spawnStartTrucks(player);
        }
    },
    // ------------------------------------------------------------------------------------------------
    // distances and radius
    // ------------------------------------------------------------------------------------------------
    isHexGrid : function()
    {
        return globals.getCircle(1, 1).size() === 6;
    },
    getDistance : function(x1, y1, x2, y2)
    {
        var dx = x2 - x1;
        var dy = y2 - y1;
        if (CIVWARS.isHexGrid())
        {
            return (Math.abs(dx) + Math.abs(dy) + Math.abs(dx + dy)) / 2;
        }
        return Math.abs(dx) + Math.abs(dy);
    },
    // radius of the building, 0 for buildings without one
    getRadius : function(building)
    {
        var radius = CIVWARS.radius[building.getBuildingID()];
        if (typeof radius === "undefined")
        {
            return 0;
        }
        return radius;
    },
    // Shows the territories of the players: all fields in the radius of their buildings. A field in the radius of the
    // buildings of several players belongs to the nearest one. In fog of war only the own team is shown.
    // Does nothing as long as the buildings with a radius and their owners stay the same.
    refreshTerritory : function(map)
    {
        if (!CIVWARS.isActive(map) || !CIVWARS.isHexGrid())
        {
            return;
        }
        var viewPlayer = map.getCurrentViewPlayer();
        var fog = map.getGameRules().getFogMode() !== GameEnums.Fog_Off;
        var list = [];
        var signature = (viewPlayer !== null ? viewPlayer.getPlayerID() : -1) + ":";
        var playerCount = map.getPlayerCount();
        for (var i = 0; i < playerCount; i++)
        {
            var player = map.getPlayer(i);
            if (player.getIsDefeated() || (fog && viewPlayer !== null && viewPlayer.isEnemy(player)))
            {
                continue;
            }
            var buildings = player.getBuildings();
            var count = buildings.size();
            for (var i2 = 0; i2 < count; i2++)
            {
                var building = buildings.at(i2);
                var radius = CIVWARS.getRadius(building);
                if (radius > 0)
                {
                    list.push([building.getX(), building.getY(), radius, i]);
                    signature += building.getX() + "," + building.getY() + "," + radius + "," + i + ";";
                }
            }
            buildings.remove();
        }
        if (signature === map.getTerritorySignature())
        {
            return;
        }
        map.setTerritorySignature(signature);
        var fields = {};
        for (var i3 = 0; i3 < list.length; i3++)
        {
            var offsets = globals.getCircle(0, list[i3][2]);
            var size = offsets.size();
            for (var i4 = 0; i4 < size; i4++)
            {
                var offset = offsets.at(i4);
                var x = list[i3][0] + offset.x;
                var y = list[i3][1] + offset.y;
                if (map.onMap(x, y))
                {
                    var distance = CIVWARS.getDistance(0, 0, offset.x, offset.y);
                    var key = x + "," + y;
                    if (typeof fields[key] === "undefined" || distance < fields[key][3])
                    {
                        fields[key] = [x, y, list[i3][3], distance];
                    }
                }
            }
        }
        map.clearTerritory();
        for (var field in fields)
        {
            map.setTerritoryField(fields[field][0], fields[field][1], fields[field][2]);
        }
        map.updateTerritoryBorders();
    },
    // logistical centers of the owner of the building which have it in their radius
    getCentersInRange : function(building, map)
    {
        var result = [];
        var owner = building.getOwner();
        if (owner === null)
        {
            return result;
        }
        var scope = CIVWARS.scopeCenters;
        CIVWARS.scopeCenters = null;
        var centers = CIVWARS.getCenters(owner);
        CIVWARS.scopeCenters = scope;
        for (var i = 0; i < centers.length; i++)
        {
            if (centers[i] !== building &&
                CIVWARS.getDistance(centers[i].getX(), centers[i].getY(), building.getX(), building.getY()) <= CIVWARS.getRadius(centers[i]))
            {
                result.push(centers[i]);
            }
        }
        return result;
    },
    // ------------------------------------------------------------------------------------------------
    // deposits
    // ------------------------------------------------------------------------------------------------
    getDeposit : function(terrain)
    {
        return terrain.getVariables().createVariable(CIVWARS.depositVariable).readDataString();
    },
    setDeposit : function(terrain, id)
    {
        terrain.getVariables().createVariable(CIVWARS.depositVariable).writeDataString(id);
        terrain.loadSprites();
    },
    // amount of deposits of the given kind on the given field and next to it. A deposit below another building doesn't count.
    countDeposits : function(map, x, y, id)
    {
        var count = 0;
        var fields = ACTION.getNeighbourFields(Qt.point(x, y));
        fields.push(Qt.point(x, y));
        for (var i = 0; i < fields.length; i++)
        {
            if (map.onMap(fields[i].x, fields[i].y))
            {
                var terrain = map.getTerrain(fields[i].x, fields[i].y);
                if (CIVWARS.getDeposit(terrain) === id &&
                    (terrain.getBuilding() === null || (fields[i].x === x && fields[i].y === y)))
                {
                    count++;
                }
            }
        }
        return count;
    },
    // text of the deposit of a field e.g. " (Iron)", "" without deposit
    getDepositText : function(terrain)
    {
        var deposit = CIVWARS.getDeposit(terrain);
        if (deposit === CIVWARS.iron)
        {
            return " (" + qsTr("Iron") + ")";
        }
        else if (deposit === CIVWARS.oilDeposit)
        {
            return " (" + qsTr("Oil") + ")";
        }
        return "";
    },
    // called once for each match: a map without deposits gets random ones
    initMap : function(map)
    {
        var done = map.getPlayer(0).getVariables().createVariable(CIVWARS.depositsDoneVariable);
        if (done.readDataBool())
        {
            return;
        }
        done.writeDataBool(true);
        var width = map.getMapWidth();
        var heigth = map.getMapHeight();
        var land = [];
        var mountains = [];
        for (var x = 0; x < width; x++)
        {
            for (var y = 0; y < heigth; y++)
            {
                var terrain = map.getTerrain(x, y);
                if (CIVWARS.getDeposit(terrain) !== "")
                {
                    // the map has its own deposits
                    return;
                }
                if (terrain.getBuilding() === null)
                {
                    var id = terrain.getID();
                    if (CIVWARS.mountainIds.indexOf(id) >= 0)
                    {
                        mountains.push(terrain);
                    }
                    else if (CIVWARS.depositTerrains.indexOf(id) >= 0)
                    {
                        land.push(terrain);
                    }
                }
            }
        }
        var fields = land.length + mountains.length;
        for (var i = 0; i < CIVWARS.randomDeposits.length; i++)
        {
            var deposit = CIVWARS.randomDeposits[i][0];
            var count = Math.ceil(fields / CIVWARS.randomDeposits[i][1]);
            for (var i2 = 0; i2 < count; i2++)
            {
                var list = land;
                if (deposit === CIVWARS.iron && mountains.length > 0 && globals.randInt(0, 99) < CIVWARS.ironOnMountainChance)
                {
                    list = mountains;
                }
                if (list.length === 0)
                {
                    continue;
                }
                var index = globals.randInt(0, list.length - 1);
                CIVWARS.setDeposit(list[index], deposit);
                list.splice(index, 1);
            }
        }
    },
    // ------------------------------------------------------------------------------------------------
    // population
    // ------------------------------------------------------------------------------------------------
    hasPopulation : function(building)
    {
        return building !== null && CIVWARS.populationIds.indexOf(building.getBuildingID()) >= 0;
    },
    getPopulation : function(building)
    {
        // only reads: a building which never changed its population has the one of the start
        var variables = building.getVariables();
        if (variables.createVariable(CIVWARS.populationInitVariable).readDataBool() === false)
        {
            return CIVWARS.startPopulation;
        }
        return variables.createVariable(CIVWARS.populationVariable).readDataInt32();
    },
    setPopulation : function(building, population)
    {
        var variables = building.getVariables();
        variables.createVariable(CIVWARS.populationInitVariable).writeDataBool(true);
        variables.createVariable(CIVWARS.populationVariable).writeDataInt32(Math.max(0, population));
    },
    getGrowthFood : function(population)
    {
        return CIVWARS.growthFoodBase + CIVWARS.growthFoodPerPopulation * population;
    },
    // called at the start of the turn of the owner: the population eats and grows if enough Food is left
    feedPopulation : function(building)
    {
        var population = CIVWARS.getPopulation(building);
        var food = CIVWARS.getStored(building, CIVWARS.food);
        var needed = population * CIVWARS.foodPerPopulation;
        if (food < needed)
        {
            // not enough Food: everything is eaten and nobody is born
            CIVWARS.changeStored(building, CIVWARS.food, -food);
            return;
        }
        CIVWARS.changeStored(building, CIVWARS.food, -needed);
        var growth = CIVWARS.getGrowthFood(population);
        if (population < CIVWARS.maxPopulation && food - needed >= growth)
        {
            CIVWARS.changeStored(building, CIVWARS.food, -growth);
            CIVWARS.setPopulation(building, population + 1);
        }
    },
    // city or headquarters of the player with population which is the nearest to the given field, or null
    getPopulationSource : function(player, x, y)
    {
        var best = null;
        var bestDistance = -1;
        if (player === null)
        {
            return best;
        }
        var buildings = player.getBuildings();
        var count = buildings.size();
        for (var i = 0; i < count; i++)
        {
            var building = buildings.at(i);
            if (CIVWARS.hasPopulation(building) && CIVWARS.getPopulation(building) >= CIVWARS.unitPopulation)
            {
                var distance = CIVWARS.getDistance(building.getX(), building.getY(), x, y);
                if (best === null || distance < bestDistance)
                {
                    best = building;
                    bestDistance = distance;
                }
            }
        }
        buildings.remove();
        return best;
    },
    // if the player has the population for a unit produced by the given building
    canDraft : function(player, building)
    {
        return CIVWARS.unitPopulation <= 0 || CIVWARS.getPopulationSource(player, building.getX(), building.getY()) !== null;
    },
    // called by the start of turn of all buildings
    onBuildingStartOfTurn : function(building, map)
    {
        if (!CIVWARS.isActive(map) || building.getOwner() === null || building.getOwner().getIsDefeated())
        {
            return;
        }
        if (CIVWARS.hasPopulation(building))
        {
            CIVWARS.feedPopulation(building);
            if (building.getBuildingID() !== CIVWARS.city)
            {
                // a headquarters creates resources like a city, the city does it with its own script
                CIVWARS.produce(building, map);
            }
        }
    },
    getResource : function(player, resource)
    {
        if (player === null)
        {
            return 0;
        }
        var centers = CIVWARS.getCenters(player);
        var stock = 0;
        for (var i = 0; i < centers.length; i++)
        {
            stock += CIVWARS.getStored(centers[i], resource);
        }
        return stock;
    },
    // ------------------------------------------------------------------------------------------------
    // storage of the buildings
    // ------------------------------------------------------------------------------------------------
    // most of the resource the building can store
    getStoreCapacity : function(building, resource)
    {
        var storage = CIVWARS.storage[building.getBuildingID()];
        if (typeof storage === "undefined")
        {
            return 0;
        }
        return storage[CIVWARS.resources.indexOf(resource)];
    },
    getStored : function(building, resource)
    {
        return building.getVariables().createVariable(CIVWARS.storeVariablePrefix + resource).readDataInt32();
    },
    // Adds resources to the store of the building or takes them from it with a negative amount.
    // Returns the amount which was really added or taken: the store is never above its capacity or below 0.
    changeStored : function(building, resource, amount)
    {
        var stored = CIVWARS.getStored(building, resource);
        var value = stored + amount;
        if (amount > 0)
        {
            var capacity = CIVWARS.getStoreCapacity(building, resource);
            if (value > capacity)
            {
                value = Math.max(stored, capacity);
            }
        }
        if (value < 0)
        {
            value = 0;
        }
        building.getVariables().createVariable(CIVWARS.storeVariablePrefix + resource).writeDataInt32(value);
        return value - stored;
    },
    // if the building stores resources at all
    hasStorage : function(building)
    {
        return building !== null && typeof CIVWARS.storage[building.getBuildingID()] !== "undefined";
    },
    // text of the store of a building e.g. " [M 12/30]", "" for buildings without store
    getStoreText : function(building)
    {
        if (!CIVWARS.hasStorage(building) || !CIVWARS.isActive(building.getMap()))
        {
            return "";
        }
        var text = "";
        for (var i = 0; i < CIVWARS.resources.length; i++)
        {
            var capacity = CIVWARS.getStoreCapacity(building, CIVWARS.resources[i]);
            if (capacity > 0)
            {
                if (text !== "")
                {
                    text += " ";
                }
                text += CIVWARS.resourceShortNames[i] + " " + CIVWARS.getStored(building, CIVWARS.resources[i]) + "/" + capacity;
            }
        }
        if (CIVWARS.hasPopulation(building))
        {
            text = qsTr("Pop") + " " + CIVWARS.getPopulation(building) + " " + text;
        }
        return " [" + text + "]";
    },
    // the logistical centers of the player
    getCenters : function(player)
    {
        var centers = [];
        if (player === null)
        {
            return centers;
        }
        if (CIVWARS.scopeCenters !== null)
        {
            return CIVWARS.scopeCenters;
        }
        var buildings = player.getBuildings();
        var count = buildings.size();
        for (var i = 0; i < count; i++)
        {
            var building = buildings.at(i);
            if (CIVWARS.centerIds.indexOf(building.getBuildingID()) >= 0)
            {
                centers.push(building);
            }
        }
        buildings.remove();
        return centers;
    },
    // buildings of the given player with one of the given ids next to the given field
    getAdjacentBuildings : function(map, x, y, player, ids)
    {
        var result = [];
        var fields = ACTION.getNeighbourFields(Qt.point(x, y));
        for (var i = 0; i < fields.length; i++)
        {
            if (map.onMap(fields[i].x, fields[i].y))
            {
                var building = map.getTerrain(fields[i].x, fields[i].y).getBuilding();
                if (building !== null && building.getOwner() === player && ids.indexOf(building.getBuildingID()) >= 0 &&
                    result.indexOf(building) < 0)
                {
                    result.push(building);
                }
            }
        }
        return result;
    },
    // Limits the stock of the owner of a building which produces units to the logistical centers next to it,
    // until endProductionScope is called. Does nothing without localProduction.
    beginProductionScope : function(building, map)
    {
        CIVWARS.scopeCenters = null;
        if (!CIVWARS.localProduction || building === null || building.getOwner() === null || !CIVWARS.isActive(map))
        {
            return;
        }
        var centers = CIVWARS.getCentersInRange(building, map);
        if (CIVWARS.centerIds.indexOf(building.getBuildingID()) >= 0)
        {
            centers.push(building);
        }
        CIVWARS.scopeCenters = centers;
    },
    endProductionScope : function()
    {
        CIVWARS.scopeCenters = null;
    },
    // Spreads a gain over the logistical centers of the player or takes a loss from them.
    // A gain which doesn't fit into the centers is lost.
    changeCenterStock : function(player, resource, amount)
    {
        var centers = CIVWARS.getCenters(player);
        for (var i = 0; i < centers.length && amount !== 0; i++)
        {
            amount -= CIVWARS.changeStored(centers[i], resource, amount);
        }
    },
    setResource : function(player, resource, value)
    {
        if (player === null)
        {
            return;
        }
        if (value < 0)
        {
            value = 0;
        }
        CIVWARS.addResource(player, resource, value - CIVWARS.getResource(player, resource));
    },
    // most of the resource the logistical centers of the player can store
    getCapacity : function(player, resource)
    {
        var centers = CIVWARS.getCenters(player);
        var capacity = 0;
        for (var i = 0; i < centers.length; i++)
        {
            capacity += CIVWARS.getStoreCapacity(centers[i], resource);
        }
        return capacity;
    },
    // Adds resources to the logistical centers of the player or takes them from there.
    addResource : function(player, resource, amount)
    {
        if (player === null)
        {
            return;
        }
        CIVWARS.changeCenterStock(player, resource, amount);
    },
    // short text of all resources of the player e.g. "F 3  M 30  O 0"
    getResourceText : function(player)
    {
        // the info of the player is updated after each action: the territories are kept up to date with it
        if (player !== null)
        {
            CIVWARS.refreshTerritory(player.getMap());
        }
        var text = "";
        for (var i = 0; i < CIVWARS.resources.length; i++)
        {
            if (i > 0)
            {
                text += " ";
            }
            text += CIVWARS.resourceShortNames[i] + " " + CIVWARS.getResource(player, CIVWARS.resources[i]);
            var capacity = CIVWARS.getCapacity(player, CIVWARS.resources[i]);
            if (capacity >= 0)
            {
                text += "/" + capacity;
            }
        }
        return text;
    },
    // amount of fields next to the given field with one of the given terrains.
    // Uses the neighbour helper of the grid, so it works on the hex grid and on the square grid.
    countAdjacentTerrains : function(map, x, y, terrainIds)
    {
        var count = 0;
        var fields = ACTION.getNeighbourFields(Qt.point(x, y));
        for (var i = 0; i < fields.length; i++)
        {
            if (map.onMap(fields[i].x, fields[i].y))
            {
                var terrain = map.getTerrain(fields[i].x, fields[i].y);
                // getID is the id of the building for fields with a building, so developed fields don't count
                if (terrainIds.indexOf(terrain.getID()) >= 0)
                {
                    count++;
                }
            }
        }
        return count;
    },
    // production of a building of the given type at the given field as [resource, amount] or null
    getProductionAt : function(map, buildingId, x, y)
    {
        if (buildingId === CIVWARS.farmhouse)
        {
            return [CIVWARS.food, CIVWARS.foodPerPlains * CIVWARS.countAdjacentTerrains(map, x, y, CIVWARS.plainsIds)];
        }
        else if (buildingId === CIVWARS.lumberMill)
        {
            return [CIVWARS.metal, CIVWARS.metalPerForest * CIVWARS.countAdjacentTerrains(map, x, y, CIVWARS.forestIds)];
        }
        else if (buildingId === CIVWARS.mine)
        {
            return [CIVWARS.metal, CIVWARS.mineBaseYield +
                                   CIVWARS.metalPerMountain * CIVWARS.countAdjacentTerrains(map, x, y, CIVWARS.mountainIds) +
                                   CIVWARS.metalPerIron * CIVWARS.countDeposits(map, x, y, CIVWARS.iron)];
        }
        else if (buildingId === CIVWARS.oilRefinery)
        {
            return [CIVWARS.oil, CIVWARS.oilBaseYield + CIVWARS.oilPerDeposit * CIVWARS.countDeposits(map, x, y, CIVWARS.oilDeposit)];
        }
        return null;
    },
    getProduction : function(building, map)
    {
        return CIVWARS.getProductionAt(map, building.getBuildingID(), building.getX(), building.getY());
    },
    // all resources a building creates each turn as list of [resource, amount]
    getProductionList : function(building, map)
    {
        var list = [];
        if (CIVWARS.hasPopulation(building))
        {
            for (var i = 0; i < CIVWARS.resources.length; i++)
            {
                if (CIVWARS.cityProduction[i] > 0)
                {
                    list.push([CIVWARS.resources[i], CIVWARS.cityProduction[i]]);
                }
            }
        }
        var production = CIVWARS.getProduction(building, map);
        if (production !== null && production[1] > 0)
        {
            list.push(production);
        }
        return list;
    },
    getResourceName : function(resource)
    {
        return CIVWARS.resourceNames[CIVWARS.resources.indexOf(resource)];
    },
    // text of the production of a building e.g. "+3 Food"
    getProductionText : function(production)
    {
        if (production === null)
        {
            return "";
        }
        return "+" + production[1] + " " + CIVWARS.getResourceName(production[0]);
    },
    // called at the start of the turn of the owner of the building
    produce : function(building, map)
    {
        var owner = building.getOwner();
        if (owner === null || owner.getIsDefeated())
        {
            return;
        }
        var production = CIVWARS.getProductionList(building, map);
        for (var i = 0; i < production.length; i++)
        {
            // the building fills its own store, a truck brings the resources of a mine or farm to a logistical center
            CIVWARS.changeStored(building, production[i][0], production[i][1]);
        }
        CIVWARS.collect(building, map);
    },
    // the logistical centers of the owner next to a mine, farm, mill or refinery take what it has stored, as far as it fits
    collect : function(building, map)
    {
        if (!CIVWARS.autoCollect || CIVWARS.centerIds.indexOf(building.getBuildingID()) >= 0)
        {
            return;
        }
        var centers = CIVWARS.getCentersInRange(building, map);
        for (var i = 0; i < CIVWARS.resources.length; i++)
        {
            var resource = CIVWARS.resources[i];
            for (var i2 = 0; i2 < centers.length; i2++)
            {
                var stored = CIVWARS.getStored(building, resource);
                if (stored <= 0)
                {
                    break;
                }
                var moved = CIVWARS.changeStored(centers[i2], resource, stored);
                CIVWARS.changeStored(building, resource, -moved);
            }
        }
    },
    getWorkOption : function(id)
    {
        for (var i = 0; i < CIVWARS.workOptions.length; i++)
        {
            if (CIVWARS.workOptions[i][0] === id)
            {
                return CIVWARS.workOptions[i];
            }
        }
        return null;
    },
    // if the work can be done on the given terrain at all, without looking at the costs
    isWorkPossible : function(id, terrain, map)
    {
        if (terrain === null)
        {
            return false;
        }
        if (id === CIVWARS.missile)
        {
            // a missile is build on an empty silo
            return terrain.getBuilding() !== null && terrain.getBuilding().getBuildingID() === CIVWARS.silo;
        }
        if (terrain.getBuilding() !== null)
        {
            return false;
        }
        var terrainId = terrain.getID();
        if (id === CIVWARS.clearForest)
        {
            return CIVWARS.forestIds.indexOf(terrainId) >= 0;
        }
        if (CIVWARS.buildTerrains.indexOf(terrainId) < 0)
        {
            return false;
        }
        if (id === CIVWARS.port)
        {
            // a port is build on land at the coast: ships are produced on its field and leave over the water next to it
            return CIVWARS.countAdjacentTerrains(map, terrain.getX(), terrain.getY(), CIVWARS.portWaterIds) > 0;
        }
        return true;
    },
    canAfford : function(player, id)
    {
        var option = CIVWARS.getWorkOption(id);
        return option !== null &&
               player.getFunds() >= option[2] &&
               CIVWARS.getResource(player, CIVWARS.metal) >= option[3];
    },
    getWorkPointsNeeded : function(id)
    {
        var option = CIVWARS.getWorkOption(id);
        if (option === null)
        {
            return 0;
        }
        return option[4] * CIVWARS.workPointsPerTurn;
    },
    // text of a work for the menu e.g. "Build Airport - 8000 / 40 M / 4 turns"
    getWorkText : function(option)
    {
        var text = option[1];
        if (option[0] === CIVWARS.clearForest)
        {
            return text + " +" + CIVWARS.clearForestMetal + " " + CIVWARS.getResourceName(CIVWARS.metal);
        }
        text += " - " + option[2] + " / " + option[3] + " " + CIVWARS.resourceShortNames[1] + " / " + option[4];
        if (option[4] === 1)
        {
            text += qsTr(" turn");
        }
        else
        {
            text += qsTr(" turns");
        }
        return text;
    },
    // building which holds a work in progress on the given field or null: a building site or a silo with a missile in work
    getSite : function(terrain)
    {
        if (terrain === null)
        {
            return null;
        }
        var building = terrain.getBuilding();
        if (building === null)
        {
            return null;
        }
        var id = building.getBuildingID();
        if (id !== CIVWARS.site && id !== CIVWARS.silo)
        {
            return null;
        }
        var project = building.getVariables().createVariable(CIVWARS.projectVariable).readDataString();
        if (project === "" || CIVWARS.getWorkOption(project) === null)
        {
            return null;
        }
        return building;
    },
    getSiteProject : function(site)
    {
        return site.getVariables().createVariable(CIVWARS.projectVariable).readDataString();
    },
    // work points infantry added to the work
    getSiteWorkPoints : function(site)
    {
        return site.getVariables().createVariable(CIVWARS.projectPointsVariable).readDataInt32();
    },
    // Progress of the work. A silo of a player works on its missile on its own: each day since the work began counts like
    // a turn of work of an infantry. The days are counted instead of adding points each turn, so the work can't get stuck.
    getSitePoints : function(site)
    {
        var points = CIVWARS.getSiteWorkPoints(site);
        if (site.getBuildingID() === CIVWARS.silo && site.getOwner() !== null)
        {
            var days = site.getMap().getCurrentDay() - site.getVariables().createVariable(CIVWARS.projectDayVariable).readDataInt32();
            if (days > 0)
            {
                points += days * CIVWARS.workPointsPerTurn;
            }
        }
        var needed = CIVWARS.getWorkPointsNeeded(CIVWARS.getSiteProject(site));
        if (points > needed)
        {
            points = needed;
        }
        return points;
    },
    isSiteFinished : function(site)
    {
        return CIVWARS.getSitePoints(site) >= CIVWARS.getWorkPointsNeeded(CIVWARS.getSiteProject(site));
    },
    // id of the player a work in progress belongs to. A building site belongs to its owner, so capturing it takes over the work.
    getSitePlayer : function(site)
    {
        // The work of a building with an owner belongs to this owner: capturing a building site or a silo takes over its work.
        // Only a silo without owner remembers who works on its missile.
        if (site.getBuildingID() === CIVWARS.site || site.getOwner() !== null)
        {
            return site.getOwnerID();
        }
        return site.getVariables().createVariable(CIVWARS.projectPlayerVariable).readDataInt32();
    },
    // stores a work on its building. The day is only given when the work begins.
    setSiteProject : function(site, id, points, playerId, day)
    {
        var variables = site.getVariables();
        variables.createVariable(CIVWARS.projectVariable).writeDataString(id);
        variables.createVariable(CIVWARS.projectPointsVariable).writeDataInt32(points);
        variables.createVariable(CIVWARS.projectPlayerVariable).writeDataInt32(playerId);
        if (typeof day !== "undefined")
        {
            variables.createVariable(CIVWARS.projectDayVariable).writeDataInt32(day);
        }
        if (site.getBuildingID() === CIVWARS.silo)
        {
            // shows the unfinished missile
            site.updateBuildingSprites();
        }
    },
    // text of a work in progress e.g. "Build Airport 20/40"
    getSiteText : function(site)
    {
        var project = CIVWARS.getSiteProject(site);
        return CIVWARS.getWorkOption(project)[1] + " " + CIVWARS.getSitePoints(site) + "/" + CIVWARS.getWorkPointsNeeded(project);
    },
    // building with a work of the owner of the unit on the field the unit stands on, or null
    getUnitSite : function(unit, map)
    {
        if (unit === null || unit.getTerrain() === null)
        {
            return null;
        }
        var site = CIVWARS.getSite(unit.getTerrain());
        if (site === null || CIVWARS.getSitePlayer(site) !== unit.getOwner().getPlayerID())
        {
            return null;
        }
        return site;
    },
    // id of the work the unit can continue on the field it stands on, "" if there's none
    getProject : function(unit, map)
    {
        var site = CIVWARS.getUnitSite(unit, map);
        if (site === null)
        {
            return "";
        }
        return CIVWARS.getSiteProject(site);
    },
    getProjectPoints : function(unit, map)
    {
        var site = CIVWARS.getUnitSite(unit, map);
        if (site === null)
        {
            return 0;
        }
        return CIVWARS.getSitePoints(site);
    },
    // turns to produce the given unit
    getProductionTurns : function(unitId)
    {
        if (typeof CIVWARS.productionTurnsUnits[unitId] !== "undefined")
        {
            return CIVWARS.productionTurnsUnits[unitId];
        }
        var costs = Global[unitId].getBaseCost();
        for (var i = 0; i < CIVWARS.productionTurns.length; ++i)
        {
            if (costs <= CIVWARS.productionTurns[i][0])
            {
                return CIVWARS.productionTurns[i][1];
            }
        }
        return 1;
    },
    // turns until the unit leaves its factory, 0 for a finished unit
    getBuildTurnsLeft : function(unit)
    {
        if (unit === null)
        {
            return 0;
        }
        return unit.getVariables().createVariable(CIVWARS.buildLeftVariable).readDataInt32();
    },
    // Called by the engine for a unit which is still produced: the finished part of its sprite from its bottom, the part above it is greyed out.
    // The engine keeps such a unit greyed out and unable to move.
    getUnitBuildFraction : function(unit)
    {
        var left = CIVWARS.getBuildTurnsLeft(unit);
        var turns = unit.getVariables().createVariable(CIVWARS.buildTurnsVariable).readDataInt32();
        if (left <= 0 || turns <= 0)
        {
            return 1;
        }
        return (turns - left + 1) / (turns + 1);
    },
    // a new unit which needs more than one turn stays unfinished in its factory
    startProduction : function(unit)
    {
        var turns = CIVWARS.getProductionTurns(unit.getUnitID());
        if (turns > 1)
        {
            unit.getVariables().createVariable(CIVWARS.buildTurnsVariable).writeDataInt32(turns);
            unit.getVariables().createVariable(CIVWARS.buildLeftVariable).writeDataInt32(turns);
            unit.updateSprites(false);
            // the unit has as much of its hp as is build of it, so an unfinished unit fights and defends weaker
            unit.setHp(10 * CIVWARS.getUnitBuildFraction(unit));
        }
    },
    // if the unit is still produced
    isInProduction : function(unit)
    {
        return CIVWARS.getBuildTurnsLeft(unit) > 0;
    },
    // called by the engine at the start of the turn of the owner of a unit which is still produced
    onUnitStartOfTurn : function(unit, map)
    {
        var before = CIVWARS.getUnitBuildFraction(unit);
        var left = CIVWARS.getBuildTurnsLeft(unit) - 1;
        if (left < 0)
        {
            left = 0;
        }
        unit.getVariables().createVariable(CIVWARS.buildLeftVariable).writeDataInt32(left);
        unit.updateSprites(false);
        // the hp grow with the unit, damage it took while it was produced stays
        var hp = unit.getHp() + 10 * (CIVWARS.getUnitBuildFraction(unit) - before);
        // the hp are stored rounded, a finished unit without damage has all of them
        if (hp > 9.9)
        {
            hp = 10;
        }
        unit.setHp(hp);
        if (left === 0)
        {
            unit.setHasMoved(false);
        }
    },
    // ------------------------------------------------------------------------------------------------
    // trucks
    // ------------------------------------------------------------------------------------------------
    isTruck : function(unit)
    {
        return unit !== null && unit.getUnitID() === CIVWARS.truck;
    },
    getCargo : function(unit, resource)
    {
        return unit.getVariables().createVariable(CIVWARS.cargoVariablePrefix + resource).readDataInt32();
    },
    getCargoTotal : function(unit)
    {
        var total = 0;
        for (var i = 0; i < CIVWARS.resources.length; i++)
        {
            total += CIVWARS.getCargo(unit, CIVWARS.resources[i]);
        }
        return total;
    },
    // the trucks of the start of a human player, on free fields around the first logistical center
    spawnStartTrucks : function(player)
    {
        if (CIVWARS.startTrucks <= 0 || !CIVWARS.isHuman(player))
        {
            return;
        }
        var map = player.getMap();
        var centers = CIVWARS.getCenters(player);
        if (centers.length === 0)
        {
            return;
        }
        var left = CIVWARS.startTrucks;
        var fields = globals.getCircle(1, 2);
        var size = fields.size();
        for (var i = 0; i < size && left > 0; i++)
        {
            var x = centers[0].getX() + fields.at(i).x;
            var y = centers[0].getY() + fields.at(i).y;
            if (map.onMap(x, y))
            {
                var terrain = map.getTerrain(x, y);
                if (terrain.getUnit() === null && terrain.getBuilding() === null &&
                    (CIVWARS.buildTerrains.indexOf(terrain.getID()) >= 0 || terrain.getID() === CIVWARS.road))
                {
                    map.spawnUnit(x, y, CIVWARS.truck, player, 0, true);
                    left--;
                }
            }
        }
    },
    isAutoTruck : function(unit)
    {
        return CIVWARS.isTruck(unit) && unit.getVariables().createVariable(CIVWARS.autoVariable).readDataBool();
    },
    // switches the automatic delivery of a truck on or off. It drives when its owner ends the turn.
    setAutoTruck : function(unit, auto, map)
    {
        unit.getVariables().createVariable(CIVWARS.autoVariable).writeDataBool(auto);
        unit.getVariables().createVariable(CIVWARS.autoDayVariable).writeDataInt32(auto ? map.getCurrentDay() : 0);
    },
    // free space of the building for the cargo of the truck
    getUnloadTotal : function(unit, building)
    {
        var total = 0;
        for (var i = 0; i < CIVWARS.resources.length; i++)
        {
            total += CIVWARS.getUnloadAmount(unit, building, CIVWARS.resources[i]);
        }
        return total;
    },
    getStoredTotal : function(building)
    {
        var total = 0;
        for (var i = 0; i < CIVWARS.resources.length; i++)
        {
            total += CIVWARS.getStored(building, CIVWARS.resources[i]);
        }
        return total;
    },
    // Building an automatic truck drives to: a logistical center if it carries enough, else a mine, farm, mill or
    // refinery with resources. taken are the buildings other trucks already drive to in this turn.
    getAutoTarget : function(unit, map, taken)
    {
        var player = unit.getOwner();
        var buildings = player.getBuildings();
        var count = buildings.size();
        var cargo = CIVWARS.getCargoTotal(unit);
        var best = null;
        var bestValue = 0;
        var bestCenter = null;
        var bestCenterValue = 0;
        for (var i = 0; i < count; i++)
        {
            var building = buildings.at(i);
            if (!CIVWARS.hasStorage(building))
            {
                continue;
            }
            var other = building.getTerrain().getUnit();
            if (other !== null && other !== unit)
            {
                // the field is taken
                continue;
            }
            var distance = CIVWARS.getDistance(building.getX(), building.getY(), unit.getX(), unit.getY());
            if (CIVWARS.centerIds.indexOf(building.getBuildingID()) >= 0)
            {
                var space = CIVWARS.getUnloadTotal(unit, building);
                if (space > 0)
                {
                    var centerValue = space / (distance + 1);
                    if (bestCenter === null || centerValue > bestCenterValue)
                    {
                        bestCenter = building;
                        bestCenterValue = centerValue;
                    }
                }
            }
            else if (taken.indexOf(building) < 0)
            {
                var stored = Math.min(CIVWARS.getStoredTotal(building), CIVWARS.truckCapacity - cargo);
                if (stored > 0)
                {
                    var value = stored / (distance + 1);
                    if (best === null || value > bestValue)
                    {
                        best = building;
                        bestValue = value;
                    }
                }
            }
        }
        buildings.remove();
        if (bestCenter !== null && (best === null || cargo * 100 >= CIVWARS.truckCapacity * CIVWARS.autoDeliverLoad))
        {
            return bestCenter;
        }
        return best;
    },
    // one turn of a truck which delivers on its own
    runAutoTruck : function(unit, map, taken)
    {
        var target = CIVWARS.getAutoTarget(unit, map, taken);
        if (target === null)
        {
            return;
        }
        var isCenter = CIVWARS.centerIds.indexOf(target.getBuildingID()) >= 0;
        if (!isCenter)
        {
            taken.push(target);
        }
        if (unit.moveTowards(target.getX(), target.getY()))
        {
            // arrived: resources are brought to a center and taken from every other building
            CIVWARS.transferCargo(unit, target, CIVWARS.allResources, !isCenter);
        }
        unit.setHasMoved(true);
    },
    // Called when a player ends the turn, before the turn ends: the automatic trucks which weren't moved by hand drive.
    runAutoTrucks : function(player, map)
    {
        if (!CIVWARS.isActive(map) || player === null)
        {
            return;
        }
        var units = player.getUnits();
        var count = units.size();
        var taken = [];
        for (var i = 0; i < count; i++)
        {
            var unit = units.at(i);
            if (CIVWARS.isAutoTruck(unit) && unit.getTerrain() !== null && CIVWARS.getBuildTurnsLeft(unit) <= 0 &&
                (!unit.getHasMoved() ||
                 unit.getVariables().createVariable(CIVWARS.autoDayVariable).readDataInt32() === map.getCurrentDay()))
            {
                CIVWARS.runAutoTruck(unit, map, taken);
            }
        }
        units.remove();
    },
    // text of the cargo of a truck e.g. "Cargo 30/40: M 20 O 10"
    getCargoText : function(unit)
    {
        var text = qsTr("Cargo") + " " + CIVWARS.getCargoTotal(unit) + "/" + CIVWARS.truckCapacity;
        var first = true;
        for (var i = 0; i < CIVWARS.resources.length; i++)
        {
            var cargo = CIVWARS.getCargo(unit, CIVWARS.resources[i]);
            if (cargo > 0)
            {
                text += (first ? ": " : " ") + CIVWARS.resourceShortNames[i] + " " + cargo;
                first = false;
            }
        }
        if (CIVWARS.isAutoTruck(unit))
        {
            text += " " + qsTr("(auto)");
        }
        return text;
    },
    // building of the owner of the unit with a store on the given field, or null
    getTruckBuilding : function(unit, x, y, map)
    {
        if (!map.onMap(x, y))
        {
            return null;
        }
        var building = map.getTerrain(x, y).getBuilding();
        if (building === null || building.getOwner() !== unit.getOwner() || !CIVWARS.hasStorage(building))
        {
            return null;
        }
        return building;
    },
    // amount of the resource the truck would load from the building: what's stored and fits
    getLoadAmount : function(unit, building, resource, free)
    {
        if (typeof free === "undefined")
        {
            free = CIVWARS.truckCapacity - CIVWARS.getCargoTotal(unit);
        }
        return Math.max(0, Math.min(CIVWARS.getStored(building, resource), free));
    },
    // amount of the resource the truck would unload into the building: its cargo which fits into the store
    getUnloadAmount : function(unit, building, resource)
    {
        var space = CIVWARS.getStoreCapacity(building, resource) - CIVWARS.getStored(building, resource);
        return Math.max(0, Math.min(CIVWARS.getCargo(unit, resource), space));
    },
    // Moves resources between a truck and the building it stands on. resource is one resource or all of them.
    // Returns the amount which was moved.
    transferCargo : function(unit, building, resource, load)
    {
        var moved = 0;
        for (var i = 0; i < CIVWARS.resources.length; i++)
        {
            var current = CIVWARS.resources[i];
            if (resource !== CIVWARS.allResources && resource !== current)
            {
                continue;
            }
            var amount = load ? CIVWARS.getLoadAmount(unit, building, current) : CIVWARS.getUnloadAmount(unit, building, current);
            if (amount > 0)
            {
                var cargo = unit.getVariables().createVariable(CIVWARS.cargoVariablePrefix + current);
                if (load)
                {
                    amount = -CIVWARS.changeStored(building, current, -amount);
                    cargo.writeDataInt32(cargo.readDataInt32() + amount);
                }
                else
                {
                    amount = CIVWARS.changeStored(building, current, amount);
                    cargo.writeDataInt32(cargo.readDataInt32() - amount);
                }
                moved += amount;
            }
        }
        return moved;
    },
    // units a building produces in this mode on top of its normal ones
    getConstructionList : function(list, building)
    {
        if (building !== null && CIVWARS.isActive(building.getMap()) && CIVWARS.isHuman(building.getOwner()) &&
            CIVWARS.truckFactories.indexOf(building.getBuildingID()) >= 0 && list.indexOf(CIVWARS.truck) < 0)
        {
            return list.concat([CIVWARS.truck]);
        }
        return list;
    },
    // progress of the bar of the unit info: of the production of the unit or of the work it can continue
    getProjectProgress : function(unit, map)
    {
        if (CIVWARS.getBuildTurnsLeft(unit) > 0)
        {
            return CIVWARS.getUnitBuildFraction(unit);
        }
        if (CIVWARS.isTruck(unit))
        {
            return CIVWARS.getCargoTotal(unit) / CIVWARS.truckCapacity;
        }
        return CIVWARS.getProjectPoints(unit, map) / CIVWARS.getWorkPointsNeeded(CIVWARS.getProject(unit, map));
    },
    // text of the progress of the work the unit can continue e.g. "Build Airport 20/40", "" without work
    getProjectText : function(unit, map)
    {
        var left = CIVWARS.getBuildTurnsLeft(unit);
        if (left > 0)
        {
            return qsTr("In production: %0 turn(s) left").replace("%0", left);
        }
        if (CIVWARS.isTruck(unit))
        {
            return CIVWARS.getCargoText(unit);
        }
        var site = CIVWARS.getUnitSite(unit, map);
        if (site === null)
        {
            return "";
        }
        return CIVWARS.getSiteText(site);
    },
    // Work points of one turn of work of the unit. Uses the capture rate of the unit like the apc does for its airport:
    // as many points as the unit has hp plus the capture boni of the co's.
    getWorkRate : function(unit)
    {
        var before = unit.getCapturePoints();
        unit.increaseCapturePoints(Qt.point(unit.getX(), unit.getY()));
        var rate = unit.getCapturePoints() - before;
        unit.setCapturePoints(before);
        if (rate < 1)
        {
            rate = 1;
        }
        return rate;
    },
    // creates the result of a finished work on the field of the unit
    finishWork : function(id, unit, map)
    {
        var x = unit.getX();
        var y = unit.getY();
        var terrain = map.getTerrain(x, y);
        if (id === CIVWARS.road)
        {
            // the unit stays on the field
            map.replaceTerrainOnly(CIVWARS.road, x, y, true, false);
            map.updateSprites(x, y);
            // the border of the territory is part of the fields: it's drawn again for the new field
            map.setTerritorySignature("");
        }
        else if (id === CIVWARS.missile)
        {
            // the empty silo gets its missile and belongs to the builder
            terrain.loadBuilding(CIVWARS.loadedSilo);
            terrain.getBuilding().setUnitOwner(unit);
        }
        else
        {
            // replaces the building site
            terrain.loadBuilding(id);
            terrain.getBuilding().setUnitOwner(unit);
        }
    },
    getMissileRadius : function(map)
    {
        if (CIVWARS.isActive(map))
        {
            return CIVWARS.missileRadius;
        }
        return CIVWARS.normalMissileRadius;
    },
    // vision of silos and building sites: like the other buildings in this mode, none in a normal match
    getSiloVision : function(building, map)
    {
        if (typeof map !== "undefined" && map !== null && CIVWARS.isActive(map))
        {
            return map.getGameRules().getBuildingVision();
        }
        return -1;
    },
    // name of a silo with the missile it works on e.g. "Silo: Build Missile 10/20"
    getSiloName : function(building)
    {
        if (typeof building !== "undefined" && building !== null && typeof building.getTerrain === "function" &&
            CIVWARS.getSite(building.getTerrain()) !== null)
        {
            return qsTr("Silo: ") + CIVWARS.getSiteText(building);
        }
        return qsTr("Silo");
    },
    // Launches the missile of a silo at the given field. The silo is empty afterwards and keeps its owner.
    // The launch and the damage are the ones of the missile action of the infantry.
    launchSilo : function(silo, targetX, targetY, map)
    {
        if (silo === null || silo.getBuildingID() !== CIVWARS.loadedSilo)
        {
            return false;
        }
        var x = silo.getX();
        var y = silo.getY();
        var owner = silo.getOwner();
        var terrain = map.getTerrain(x, y);
        terrain.loadBuilding(CIVWARS.silo);
        terrain.getBuilding().setOwner(owner);
        ACTION_MISSILE.postAnimationTargetX = targetX;
        ACTION_MISSILE.postAnimationTargetY = targetY;
        var animation = GameAnimationFactory.createAnimation(map, x - 1, y - 7, 30);
        animation.addSprite("silo+launch", map.getImageSize() / 4, -map.getImageSize() / 2, 0, 2, 0);
        animation.setSound("missile_launch.wav");
        animation.setEndOfAnimationCall("ACTION_MISSILE", "performPostAnimation");
        return true;
    },
    // if the silo of the player builds a missile on its own right now
    isSiloBuilding : function(silo)
    {
        return silo !== null && silo.getBuildingID() === CIVWARS.silo && CIVWARS.getSite(silo.getTerrain()) !== null;
    },
    // A silo of a player builds a missile on its own: the costs are paid with the order and the silo works on it
    // at the start of each turn of its owner, with the work of an undamaged infantry.
    orderSiloMissile : function(silo, map)
    {
        var player = silo.getOwner();
        var option = CIVWARS.getWorkOption(CIVWARS.missile);
        if (player === null || silo.getBuildingID() !== CIVWARS.silo || CIVWARS.isSiloBuilding(silo) || !CIVWARS.canAfford(player, CIVWARS.missile))
        {
            return false;
        }
        player.addFunds(-option[2]);
        CIVWARS.addResource(player, CIVWARS.metal, -option[3]);
        CIVWARS.setSiteProject(silo, CIVWARS.missile, 0, player.getPlayerID(), map.getCurrentDay());
        return true;
    },
    // if the missile a silo of a player works on is done
    isSiloMissileReady : function(silo)
    {
        return CIVWARS.isSiloBuilding(silo) && silo.getOwner() !== null && CIVWARS.isSiteFinished(silo);
    },
    // turns a silo with a finished missile into a silo which can launch it
    finishSiloMissile : function(silo, map)
    {
        if (!CIVWARS.isSiloMissileReady(silo))
        {
            return false;
        }
        var owner = silo.getOwner();
        var terrain = silo.getTerrain();
        terrain.loadBuilding(CIVWARS.loadedSilo);
        terrain.getBuilding().setOwner(owner);
        return true;
    },
    // called at the start of the turn of the owner of an empty silo
    advanceSiloMissile : function(silo, map)
    {
        CIVWARS.finishSiloMissile(silo, map);
    },
    // sprites of an empty silo: a silo which works on a missile shows its lower half
    loadSiloSprites : function(building, neutral)
    {
        var mask = "silo+mask";
        if (typeof building.getTerrain === "function" && building.getTerrain() !== null && CIVWARS.isSiloBuilding(building))
        {
            // the lower half of the missile stands in the silo: its top is still missing
            mask = "silo+rocket+half+mask";
        }
        building.loadSpriteV2("silo", GameEnums.Recoloring_Matrix, 400, Qt.point(0,0), TERRAIN.getBuildingTerrainPalette(building));
        building.loadSpriteV2(mask, GameEnums.Recoloring_Matrix, 400, Qt.point(0,0), "", neutral);
    },
    // One turn of work of the unit on the field it stands on: begins the given work or continues the one on its field.
    // Returns "" if nothing was done, else "begun", "continued" or "finished".
    doWork : function(id, unit, map)
    {
        var x = unit.getX();
        var y = unit.getY();
        var player = unit.getOwner();
        var terrain = map.getTerrain(x, y);
        var begun = false;
        var site = null;
        if (id === CIVWARS.continueWork)
        {
            site = CIVWARS.getUnitSite(unit, map);
            if (site === null)
            {
                return "";
            }
            id = CIVWARS.getSiteProject(site);
        }
        else
        {
            var option = CIVWARS.getWorkOption(id);
            if (option === null || !CIVWARS.isWorkPossible(id, terrain, map) || !CIVWARS.canAfford(player, id))
            {
                return "";
            }
            // the whole costs are paid when the work begins
            player.addFunds(-option[2]);
            CIVWARS.addResource(player, CIVWARS.metal, -option[3]);
            if (id === CIVWARS.clearForest)
            {
                // the unit stays on the field
                var deposit = CIVWARS.getDeposit(terrain);
                map.replaceTerrainOnly("PLAINS", x, y, false, false);
                if (deposit !== "")
                {
                    map.getTerrain(x, y).getVariables().createVariable(CIVWARS.depositVariable).writeDataString(deposit);
                }
                map.updateSprites(x, y);
                map.setTerritorySignature("");
                CIVWARS.addResource(player, CIVWARS.metal, CIVWARS.clearForestMetal);
                return "finished";
            }
            if (CIVWARS.getWorkPointsNeeded(id) > CIVWARS.getWorkRate(unit))
            {
                // the work needs more than this turn: it's stored on a building site or on the silo of the missile
                if (id !== CIVWARS.missile)
                {
                    terrain.loadBuilding(CIVWARS.site);
                    terrain.getBuilding().setUnitOwner(unit);
                }
                site = terrain.getBuilding();
                CIVWARS.setSiteProject(site, id, 0, player.getPlayerID(), map.getCurrentDay());
            }
            begun = true;
        }
        var rate = CIVWARS.getWorkRate(unit);
        if (site === null)
        {
            // done in this turn
            CIVWARS.finishWork(id, unit, map);
            return "finished";
        }
        // the work of the infantry is added to what the infantry did before. The progress also counts the days a silo worked itself
        CIVWARS.setSiteProject(site, id, CIVWARS.getSiteWorkPoints(site) + rate, player.getPlayerID());
        if (CIVWARS.isSiteFinished(site))
        {
            CIVWARS.finishWork(id, unit, map);
            return "finished";
        }
        return begun ? "begun" : "continued";
    },
    // ------------------------------------------------------------------------------------------------
    // supply functions
    // ------------------------------------------------------------------------------------------------
    isHuman : function(player)
    {
        if (player === null)
        {
            return false;
        }
        // In a network match the host tells all computers which seats are played by humans: on the other computers
        // the seat of a human and of an ai look the same.
        var seat = player.getVariables().createVariable(CIVWARS.seatTypeVariable).readDataInt32();
        if (seat !== 0)
        {
            return seat === 1;
        }
        var input = player.getBaseGameInput();
        return input !== null && input.getAiType() === GameEnums.AiTypes_Human;
    },
    // Metal needed to manufacture a unit of the given type
    getIndustrialMetal : function(unitId)
    {
        var category = CIVWARS.industrialMetalUnits[unitId];
        if (category === undefined)
        {
            var unitType = Global[unitId].getUnitType();
            for (var i = 0; i < CIVWARS.industrialMetalDefaults.length; i++)
            {
                if (CIVWARS.industrialMetalDefaults[i][0] === unitType)
                {
                    category = CIVWARS.industrialMetalDefaults[i][1];
                    break;
                }
            }
        }
        var metal = CIVWARS.industrialMetal[category];
        if (metal === undefined)
        {
            metal = 0;
        }
        return metal;
    },
    // if the player owns the Metal which is needed to manufacture the unit. Its supply doesn't matter for this.
    canManufacture : function(player, unitId)
    {
        if (!CIVWARS.paysSupply(player))
        {
            return true;
        }
        return CIVWARS.getResource(player, CIVWARS.metal) >= CIVWARS.getIndustrialMetal(unitId);
    },
    // if the supply of the units of this player is taken from his stock
    paysSupply : function(player)
    {
        return player !== null && (CIVWARS.aiPaysSupply || CIVWARS.isHuman(player));
    },
    isFreeAmmoWeapon : function(weaponId)
    {
        if (weaponId === "")
        {
            return true;
        }
        for (var i = 0; i < CIVWARS.freeAmmoWeaponEndings.length; i++)
        {
            if (weaponId.endsWith(CIVWARS.freeAmmoWeaponEndings[i]))
            {
                return true;
            }
        }
        return false;
    },
    isOilFreeMovement : function(movementType)
    {
        return CIVWARS.oilFreeMovementTypes.indexOf(movementType) >= 0;
    },
    // highest amount which can be paid with the given stock
    getAffordable : function(wanted, stock, costPerPoint)
    {
        if (wanted <= 0)
        {
            return 0;
        }
        if (costPerPoint <= 0)
        {
            return wanted;
        }
        var affordable = Math.floor(stock / costPerPoint);
        if (affordable > wanted)
        {
            affordable = wanted;
        }
        return affordable;
    },
    getCosts : function(amount, costPerPoint)
    {
        return Math.ceil(amount * costPerPoint);
    },
    // Takes the Metal and Oil for the wanted ammo and fuel from the stock of the player and
    // returns [fuel, ammo1, ammo2]: what the stock was enough for. The stock never drops below 0.
    takeSupply : function(player, weapon1Id, weapon2Id, movementType, fuel, ammo1, ammo2)
    {
        var result = [fuel > 0 ? fuel : 0, ammo1 > 0 ? ammo1 : 0, ammo2 > 0 ? ammo2 : 0];
        if (!CIVWARS.paysSupply(player))
        {
            return result;
        }
        if (!CIVWARS.isOilFreeMovement(movementType))
        {
            var oil = CIVWARS.getResource(player, CIVWARS.oil);
            result[0] = CIVWARS.getAffordable(result[0], oil, CIVWARS.oilPerFuel);
            CIVWARS.setResource(player, CIVWARS.oil, oil - CIVWARS.getCosts(result[0], CIVWARS.oilPerFuel));
        }
        var metal = CIVWARS.getResource(player, CIVWARS.metal);
        if (!CIVWARS.isFreeAmmoWeapon(weapon1Id))
        {
            result[1] = CIVWARS.getAffordable(result[1], metal, CIVWARS.metalPerAmmo);
            metal -= CIVWARS.getCosts(result[1], CIVWARS.metalPerAmmo);
        }
        if (!CIVWARS.isFreeAmmoWeapon(weapon2Id))
        {
            result[2] = CIVWARS.getAffordable(result[2], metal, CIVWARS.metalPerAmmo);
            metal -= CIVWARS.getCosts(result[2], CIVWARS.metalPerAmmo);
        }
        CIVWARS.setResource(player, CIVWARS.metal, metal);
        return result;
    },
    // Called by Unit::refill for each resupply of a unit, whatever resupplies it: buildings, apc's or co powers.
    // Gets the ammo and fuel the unit would get and returns [fuel, ammo1, ammo2] it really gets,
    // or null if this mode isn't active.
    onRefill : function(unit, fuelGain, ammo1Gain, ammo2Gain, map)
    {
        if (!CIVWARS.isActive(map) || unit === null || unit.getOwner() === null)
        {
            return null;
        }
        return CIVWARS.takeSupply(unit.getOwner(), unit.getWeapon1ID(), unit.getWeapon2ID(), unit.getMovementType(),
                                  fuelGain, ammo1Gain, ammo2Gain);
    },
    // ammo and fuel a unit of the given type enters the match with as {fuel, ammo1, ammo2, weapon1, weapon2, movementType}
    unitSupplyCache : {},
    getUnitSupply : function(unitId)
    {
        if (CIVWARS.unitSupplyCache[unitId] === undefined)
        {
            // reads the values from the init function of the unit with an object which only stores them
            var data = {fuel : 0, ammo1 : 0, ammo2 : 0, weapon1 : "", weapon2 : "", movementType : ""};
            var recorder = new Proxy({}, {
                get : function(target, name)
                {
                    if (name === "setFuel") { return function(value) { data.fuel = value; }; }
                    if (name === "setAmmo1") { return function(value) { data.ammo1 = value; }; }
                    if (name === "setAmmo2") { return function(value) { data.ammo2 = value; }; }
                    if (name === "setWeapon1ID") { return function(value) { data.weapon1 = value; }; }
                    if (name === "setWeapon2ID") { return function(value) { data.weapon2 = value; }; }
                    return function() { return 0; };
                }
            });
            Global[unitId].init(recorder);
            data.movementType = Global[unitId].getMovementType();
            CIVWARS.unitSupplyCache[unitId] = data;
        }
        return CIVWARS.unitSupplyCache[unitId];
    },
    // [metal, oil] a new unit of the given type needs to enter the match fully supplied
    getUnitSupplyCosts : function(unitId)
    {
        var supply = CIVWARS.getUnitSupply(unitId);
        var metal = 0;
        var oil = 0;
        if (supply.ammo1 > 0 && !CIVWARS.isFreeAmmoWeapon(supply.weapon1))
        {
            metal += CIVWARS.getCosts(supply.ammo1, CIVWARS.metalPerAmmo);
        }
        if (supply.ammo2 > 0 && !CIVWARS.isFreeAmmoWeapon(supply.weapon2))
        {
            metal += CIVWARS.getCosts(supply.ammo2, CIVWARS.metalPerAmmo);
        }
        if (supply.fuel > 0 && !CIVWARS.isOilFreeMovement(supply.movementType))
        {
            oil += CIVWARS.getCosts(supply.fuel, CIVWARS.oilPerFuel);
        }
        return [metal, oil];
    },
    // if the stock of the player is enough to produce the unit fully supplied
    canSupplyNewUnit : function(player, unitId)
    {
        if (!CIVWARS.paysSupply(player))
        {
            return true;
        }
        var costs = CIVWARS.getUnitSupplyCosts(unitId);
        return CIVWARS.getResource(player, CIVWARS.metal) >= costs[0] + CIVWARS.getIndustrialMetal(unitId) &&
               CIVWARS.getResource(player, CIVWARS.oil) >= costs[1];
    },
    // [fuel, ammo1, ammo2] a new unit of the given type would enter the match with right now.
    // Doesn't change the stock of the player.
    getExpectedUnitSupply : function(player, unitId)
    {
        var supply = CIVWARS.getUnitSupply(unitId);
        var result = [supply.fuel > 0 ? supply.fuel : 0, supply.ammo1 > 0 ? supply.ammo1 : 0, supply.ammo2 > 0 ? supply.ammo2 : 0];
        if (!CIVWARS.paysSupply(player))
        {
            return result;
        }
        if (!CIVWARS.isOilFreeMovement(supply.movementType))
        {
            result[0] = CIVWARS.getAffordable(result[0], CIVWARS.getResource(player, CIVWARS.oil), CIVWARS.oilPerFuel);
        }
        var metal = CIVWARS.getResource(player, CIVWARS.metal) - CIVWARS.getIndustrialMetal(unitId);
        if (metal < 0)
        {
            metal = 0;
        }
        if (!CIVWARS.isFreeAmmoWeapon(supply.weapon1))
        {
            result[1] = CIVWARS.getAffordable(result[1], metal, CIVWARS.metalPerAmmo);
            metal -= CIVWARS.getCosts(result[1], CIVWARS.metalPerAmmo);
        }
        if (!CIVWARS.isFreeAmmoWeapon(supply.weapon2))
        {
            result[2] = CIVWARS.getAffordable(result[2], metal, CIVWARS.metalPerAmmo);
        }
        return result;
    },
    // Text of the Metal and Oil of a new unit for the build menu.
    // " / 20 M" is the Metal needed to manufacture the unit, without it the unit can't be produced.
    // " + supply 6 M 70 O" is the Metal for its ammo and the Oil for its fuel. If the stock of the player isn't enough for it
    // the unit is produced with less ammo and fuel and the text shows with how much e.g. "(ammo 4/6 fuel 20/70)".
    getUnitSupplyText : function(unitId, player)
    {
        var costs = CIVWARS.getUnitSupplyCosts(unitId);
        var industrial = CIVWARS.getIndustrialMetal(unitId);
        var text = "";
        var turns = CIVWARS.getProductionTurns(unitId);
        if (turns > 1)
        {
            text += " / " + qsTr("%0 turns").replace("%0", turns);
        }
        if (industrial > 0)
        {
            text += " / " + industrial + " " + CIVWARS.resourceShortNames[1];
        }
        if (costs[0] > 0 || costs[1] > 0)
        {
            text += qsTr(" + supply");
            if (costs[0] > 0)
            {
                text += " " + costs[0] + " " + CIVWARS.resourceShortNames[1];
            }
            if (costs[1] > 0)
            {
                text += " " + costs[1] + " " + CIVWARS.resourceShortNames[2];
            }
        }
        if (typeof player !== "undefined" && player !== null &&
            CIVWARS.canManufacture(player, unitId) && !CIVWARS.canSupplyNewUnit(player, unitId))
        {
            var supply = CIVWARS.getUnitSupply(unitId);
            var expected = CIVWARS.getExpectedUnitSupply(player, unitId);
            var maxAmmo = 0;
            var ammo = 0;
            if (supply.ammo1 > 0 && !CIVWARS.isFreeAmmoWeapon(supply.weapon1))
            {
                maxAmmo += supply.ammo1;
                ammo += expected[1];
            }
            if (supply.ammo2 > 0 && !CIVWARS.isFreeAmmoWeapon(supply.weapon2))
            {
                maxAmmo += supply.ammo2;
                ammo += expected[2];
            }
            text += " (";
            if (maxAmmo > 0)
            {
                text += qsTr("ammo ") + ammo + "/" + maxAmmo;
            }
            if (costs[1] > 0)
            {
                if (maxAmmo > 0)
                {
                    text += " ";
                }
                text += qsTr("fuel ") + expected[0] + "/" + supply.fuel;
            }
            text += ")";
        }
        return text;
    },
    // if the unit is an aircraft on an airport of its owner or of an ally
    isLanded : function(unit, map)
    {
        if (unit === null || unit.getUnitType() !== GameEnums.UnitType_Air)
        {
            return false;
        }
        var terrain = unit.getTerrain();
        if (terrain === null)
        {
            return false;
        }
        var building = terrain.getBuilding();
        if (building === null || CIVWARS.landingBuildings.indexOf(building.getBuildingID()) < 0)
        {
            return false;
        }
        var owner = building.getOwner();
        return owner !== null && unit.getOwner().checkAlliance(owner) === GameEnums.Alliance_Friend;
    },
    // Called by Unit::getFuelCostModifier, which all units use for the fuel they need each day.
    // Returns the change of these costs: a landed aircraft doesn't need any fuel.
    getFuelCostModifier : function(unit, costs, map)
    {
        if (CIVWARS.isActive(map) && CIVWARS.isLanded(unit, map))
        {
            // the scripts of the units don't let the costs drop below 0
            return -100000;
        }
        return 0;
    },
    // Called when a unit was produced: its ammo and fuel are taken from the stock of the player.
    // If the stock isn't enough the unit enters the match with what was left.
    onUnitProduced : function(unit, map, building)
    {
        if (!CIVWARS.isActive(map) || unit === null)
        {
            return;
        }
        if (typeof building !== "undefined")
        {
            // the unit is made of the resources of the logistical centers next to its factory
            CIVWARS.beginProductionScope(building, map);
            try
            {
                CIVWARS.onUnitProduced(unit, map);
            }
            finally
            {
                CIVWARS.endProductionScope();
            }
            return;
        }
        CIVWARS.startProduction(unit);
        if (!CIVWARS.paysSupply(unit.getOwner()))
        {
            return;
        }
        // the crew of the unit comes from the nearest city
        var city = CIVWARS.getPopulationSource(unit.getOwner(), unit.getX(), unit.getY());
        if (city !== null)
        {
            CIVWARS.setPopulation(city, CIVWARS.getPopulation(city) - CIVWARS.unitPopulation);
        }
        // the Metal to manufacture the unit comes first, what's left is used for its ammo.
        // The stock never drops below 0: the build menu doesn't offer units without enough Metal.
        CIVWARS.addResource(unit.getOwner(), CIVWARS.metal, -CIVWARS.getIndustrialMetal(unit.getUnitID()));
        var supply = CIVWARS.takeSupply(unit.getOwner(), unit.getWeapon1ID(), unit.getWeapon2ID(), unit.getMovementType(),
                                        unit.getFuel(), unit.getAmmo1(), unit.getAmmo2());
        if (unit.getMaxFuel() > 0)
        {
            unit.setFuel(supply[0]);
        }
        if (unit.getMaxAmmo1() > 0)
        {
            unit.setAmmo1(supply[1]);
        }
        if (unit.getMaxAmmo2() > 0)
        {
            unit.setAmmo2(supply[2]);
        }
    },
    // loads the sprites of a building which looks the same for all owners and only uses the colour of its owner
    loadBuildingSprites : function(building, neutral)
    {
        var sprite = CIVWARS.buildingSprites[building.getBuildingID()];
        building.loadSpriteV2(sprite + "+mask", GameEnums.Recoloring_Matrix, 400, Qt.point(0, 0), "", neutral);
        building.loadSpriteV2(sprite, GameEnums.Recoloring_Matrix, 400, Qt.point(0, 0), TERRAIN.getBuildingTerrainPalette(building));
    },
};
