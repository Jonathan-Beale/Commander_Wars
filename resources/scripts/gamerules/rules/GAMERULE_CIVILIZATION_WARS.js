var Constructor = function()
{
    this.getRuleDescription = function(buggyVar, itemNumber, rule)
    {
        return qsTr("Prototype: players own Food, Metal and Oil. Farmhouses, Lumber Mills, Mines and Oil Refineries create them from the terrain next to them. Infantry can build roads and buildings and clear forests.");
    };
    this.getRuleName = function(buggyVar, itemNumber, rule)
    {
        return qsTr("Civilization Wars");
    };
    this.getRuleType = function(itemNumber)
    {
        return ["checkbox"];
    };
    this.getRuleCategory = function(rule, itemNumber)
    {
        return GAMERULE.ruleCategories[GAMERULE.ruleCategoryGamePlay];
    };
};

Constructor.prototype = GAMERULE;
var GAMERULE_CIVILIZATION_WARS = new Constructor();
