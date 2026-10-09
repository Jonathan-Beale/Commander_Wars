var Constructor = function()
{
    // a field without a tile. It isn't drawn and no unit can enter it, cause no movement table knows it.
    // Used by the hex grid prototype for maps which aren't a parallelogram e.g. hexagon shaped maps.
    this.getTerrainGroup = function()
    {
        return 0;
    };
    this.getDefaultPalette = function()
    {
        return "";
    };
    this.init = function (terrain)
    {
        terrain.setTerrainName(VOID.getName());
    };
    this.getName = function()
    {
        return qsTr("Edge of the map");
    };
    this.getDefense = function()
    {
        return 0;
    };
    this.loadBaseSprite = function(terrain, map)
    {
    };
    this.loadOverlaySprite = function(terrain, map)
    {
    };
    this.getMiniMapIcon = function()
    {
        return "";
    };
    this.getDescription = function()
    {
        return qsTr("There is no field here.");
    };
    this.getTerrainSprites = function()
    {
        return [];
    };
};
Constructor.prototype = TERRAIN;
var VOID = new Constructor();
