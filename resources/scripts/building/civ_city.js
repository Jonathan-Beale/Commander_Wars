var Constructor = function()
{
    // Civilization Wars prototype, see resources/scripts/general/civwars.js
    this.getBuildingGroup = function()
    {
        return 1;
    };
    this.loadSprites = function(building, neutral, map)
    {
        // the look belongs to the type of the building, the owner only changes the colour
        CIVWARS.loadBuildingSprites(building, neutral);
    };
    this.getName = function()
    {
        return qsTr("City");
    };
    this.getDescription = function()
    {
        return qsTr("Populated city which creates funds and a bit of Food, Metal and Oil. Ground units can repair and resupply here.");
    };
    this.getBaseIncome = function(building, map)
    {
        return CIVWARS.fundsIncome["CIV_CITY"];
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
var CIV_CITY = new Constructor();
