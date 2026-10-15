var Constructor = function()
{
    // Civilization Wars prototype: an empty silo of the player builds a missile without infantry.
    // The costs and the time are the ones of the missile in resources/scripts/general/civwars.js
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
        // The action is also offered without enough funds or Metal: its menu shows the costs and can't be confirmed then.
        // While the silo works on a missile the menu shows the progress.
        return owner !== null && owner === map.getCurrentPlayer() && CIVWARS.isHuman(owner) &&
               building.getBuildingID() === CIVWARS.silo;
    };
    this.getActionText = function(map)
    {
        return qsTr("Build Missile");
    };
    this.getIcon = function(map)
    {
        return "missile_icon";
    };
    this.isFinalStep = function(action, map)
    {
        if (action.getInputStep() === 0)
        {
            return false;
        }
        else
        {
            return true;
        }
    };
    this.getStepInputType = function(action, map)
    {
        if (action.getInputStep() === 0)
        {
            return "MENU";
        }
        return "";
    };
    this.getStepData = function(action, data, map)
    {
        var building = action.getTargetBuilding();
        var option = CIVWARS.getWorkOption(CIVWARS.missile);
        if (CIVWARS.isSiloMissileReady(building))
        {
            // the missile is done: normally the silo gets it at the start of the turn, this entry does it by hand
            data.addData(qsTr("Finish Missile"), CIVWARS.finishMissile, option[5], 0, true);
        }
        else if (CIVWARS.isSiloBuilding(building))
        {
            // already ordered: only shows the progress
            data.addData(CIVWARS.getSiteText(building), CIVWARS.missile, option[5], 0, false);
        }
        else
        {
            data.addData(CIVWARS.getWorkText(option), CIVWARS.missile, option[5], 0, CIVWARS.canAfford(building.getOwner(), CIVWARS.missile));
        }
    };
    this.perform = function(action, map)
    {
        action.startReading();
        var entry = action.readDataString();
        if (entry === CIVWARS.finishMissile)
        {
            CIVWARS.finishSiloMissile(action.getTargetBuilding(), map);
        }
        else
        {
            CIVWARS.orderSiloMissile(action.getTargetBuilding(), map);
        }
    };
    this.getName = function()
    {
        return qsTr("Build Missile");
    };
    this.getDescription = function()
    {
        return qsTr("Civilization Wars: orders an empty silo to build a missile. The costs are paid at once and the silo needs some turns to finish it.");
    };
}

Constructor.prototype = ACTION;
var ACTION_CIV_BUILD_MISSILE = new Constructor();
