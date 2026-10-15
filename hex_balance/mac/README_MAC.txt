Commander Wars hex prototype on a Mac
=====================================

This folder is the source of the game. A Mac has to build it once, there is no ready made Mac version.

1. Install the command line tools of Xcode:   xcode-select --install
2. Install Homebrew from https://brew.sh
3. Open the Terminal in this folder and build:

       bash hex_balance/mac/build_mac.sh

   It installs cmake, ninja, Qt 6 and OpenSSL with Homebrew and compiles the game (several minutes).
   If it stops with an error, send build_mac_configure.log and build_mac.log to the host.

4. When the host waits in the player list of its match, join it:

       bash hex_balance/mac/join_mac.sh 192.168.1.20 yourname

   Use the address the host tells you. Press "Ready" when you see the players.

Notes
-----
* The connection isn't encrypted. Only play with people and in networks you trust.
* The game refuses to join when its scripts differ from the ones of the host: use the source bundle the host made last.
* If the match runs out of sync the game says so. Send build/balance/join_mac/console.log to the host.
  For more details start the join script with COW_SYNC_DEBUG=1 in front of it.
* This build was never tested on a Mac before. Building or playing may need fixes.
