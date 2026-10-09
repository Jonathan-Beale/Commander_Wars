// this is the base class for terrain
var UNITRANKINGSYSTEM =
{
	co1 : -1, // don't change this value
	co2 : -2, // don't change this value
	// moddable part starts here:
    getMaxRang : function(unit)
    {
        return 3;
    },
    icons : ["", "lieutenant", "general", "veteran"],
    names : [qsTr("Soldier"), qsTr("Experienced"), qsTr("Veteran"), qsTr("Elite")],
    offensiveBonus : [0, 5, 10, 20],
    defensiveBonus : [0, 0,  0, 20],
    getIcon : function(rang)
	{
        if (rang === UNITRANKINGSYSTEM.co1)
		{
            return "co0";
		}
        else if (rang === UNITRANKINGSYSTEM.co2)
		{
            return "co1";
		}
		else
		{			
			return UNITRANKINGSYSTEM.icons[rang];
		}
    },
    // Promotions are earned by dealing damage: the funds value of the damage a unit deals fills its progress meter.
    // The meter of a rank is filled by damage worth the own costs of the unit multiplied with the factor of the rank.
    // e.g. a unit which costs 7000 needs to deal damage worth 7000 for the first rank and 10500 for the second one.
    rankCostFactor : [1.0, 1.5, 2.0],
    // set this to true to get a rank for each destroyed unit as well, like in the original game
    rankUpOnKill : false,
    progressVariable : "RANK_PROGRESS",
    // funds value of the damage needed to leave the given rank, 0 if there's no higher rank
    getRankCosts : function(unit, rang)
    {
        if (rang < 0 || rang >= UNITRANKINGSYSTEM.getMaxRang(unit))
        {
            return 0;
        }
        var factor = UNITRANKINGSYSTEM.rankCostFactor[rang];
        if (factor === undefined)
        {
            factor = UNITRANKINGSYSTEM.rankCostFactor[UNITRANKINGSYSTEM.rankCostFactor.length - 1];
        }
        var costs = Math.round(unit.getCosts() * factor);
        if (costs < 1)
        {
            costs = 1;
        }
        return costs;
    },
    // funds value of the damage dealt since the last promotion
    getProgress : function(unit)
    {
        return unit.getVariables().createVariable(UNITRANKINGSYSTEM.progressVariable).readDataInt32();
    },
    setProgress : function(unit, value)
    {
        unit.getVariables().createVariable(UNITRANKINGSYSTEM.progressVariable).writeDataInt32(value);
    },
    // share of the filled progress meter from 0 to 1, -1 for units which can't be promoted any further
    getProgressShare : function(unit)
    {
        var costs = UNITRANKINGSYSTEM.getRankCosts(unit, unit.getUnitRank());
        if (costs <= 0)
        {
            return -1;
        }
        var share = UNITRANKINGSYSTEM.getProgress(unit) / costs;
        if (share > 1)
        {
            share = 1;
        }
        return share;
    },
    // called after a battle with the funds value of the damage the unit dealt
    addDamageProgress : function(unit, damageValue)
    {
        if (unit === null || damageValue <= 0 || unit.getHp() <= 0)
        {
            return;
        }
        var rang = unit.getUnitRank();
        var costs = UNITRANKINGSYSTEM.getRankCosts(unit, rang);
        if (costs <= 0)
        {
            // co units and units with the highest rank
            return;
        }
        var progress = UNITRANKINGSYSTEM.getProgress(unit) + Math.round(damageValue);
        // damage above the meter is kept for the next rank
        while (costs > 0 && progress >= costs)
        {
            progress -= costs;
            rang = rang + 1;
            costs = UNITRANKINGSYSTEM.getRankCosts(unit, rang);
        }
        if (costs <= 0)
        {
            progress = 0;
        }
        UNITRANKINGSYSTEM.setProgress(unit, progress);
        if (rang !== unit.getUnitRank())
        {
            unit.setUnitRank(rang);
            UNITRANKINGSYSTEM.onRankGained(unit);
        }
    },

    // Promotion tree: each rank gives one promotion which is spend on one of the branches.
    // The bonus of a branch depends on how many promotions were spend on it.
    branches : ["OFFENSE", "DEFENSE", "UTILITY"],
    branchNames : [qsTr("Offense"), qsTr("Defense"), qsTr("Utility")],
    branchShortNames : [qsTr("O"), qsTr("D"), qsTr("U")],
    branchIcons : ["icon_fire", "defenseStar", "flareshot"],
    branchVariablePrefix : "PROMOTION_",
    // total bonus for 0, 1, 2 and 3 promotions of a branch
    branchOffensiveBonus : [0, 10, 20, 30],
    branchDefensiveBonus : [0, 10, 20, 30],
    utilityMovementBonus : [0, 1, 1, 2],
    utilityVisionBonus : [0, 1, 2, 3],
    permanentDuration : 1000000,
    getBranchPicks : function(unit, branch)
    {
        return unit.getVariables().createVariable(UNITRANKINGSYSTEM.branchVariablePrefix + branch).readDataInt32();
    },
    getTotalPicks : function(unit)
    {
        var total = 0;
        for (var i = 0; i < UNITRANKINGSYSTEM.branches.length; i++)
        {
            total += UNITRANKINGSYSTEM.getBranchPicks(unit, UNITRANKINGSYSTEM.branches[i]);
        }
        return total;
    },
    // promotions the unit earned but didn't spend yet
    getPendingPicks : function(unit)
    {
        var rang = unit.getUnitRank();
        if (rang <= 0)
        {
            return 0;
        }
        var pending = rang - UNITRANKINGSYSTEM.getTotalPicks(unit);
        if (pending < 0)
        {
            pending = 0;
        }
        return pending;
    },
    getTableValue : function(table, picks)
    {
        if (picks >= table.length)
        {
            picks = table.length - 1;
        }
        if (picks < 0)
        {
            picks = 0;
        }
        return table[picks];
    },
    // text shown for a branch in the promotion menu
    getBranchDescription : function(unit, branchIdx)
    {
        var branch = UNITRANKINGSYSTEM.branches[branchIdx];
        var next = UNITRANKINGSYSTEM.getBranchPicks(unit, branch) + 1;
        var text = UNITRANKINGSYSTEM.branchNames[branchIdx] + " " + next + ": ";
        if (branch === "OFFENSE")
        {
            text += "+" + UNITRANKINGSYSTEM.getTableValue(UNITRANKINGSYSTEM.branchOffensiveBonus, next) + qsTr("% attack");
        }
        else if (branch === "DEFENSE")
        {
            text += "+" + UNITRANKINGSYSTEM.getTableValue(UNITRANKINGSYSTEM.branchDefensiveBonus, next) + qsTr("% defense");
        }
        else
        {
            text += "+" + UNITRANKINGSYSTEM.getTableValue(UNITRANKINGSYSTEM.utilityMovementBonus, next) + qsTr(" move ") +
                    "+" + UNITRANKINGSYSTEM.getTableValue(UNITRANKINGSYSTEM.utilityVisionBonus, next) + qsTr(" vision");
        }
        return text;
    },
    // spends one promotion of the unit on the given branch
    pickBranch : function(unit, branch)
    {
        if (UNITRANKINGSYSTEM.getPendingPicks(unit) <= 0 ||
            UNITRANKINGSYSTEM.branches.indexOf(branch) < 0)
        {
            return false;
        }
        var picks = UNITRANKINGSYSTEM.getBranchPicks(unit, branch);
        unit.getVariables().createVariable(UNITRANKINGSYSTEM.branchVariablePrefix + branch).writeDataInt32(picks + 1);
        if (branch === "UTILITY")
        {
            // movement and vision are stored as lasting boni of the unit
            var movement = UNITRANKINGSYSTEM.getTableValue(UNITRANKINGSYSTEM.utilityMovementBonus, picks + 1) -
                           UNITRANKINGSYSTEM.getTableValue(UNITRANKINGSYSTEM.utilityMovementBonus, picks);
            var vision = UNITRANKINGSYSTEM.getTableValue(UNITRANKINGSYSTEM.utilityVisionBonus, picks + 1) -
                         UNITRANKINGSYSTEM.getTableValue(UNITRANKINGSYSTEM.utilityVisionBonus, picks);
            if (movement !== 0)
            {
                unit.addMovementBonus(movement, UNITRANKINGSYSTEM.permanentDuration);
            }
            if (vision !== 0)
            {
                unit.addVisionBonus(vision, UNITRANKINGSYSTEM.permanentDuration);
            }
        }
        return true;
    },
    // branch the ai picks for a unit: indirect units go for offense, capturing units for utility and defense,
    // all other units mix offense and defense.
    getAiBranch : function(unit)
    {
        var picks = UNITRANKINGSYSTEM.getTotalPicks(unit);
        var order = ["OFFENSE", "DEFENSE", "OFFENSE"];
        if (unit.getBaseMaxRange() > 1)
        {
            order = ["OFFENSE", "OFFENSE", "UTILITY"];
        }
        else if (unit.getActionList().indexOf("ACTION_CAPTURE") >= 0)
        {
            order = ["UTILITY", "DEFENSE", "DEFENSE"];
        }
        return order[picks % order.length];
    },
    isHumanUnit : function(unit)
    {
        var owner = unit.getOwner();
        if (owner === null)
        {
            return false;
        }
        var input = owner.getBaseGameInput();
        return input !== null && input.getAiType() === GameEnums.AiTypes_Human;
    },
    // called when a unit reached a new rank. Humans pick the branch with the promote action, the ai picks at once.
    onRankGained : function(unit)
    {
        if (UNITRANKINGSYSTEM.isHumanUnit(unit))
        {
            return;
        }
        var limit = 10;
        while (UNITRANKINGSYSTEM.getPendingPicks(unit) > 0 && limit > 0)
        {
            if (!UNITRANKINGSYSTEM.pickBranch(unit, UNITRANKINGSYSTEM.getAiBranch(unit)))
            {
                break;
            }
            limit--;
        }
    },
    // short text of the spend promotions e.g. "O1 D0 U2"
    getBranchSummary : function(unit)
    {
        var text = "";
        for (var i = 0; i < UNITRANKINGSYSTEM.branches.length; i++)
        {
            if (i > 0)
            {
                text += " ";
            }
            text += UNITRANKINGSYSTEM.branchShortNames[i] + UNITRANKINGSYSTEM.getBranchPicks(unit, UNITRANKINGSYSTEM.branches[i]);
        }
        return text;
    },
    increaseRang : function(unit)
	{
		var rang = unit.getUnitRank();
		if (rang >= 0)
		{
			rang = rang + 1;
            if (rang > UNITRANKINGSYSTEM.getMaxRang())
			{
                rang = UNITRANKINGSYSTEM.getMaxRang();
			}
		}
		unit.setUnitRank(rang);
        UNITRANKINGSYSTEM.onRankGained(unit);
	},
	unloadIcons : function(unit)
	{
        // unload all unit ranking icons here
        for (var i = 0; i < UNITRANKINGSYSTEM.icons.length; i++)
        {
            unit.unloadIcon(UNITRANKINGSYSTEM.icons[i]);
        }
        unit.unloadIcon("co0")
        unit.unloadIcon("co1")
	},
    getName : function(rang)
    {
        if (rang === UNITRANKINGSYSTEM.co2)
        {
            return qsTr("CO 1");
        }
        else if (rang === UNITRANKINGSYSTEM.co1)
        {
            return qsTr("CO 2");
        }
        else
        {
            return UNITRANKINGSYSTEM.names[rang];
        }
    },
    getOffensiveBonus : function(unit)
    {
        var rang = unit.getUnitRank();
        if (rang < 0)
        {
            return 20;
        }
        return UNITRANKINGSYSTEM.getTableValue(UNITRANKINGSYSTEM.branchOffensiveBonus, UNITRANKINGSYSTEM.getBranchPicks(unit, "OFFENSE"));
    },
    getDefensiveBonus : function(unit)
    {
        var rang = unit.getUnitRank();
        if (rang < 0)
        {
            return 20;
        }
        return UNITRANKINGSYSTEM.getTableValue(UNITRANKINGSYSTEM.branchDefensiveBonus, UNITRANKINGSYSTEM.getBranchPicks(unit, "DEFENSE"));
    },
};
