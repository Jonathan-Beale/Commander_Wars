var Constructor = function()
{
    // Civilization Wars prototype: carries resources between the buildings of its owner,
    // see resources/scripts/general/civwars.js
    this.getUnitDamageID = function(unit)
    {
        return "APC";
    };
    this.init = function(unit)
    {
        unit.setAmmo1(0);
        unit.setMaxAmmo1(0);
        unit.setWeapon1ID("");

        unit.setAmmo2(0);
        unit.setMaxAmmo2(0);
        unit.setWeapon2ID("");

        unit.setFuel(60);
        unit.setMaxFuel(60);
        unit.setBaseMovementPoints(5);
        unit.setMinRange(1);
        unit.setMaxRange(1);
        unit.setVision(1);
    };
    this.loadSprites = function(unit)
    {
        unit.loadSpriteV2("logi_truck+mask", GameEnums.Recoloring_Matrix);
    };
    this.getMovementType = function()
    {
        return "MOVE_TIRE_A";
    };
    this.actionList = ["ACTION_CIV_LOAD_RESOURCES", "ACTION_CIV_UNLOAD_RESOURCES", "ACTION_CIV_AUTO_DELIVER", "ACTION_CIV_STOP_AUTO_DELIVER", "ACTION_LOAD", "ACTION_WAIT", "ACTION_CO_UNIT_0", "ACTION_CO_UNIT_1"];
    this.getBaseCost = function()
    {
        return 4000;
    };
    this.getName = function()
    {
        return qsTr("Truck");
    };
    this.doWalkingAnimation = function(action, map)
    {
        var unit = action.getTargetUnit();
        var animation = GameAnimationFactory.createWalkingAnimation(map, unit, action);
        animation.loadSpriteV2("logi_truck+walk+mask", GameEnums.Recoloring_Matrix, 2);
        animation.setSound("movetire.wav", -2);
        return animation;
    };
    this.getDescription = function()
    {
        return qsTr("A defenceless truck which carries Food, Metal and Oil. It loads the resources of a mine, farm or refinery and unloads them at a city, warehouse or headquarters, where they can be spend.");
    };
    this.getUnitType = function()
    {
        return GameEnums.UnitType_Ground;
    };
    this.getEditorPlacementSound = function()
    {
        return "movetire.wav";
    };
}

Constructor.prototype = UNIT;
var CIV_TRUCK = new Constructor();
