// skips the menus and starts a match on the hex river map: you against the normal ai.
// Delete this file to get the main menu back.
var Init =
{
    main : function(menu)
    {
        // only start a match once. Coming back to the main menu after a match stays there.
        if (typeof hexAutomationStarted !== "undefined")
        {
            return;
        }
        hexAutomationStarted = true;
        menu.enterSingleplayer([]);
    },

    mapsSelection : function(menu)
    {
        // opening the map selection by hand later on shows the normal selection
        if (typeof hexAutomationMatchStarted !== "undefined")
        {
            return;
        }
        hexAutomationMatchStarted = true;
        menu.selectMap("maps/2_player/", "Hex River.map");
        menu.buttonNext();
        menu.buttonNext();
        var selection = menu.getPlayerSelection();
        selection.selectPlayerAi(0, 0);
        selection.selectPlayerAi(1, 2);
        menu.startGame();
    },
}
