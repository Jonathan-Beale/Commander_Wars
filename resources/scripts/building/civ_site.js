var Constructor = function()
{
    // Civilization Wars prototype: unfinished building, see resources/scripts/general/civwars.js
    this.getBuildingGroup = function()
    {
        return 6;
    };
    this.loadSprites = function(building, neutral, map)
    {
        // looks like an empty silo
        building.loadSpriteV2("silo", GameEnums.Recoloring_Matrix, 400, Qt.point(0,0), TERRAIN.getBuildingTerrainPalette(building));
        building.loadSpriteV2("silo+mask", GameEnums.Recoloring_Matrix, 400, Qt.point(0,0), "", neutral);
    };
    this.getName = function(building)
    {
        // shows the work and its progress if the name of a single site is asked
        if (typeof building !== "undefined" && building !== null && typeof building.getTerrain === "function" && CIVWARS.getSite(building.getTerrain()) !== null)
        {
            return qsTr("Site: ") + CIVWARS.getSiteText(building);
        }
        return qsTr("Building site");
    };
    this.getDescription = function()
    {
        return qsTr("An unfinished building. Infantry of its owner continues the work with the work action. It can be captured, which takes over the work.");
    };
    this.getBaseIncome = function()
    {
        return 0;
    };
    this.getDefense = function()
    {
        return 1;
    };
    this.getMiniMapIcon = function()
    {
        return "minimap_silo";
    };
    this.getVision = function(building, map)
    {
        return CIVWARS.getSiloVision(building, map);
    };
}

Constructor.prototype = BUILDING;
var CIV_SITE = new Constructor();
