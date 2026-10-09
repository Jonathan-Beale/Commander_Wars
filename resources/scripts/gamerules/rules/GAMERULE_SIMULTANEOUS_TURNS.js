var Constructor = function()
{
    this.getRuleDescription = function(buggyVar, itemNumber, rule)
    {
        return qsTr("Prototype: all players act during the same day. Each round executes the next order of every player, the player acting first rotates. Orders which became impossible are cancelled.");
    };
    this.getRuleName = function(buggyVar, itemNumber, rule)
    {
        return qsTr("Simultaneous turns");
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
var GAMERULE_SIMULTANEOUS_TURNS = new Constructor();
