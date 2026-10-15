var Constructor = function()
{
    // Civilization Wars prototype: switches the automatic delivery of a truck off,
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
               CIVWARS.isAutoTruck(unit) === true;
    };
    this.getActionText = function(map)
    {
        return qsTr("Stop Auto Deliver");
    };
    this.getIcon = function(map)
    {
        return "wait";
    };
    this.isFinalStep = function(action, map)
    {
        return true;
    };
    this.perform = function(action, map)
    {
        var unit = action.getTargetUnit();
        CIVWARS.setAutoTruck(unit, false, map);
        // the truck can be used by hand in this turn
    };
    this.getName = function()
    {
        return qsTr("Stop Auto Deliver");
    };
    this.getDescription = function()
    {
        return qsTr("Civilization Wars: the truck stops delivering on its own.");
    };
}

Constructor.prototype = ACTION;
var ACTION_CIV_STOP_AUTO_DELIVER = new Constructor();
