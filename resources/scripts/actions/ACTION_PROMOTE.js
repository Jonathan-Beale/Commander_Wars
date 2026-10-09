var Constructor = function()
{
    // Spends a promotion of the unit on one branch of the promotion tree, see UNITRANKINGSYSTEM.
    // The unit can still move and act afterwards.
    this.canBePerformed = function(action, map)
    {
        var unit = action.getTargetUnit();
        var actionTargetField = action.getActionTarget();
        var targetField = action.getTarget();
        if (unit === null || unit.getHasMoved() === true)
        {
            return false;
        }
        if ((actionTargetField.x === targetField.x) && (actionTargetField.y === targetField.y))
        {
            return UNITRANKINGSYSTEM.getPendingPicks(unit) > 0;
        }
        return false;
    };
    this.getActionText = function(map)
    {
        return qsTr("Promote");
    };
    this.getIcon = function(map)
    {
        return "power";
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
        // supported types are MENU and FIELD
        if (action.getInputStep() === 0)
        {
            return "MENU";
        }
        return "";
    };
    this.getStepData = function(action, data, map)
    {
        var unit = action.getTargetUnit();
        for (var i = 0; i < UNITRANKINGSYSTEM.branches.length; i++)
        {
            data.addData(UNITRANKINGSYSTEM.getBranchDescription(unit, i), UNITRANKINGSYSTEM.branches[i],
                         UNITRANKINGSYSTEM.branchIcons[i], 0, true);
        }
    };
    this.perform = function(action, map)
    {
        action.startReading();
        var branch = action.readDataString();
        var unit = action.getTargetUnit();
        UNITRANKINGSYSTEM.pickBranch(unit, branch);
    };
    this.getName = function()
    {
        return qsTr("Promote");
    };
    this.getDescription = function()
    {
        return qsTr("Spends a promotion the unit earned by dealing damage on offense, defense or utility. The unit can still move and act afterwards.");
    };
}

Constructor.prototype = ACTION;
var ACTION_PROMOTE = new Constructor();
