var Constructor = function()
{
    this.getBuildingGroup = function()
    {
        return 6;
    };
    
    this.loadSprites = function(building, neutral, map)
    {
        // civilization wars: a silo which works on a missile shows it greyed out
        CIVWARS.loadSiloSprites(building, neutral);
    };
    this.getBaseIncome = function()
    {
        return 0;
    };
    this.getDefense = function()
    {
        return 2;
    };
    this.getMiniMapIcon = function()
    {
        return "minimap_silo";
    };

    this.getName = function(building)
    {
        return CIVWARS.getSiloName(building);
    };

    this.getDescription = function()
    {
        return qsTr("An empty missile silo which provides some defence.");
    };

    this.getTerrainAnimationBackground = function(unit, terrain, defender, map)
    {
        return "back_missile+1";
    };
    this.getVision = function(building, map)
    {
        // civilization wars: silos belong to players and give vision like their other buildings
        return CIVWARS.getSiloVision(building, map);
    };
    // civilization wars: a silo of a player builds its missile on its own, see resources/scripts/general/civwars.js
    this.actionList = ["ACTION_CIV_BUILD_MISSILE"];
    this.startOfTurn = function(building, map)
    {
        if (CIVWARS.isActive(map))
        {
            CIVWARS.advanceSiloMissile(building, map);
        }
        // the building may be a silo with a missile now
        var current = building.getTerrain().getBuilding();
        if (current !== null)
        {
            BUILDING.startOfTurn(current, map);
        }
    };
}

Constructor.prototype = BUILDING;
var SILO = new Constructor();
