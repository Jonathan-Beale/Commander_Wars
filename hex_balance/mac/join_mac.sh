#!/bin/bash
# Joins a network match of the hex prototype from a Mac. Build the game first with build_mac.sh.
#
#     bash hex_balance/mac/join_mac.sh <address of the host> [your name] [port]
#
# The game opens and connects on its own. Press "Ready" when you see the players of the match.
set -e
cd "$(dirname "$0")/../.."
ROOT="$(pwd)"

HOST="$1"
NAME="${2:-macguest}"
PORT="${3:-9101}"
if [ -z "$HOST" ]; then
    echo "usage: bash hex_balance/mac/join_mac.sh <address of the host> [your name] [port]"
    exit 1
fi
BINARY="$ROOT/build/Commander_Wars.app/Contents/MacOS/Commander_Wars"
if [ ! -x "$BINARY" ]; then
    BINARY="$ROOT/build/Commander_Wars"
fi
if [ ! -x "$BINARY" ]; then
    echo "The game isn't built yet. Run: bash hex_balance/mac/build_mac.sh"
    exit 1
fi

USERDIR="$ROOT/build/balance/join_mac"
mkdir -p "$USERDIR"
# only the settings which matter for the match, the game adds the rest with its defaults
cat > "$USERDIR/Commander_Wars.ini" <<INI
[%General]
UserPath=$USERDIR/

[Game]
Username=$NAME
UseAiProcess=false
HexGrid=true

[Network]
GamePort=$PORT
ServerAdress=127.0.0.1
SecondaryServerAdress=
INI
cat > "$USERDIR/init.js" <<JS
// written by join_mac.sh
var Init =
{
    main : function(menu)
    {
        if (typeof hexNetStarted === "undefined")
        {
            hexNetStarted = true;
            menu.enterMultiplayer();
        }
    },
    lobbyMenu : function(menu)
    {
        if (typeof hexNetLobby === "undefined")
        {
            hexNetLobby = true;
            menu.join("$HOST", "");
        }
    },
}
JS

# host and guests have to use the same connection settings
export COW_PLAIN_TCP=1
export COW_MENU_ENTER_DELAY_MS=1500
echo "connecting to $HOST on port $PORT as $NAME. The log of the game is $USERDIR/console.log"
# the game finds its resources and maps in the current folder
exec "$BINARY" --userPath "$USERDIR/" --spawnAiProcess 0
