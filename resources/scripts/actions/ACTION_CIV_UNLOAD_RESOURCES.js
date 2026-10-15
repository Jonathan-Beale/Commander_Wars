var Constructor = function()
{
    // Civilization Wars prototype: a truck unloads resources into the building of its owner it moves to.
    // All values are part of resources/scripts/general/civwars.js
    this.load = false;
    this.getAmount = function(unit, building, resource)
    {
        if (ACTION_CIV_UNLOAD_RESOURCES.load)
        {
            return CIVWARS.getLoadAmount(unit, building, resource);
        }
        return CIVWARS.getUnloadAmount(unit, building, resource);
    };
    this.canBePerformed = function(action, map)
    {
        var unit = action.getTargetUnit();
        var actionTargetField = action.getActionTarget();
        var targetField = action.getTarget();
        if (!CIVWARS.isActive(map) || !CIVWARS.isTruck(unit))
        {
            return false;
        }
        if (ACTION.isEmptyFieldAndHasNotMoved(action, unit, actionTargetField, targetField, map))
        {
            var building = CIVWARS.getTruckBuilding(unit, actionTargetField.x, actionTargetField.y, map);
            if (building !== null)
            {
                for (var i = 0; i < CIVWARS.resources.length; i++)
                {
                    if (ACTION_CIV_UNLOAD_RESOURCES.getAmount(unit, building, CIVWARS.resources[i]) > 0)
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    };
    this.getActionText = function(map)
    {
        return qsTr("Unload Resources");
    };
    this.getIcon = function(map)
    {
        return "load";
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
        var unit = action.getTargetUnit();
        var actionTargetField = action.getActionTarget();
        var building = CIVWARS.getTruckBuilding(unit, actionTargetField.x, actionTargetField.y, map);
        var entries = 0;
        for (var i = 0; i < CIVWARS.resources.length; i++)
        {
            var amount = ACTION_CIV_UNLOAD_RESOURCES.getAmount(unit, building, CIVWARS.resources[i]);
            if (amount > 0)
            {
                entries++;
            }
        }
        if (entries > 1)
        {
            data.addData(qsTr("Everything"), CIVWARS.allResources, "load", 0, true);
        }
        for (var i = 0; i < CIVWARS.resources.length; i++)
        {
            var amount = ACTION_CIV_UNLOAD_RESOURCES.getAmount(unit, building, CIVWARS.resources[i]);
            if (amount > 0)
            {
                data.addData(amount + " " + CIVWARS.resourceNames[i], CIVWARS.resources[i], "load", 0, true);
            }
        }
    };
    this.perform = function(action, map)
    {
        action.startReading();
        var resource = action.readDataString();
        // we need to move the unit to the target position
        var unit = action.getTargetUnit();
        var animation = Global[unit.getUnitID()].doWalkingAnimation(action, map);
        animation.setEndOfAnimationCall("ACTION_CIV_UNLOAD_RESOURCES", "performPostAnimation");
        // move unit to target position
        Global[unit.getUnitID()].moveUnit(unit, action, map);
        ACTION_CIV_UNLOAD_RESOURCES.postAnimationUnit = unit;
        ACTION_CIV_UNLOAD_RESOURCES.postAnimationResource = resource;
    };
    this.postAnimationUnit = null;
    this.postAnimationResource = "";
    this.performPostAnimation = function(postAnimation, map)
    {
        var unit = ACTION_CIV_UNLOAD_RESOURCES.postAnimationUnit;
        if (unit !== null)
        {
            var building = CIVWARS.getTruckBuilding(unit, unit.getX(), unit.getY(), map);
            if (building !== null)
            {
                CIVWARS.transferCargo(unit, building, ACTION_CIV_UNLOAD_RESOURCES.postAnimationResource, ACTION_CIV_UNLOAD_RESOURCES.load);
            }
            // disable unit commandments for this turn
            unit.setHasMoved(true);
        }
        ACTION_CIV_UNLOAD_RESOURCES.postAnimationUnit = null;
        ACTION_CIV_UNLOAD_RESOURCES.postAnimationResource = "";
    };
    this.getName = function()
    {
        return qsTr("Unload Resources");
    };
    this.getDescription = function()
    {
        return qsTr("Civilization Wars: a truck unloads its resources into the building of its owner it moves to, as far as the building can store them. Only resources in a city, warehouse or headquarters can be spend.");
    };
}

Constructor.prototype = ACTION;
var ACTION_CIV_UNLOAD_RESOURCES = new Constructor();
