var Constructor = function()
{
    // Civilization Wars prototype: a silo with a missile of the player launches it without infantry.
    this.canBePerformed = function(action, map)
    {
        var unit = action.getTargetUnit();
        var building = action.getTargetBuilding();
        if (!CIVWARS.isActive(map) || unit !== null || building === null)
        {
            return false;
        }
        var owner = building.getOwner();
        // the ai launches its silos with its infantry
        return owner !== null && owner === map.getCurrentPlayer() && CIVWARS.isHuman(owner) &&
               building.getBuildingID() === CIVWARS.loadedSilo;
    };
    this.getActionText = function(map)
    {
        return qsTr("Launch Missile");
    };
    this.getIcon = function(map)
    {
        return "missile_icon";
    };
    this.getStepCursor = function(action, cursorData, map)
    {
        ACTION_MISSILE.getStepCursor(action, cursorData, map);
    };
    this.getStepData = function(action, data, map)
    {
        data.setAllFields(true);
        data.setShowZData(false);
    };
    this.getStepInputType = function(action, map)
    {
        return "FIELD";
    };
    this.isFinalStep = function(action, map)
    {
        if (action.getInputStep() === 1)
        {
            return true;
        }
        else
        {
            return false;
        }
    };
    this.perform = function(action, map)
    {
        action.startReading();
        var targetX = action.readDataInt32();
        var targetY = action.readDataInt32();
        CIVWARS.launchSilo(action.getTargetBuilding(), targetX, targetY, map);
    };
    this.getName = function()
    {
        return qsTr("Launch Missile");
    };
    this.getDescription = function()
    {
        return qsTr("Civilization Wars: launches the missile of a silo at the target area. The missile deals massive damage to all units in the area of effect.");
    };
}

Constructor.prototype = ACTION;
var ACTION_CIV_LAUNCH_MISSILE = new Constructor();
