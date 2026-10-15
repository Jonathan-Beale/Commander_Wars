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
        return qsTr("Oil Refinery");
    };
    this.getDescription = function()
    {
        return qsTr("Creates 1 Oil each turn and 4 more for each oil deposit next to it.");
    };
    this.getBaseIncome = function(building, map)
    {
        return CIVWARS.fundsIncome["CIV_OIL_REFINERY"];
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
        return [];
    };
    this.getVision = function(building, map)
    {
        return map.getGameRules().getBuildingVision();
    };
}

Constructor.prototype = BUILDING;
var CIV_OIL_REFINERY = new Constructor();
