var Constructor = function()
{
    // Civilization Wars prototype: switches the automatic delivery of a truck on,
    // see resources/scripts/general/civwars.js
    this.canBePerformed = function(action, map)
    {
        var unit = action.getTargetUnit();
        var actionTargetField = action.getActionTarget();
        var targetField = action.getTarget();
        if (!CIVWARS.isActive(map) || !CIVWARS.isTruck(unit) || unit.getHasMoved())
        {
            return false;
        }
        // the truck gets the order where it stands
        return actionTargetField.x === targetField.x && actionTargetField.y === targetField.y &&
               CIVWARS.isAutoTruck(unit) === false;
    };
    this.getActionText = function(map)
    {
        return qsTr("Auto Deliver");
    };
    this.getIcon = function(map)
    {
        return "load";
    };
    this.isFinalStep = function(action, map)
    {
        return true;
    };
    this.perform = function(action, map)
    {
        var unit = action.getTargetUnit();
        CIVWARS.setAutoTruck(unit, true, map);
        // the truck drives when its owner ends the turn
        unit.setHasMoved(true);
    };
    this.getName = function()
    {
        return qsTr("Auto Deliver");
    };
    this.getDescription = function()
    {
        return qsTr("Civilization Wars: the truck delivers on its own from now on. When you end your turn it drives to a mine, farm, mill or refinery with resources, loads them and brings them to a city, warehouse or headquarters with room for them. A truck you move by hand skips its delivery in this turn.");
    };
}

Constructor.prototype = ACTION;
var ACTION_CIV_AUTO_DELIVER = new Constructor();
