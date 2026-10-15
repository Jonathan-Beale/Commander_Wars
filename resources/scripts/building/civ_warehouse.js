var Constructor = function()
{
    // Civilization Wars prototype, see resources/scripts/general/civwars.js
    this.getBuildingGroup = function()
    {
        return 1;
    };
    this.loadSprites = function(building, neutral, map)
    {
        // uses the look of the town
        building.loadSpriteV2("town+mask", GameEnums.Recoloring_Matrix, 400, Qt.point(0,0), "", neutral);
        if (building.getOwnerID() >= 0 && !neutral)
        {
            building.loadSpriteV2("town", GameEnums.Recoloring_Matrix, 400, Qt.point(0,0), TERRAIN.getBuildingTerrainPalette(building));
        }
        else
        {
            building.loadSpriteV2("town+neutral", GameEnums.Recoloring_Matrix, 400, Qt.point(0,0), TERRAIN.getBuildingTerrainPalette(building));
        }
    };
    this.getName = function()
    {
        return qsTr("Warehouse");
    };
    this.getDescription = function()
    {
        return qsTr("A logistical center: stores a lot of Metal and Oil which can be spend from here, and creates funds. Ground units can repair and resupply here.");
    };
    this.getBaseIncome = function(building, map)
    {
        return CIVWARS.fundsIncome["CIV_WAREHOUSE"];
    };
    this.startOfTurn = function(building, map)
    {
        BUILDING.startOfTurn(building, map);
        if (CIVWARS.isActive(map))
        {
            CIVWARS.produce(building, map);
        }
    };
    this.getVisionHide = function(building)
    {
        return true;
    };
    this.getRepairTypes = function(building)
    {
        return [GameEnums.UnitType_Ground, GameEnums.UnitType_Infantry];
    };
    this.getVision = function(building, map)
    {
        return map.getGameRules().getBuildingVision();
    };
}

Constructor.prototype = BUILDING;
var CIV_WAREHOUSE = new Constructor();
