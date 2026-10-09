var Constructor = function()
{
    this.loadPage = function(wikipage)
    {
        wikipage.loadHeadline(LOADEDWIKIPAGE.getName());
        wikipage.loadText(qsTr("Units earn promotions by dealing damage. The funds value of the damage a unit deals fills its progress meter, which is shown in the unit info. A rank is reached once the unit dealt damage worth its own costs multiplied with the factor of the rank. Damage above the meter counts for the next rank."));
        wikipage.loadText(qsTr("Each rank gives the unit one promotion. Select the unit and use the promote action to spend it on offense, defense or utility. The bonus of a branch grows with each promotion spend on it. Units of the ai pick their branch on their own."));
        for (var i = 0; i < UNITRANKINGSYSTEM.icons.length; i++)
        {
            wikipage.loadHeadline(UNITRANKINGSYSTEM.names[i]);
            wikipage.loadImage(UNITRANKINGSYSTEM.icons[i], 2);
            if (i < UNITRANKINGSYSTEM.rankCostFactor.length)
            {
                wikipage.loadText(qsTr("Damage needed for the next rank: ") + (UNITRANKINGSYSTEM.rankCostFactor[i] * 100) + qsTr("% of the costs of the unit"));
            }
        }
        wikipage.loadHeadline(qsTr("Promotion tree"));
        for (var picks = 1; picks < UNITRANKINGSYSTEM.branchOffensiveBonus.length; picks++)
        {
            wikipage.loadText(UNITRANKINGSYSTEM.branchNames[0] + " " + picks + ": +" + UNITRANKINGSYSTEM.branchOffensiveBonus[picks] + qsTr("% attack"));
        }
        for (var picks = 1; picks < UNITRANKINGSYSTEM.branchDefensiveBonus.length; picks++)
        {
            wikipage.loadText(UNITRANKINGSYSTEM.branchNames[1] + " " + picks + ": +" + UNITRANKINGSYSTEM.branchDefensiveBonus[picks] + qsTr("% defense"));
        }
        for (var picks = 1; picks < UNITRANKINGSYSTEM.utilityMovementBonus.length; picks++)
        {
            wikipage.loadText(UNITRANKINGSYSTEM.branchNames[2] + " " + picks + ": +" + UNITRANKINGSYSTEM.utilityMovementBonus[picks] + qsTr(" move ") +
                              "+" + UNITRANKINGSYSTEM.utilityVisionBonus[picks] + qsTr(" vision"));
        }
    };

    this.getTags = function()
    {
        return [qsTr("Others")];
    };

    this.getName = function()
    {
        return qsTr("Unit ranks");
    };
};

Constructor.prototype = WIKIPAGE;
var LOADEDWIKIPAGE = new Constructor();
