var Constructor = function()
{
    // Civilization Wars prototype: infantry builds roads and buildings or clears a forest on the field it moves to.
    // The costs and all other values are part of resources/scripts/general/civwars.js
    this.canBePerformed = function(action, map)
    {
        var unit = action.getTargetUnit();
        var actionTargetField = action.getActionTarget();
        var targetField = action.getTarget();
        if (!CIVWARS.isActive(map))
        {
            return false;
        }
        if (ACTION.isEmptyFieldAndHasNotMoved(action, unit, actionTargetField, targetField, map))
        {
            if (ACTION_CIV_WORK.isContinuing(action, map))
            {
                return true;
            }
            var terrain = map.getTerrain(actionTargetField.x, actionTargetField.y);
            for (var i = 0; i < CIVWARS.workOptions.length; i++)
            {
                if (CIVWARS.isWorkPossible(CIVWARS.workOptions[i][0], terrain, map))
                {
                    return true;
                }
            }
        }
        return false;
    };
    // building with a work in progress of the owner of the unit on the field the unit moves to, or null.
    // Any infantry of the owner can continue the work.
    this.getTargetSite = function(action, map)
    {
        var unit = action.getTargetUnit();
        var actionTargetField = action.getActionTarget();
        var site = CIVWARS.getSite(map.getTerrain(actionTargetField.x, actionTargetField.y));
        if (unit === null || site === null || CIVWARS.getSitePlayer(site) !== unit.getOwner().getPlayerID())
        {
            return null;
        }
        return site;
    };
    this.isContinuing = function(action, map)
    {
        return ACTION_CIV_WORK.getTargetSite(action, map) !== null;
    };
    this.getActionText = function(map)
    {
        return qsTr("Work");
    };
    this.getIcon = function(map)
    {
        return "build";
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
        var actionTargetField = action.getActionTarget();
        var terrain = map.getTerrain(actionTargetField.x, actionTargetField.y);
        var player = unit.getOwner();
        if (ACTION_CIV_WORK.isContinuing(action, map))
        {
            // the work was paid when it began
            var site = ACTION_CIV_WORK.getTargetSite(action, map);
            var project = CIVWARS.getWorkOption(CIVWARS.getSiteProject(site));
            data.addData(qsTr("Continue ") + CIVWARS.getSiteText(site), CIVWARS.continueWork, project[5], 0, true);
            return;
        }
        for (var i = 0; i < CIVWARS.workOptions.length; i++)
        {
            var option = CIVWARS.workOptions[i];
            if (CIVWARS.isWorkPossible(option[0], terrain, map))
            {
                var text = CIVWARS.getWorkText(option);
                // shows what the building would create on this field
                var production = CIVWARS.getProductionAt(map, option[0], actionTargetField.x, actionTargetField.y);
                if (production !== null)
                {
                    text += " (" + CIVWARS.getProductionText(production) + ")";
                }
                data.addData(text, option[0], option[5], 0, CIVWARS.canAfford(player, option[0]));
            }
        }
    };
    this.perform = function(action, map)
    {
        action.startReading();
        var work = action.readDataString();
        // we need to move the unit to the target position
        var unit = action.getTargetUnit();
        var animation = Global[unit.getUnitID()].doWalkingAnimation(action, map);
        animation.setEndOfAnimationCall("ACTION_CIV_WORK", "performPostAnimation");
        // move unit to target position
        Global[unit.getUnitID()].moveUnit(unit, action, map);
        ACTION_CIV_WORK.postAnimationUnit = unit;
        ACTION_CIV_WORK.postAnimationWork = work;
    };
    this.postAnimationUnit = null;
    this.postAnimationWork = "";
    this.performPostAnimation = function(postAnimation, map)
    {
        var unit = ACTION_CIV_WORK.postAnimationUnit;
        if (unit !== null)
        {
            var work = ACTION_CIV_WORK.postAnimationWork;
            var id = work;
            var pointsBefore = 0;
            if (work === CIVWARS.continueWork)
            {
                id = CIVWARS.getProject(unit, map);
                pointsBefore = CIVWARS.getProjectPoints(unit, map);
            }
            var x = unit.getX();
            var y = unit.getY();
            var result = CIVWARS.doWork(work, unit, map);
            var needed = CIVWARS.getWorkPointsNeeded(id);
            if (result !== "" && needed > CIVWARS.workPointsPerTurn)
            {
                // the progress is shown like the one of a capture, the way the apc shows the work on its airport
                var pointsAfter = CIVWARS.getProjectPoints(unit, map);
                if (result === "finished")
                {
                    pointsAfter = needed;
                }
                var viewPlayer = map.getCurrentViewPlayer();
                if (viewPlayer === unit.getOwner() || viewPlayer.getFieldVisible(x, y))
                {
                    var captureAnimation = GameAnimationFactory.createGameAnimationCapture(map, x * map.getImageSize() - 10, y * map.getImageSize() - 30,
                                                                                         pointsBefore, pointsAfter, needed);
                    captureAnimation.addBackgroundSprite("capture_background");
                    var armyName = Global.getArmyNameFromPlayerTable(unit.getOwner(), ACTION_CAPTURE.armyData);
                    BUILDING.addCaptureAnimationBuilding(captureAnimation, null, null, unit.getOwner());
                    captureAnimation.addSoldierSprite("soldier+" + armyName + "+mask" , unit.getOwner(), GameEnums.Recoloring_Matrix);
                    captureAnimation.addSoldierSprite("soldier+" + armyName , unit.getOwner(), GameEnums.Recoloring_None);
                }
            }
            // disable unit commandments for this turn
            unit.setHasMoved(true);
        }
        ACTION_CIV_WORK.postAnimationUnit = null;
        ACTION_CIV_WORK.postAnimationWork = "";
    };
    this.getName = function()
    {
        return qsTr("Work");
    };
    this.getDescription = function()
    {
        return qsTr("Civilization Wars: orders an infantry to build a road or a building or to clear a forest on its field. Roads and buildings cost funds and Metal when the work begins and need several turns: the unfinished building is a building site which keeps its progress, so any of your infantry can continue the work. It can also build an empty silo and a missile on an empty silo.");
    };
}

Constructor.prototype = ACTION;
var ACTION_CIV_WORK = new Constructor();
