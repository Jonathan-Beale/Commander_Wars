"""Builds a folder with everything another Windows computer needs to join a network match of the hex prototype.

    python hex_balance/make_package.py

creates build/package/CommanderWarsHex and build/package/CommanderWarsHex.zip. Copy the folder or the zip to the other
computer and start join.bat there. Nothing has to be installed on the other computer.

The package holds the game, its resources and maps and the Qt and MinGW libraries of this computer. The join check of the
game compares the scripts of both computers, so build a new package whenever the scripts or the game changed.
"""
import argparse
import os
import shutil
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
QT_BIN = r"C:\msys64\mingw64\bin"
EXE = os.path.join(REPO, "Commander_Wars.exe")
BUILD_EXE = os.path.join(REPO, "build", "Commander_Wars.exe")
# folders the game reads at runtime
DATA_FOLDERS = ["resources", "maps", "mods", "system", "data", "templates", "customTerrainImages"]
# libraries Qt loads on its own, which no import table lists
EXTRA_LIBRARIES = ["libssl-3-x64.dll", "libcrypto-3-x64.dll"]

JOIN_BAT = r"""@echo off
rem Joins a network match of the hex prototype. The host tells you the address of its computer.
setlocal
cd /d "%~dp0"
set /p HOSTADDRESS=Address of the host (e.g. 192.168.1.20):
set /p PLAYERNAME=Your name (different from the host):
if "%PLAYERNAME%"=="" set PLAYERNAME=guest
if not exist user_join mkdir user_join
powershell -NoProfile -Command "$u = ((Join-Path (Get-Location) 'user_join') -replace '\\', '/') + '/'; (Get-Content 'settings_template.ini') -replace '^Username=.*', ('Username=' + $env:PLAYERNAME) -replace '^UserPath=.*', ('UserPath=' + $u) | Set-Content 'user_join\Commander_Wars.ini'"
(
echo // written by join.bat
echo var Init =
echo {
echo     main : function^(menu^)
echo     {
echo         if ^(typeof hexNetStarted === "undefined"^)
echo         {
echo             hexNetStarted = true;
echo             menu.enterMultiplayer^(^);
echo         }
echo     },
echo     lobbyMenu : function^(menu^)
echo     {
echo         if ^(typeof hexNetLobby === "undefined"^)
echo         {
echo             hexNetLobby = true;
echo             menu.join^("%HOSTADDRESS%", ""^);
echo         }
echo     },
echo }
) > user_join\init.js
rem host and guests have to use the same connection settings
set COW_PLAIN_TCP=1
set COW_MENU_ENTER_DELAY_MS=1500
echo.
echo The game connects to %HOSTADDRESS% on port __PORT__. Press "Ready" once you see the players of the match.
start "" "%~dp0Commander_Wars.exe" --userPath "%~dp0user_join/" --spawnAiProcess 0
endlocal
"""

README = """Commander Wars hex prototype - network test package
====================================================

To join a match
---------------
1. Ask the host for the address of its computer (the IPv4 address shown by "ipconfig" on the host).
2. Start join.bat, type the address and a player name.
3. The game opens and connects on its own. When you see the list of players, press "Ready".
4. The host starts the match.

If it doesn't connect
---------------------
* The host has to be waiting in the player list of its match before you join.
* Both computers have to be in the same network, or the host has to forward port __PORT__ (TCP).
* The firewall of the host has to allow Commander_Wars.exe (Windows asks when the host starts for the first time).
* "Cannot join, your game data differs from the host": the package is older than the game of the host. Ask for a new one.

Notes
-----
* The connection isn't encrypted (COW_PLAIN_TCP): the encrypted handshake of this build crashes the host.
  Only play with people and in networks you trust.
* The settings and savegames of this package are in the folder user_join.
* If the match runs out of sync the game says so. The file user_join/console.log has the details: send it to the host.
"""


def imported_libraries(path):
    """names of the dlls the given exe or dll imports"""
    result = subprocess.run([os.path.join(QT_BIN, "objdump.exe"), "-p", path], capture_output=True, text=True, errors="replace")
    names = []
    for line in result.stdout.splitlines():
        line = line.strip()
        if line.startswith("DLL Name:"):
            names.append(line.split(":", 1)[1].strip())
    return names


def copy_libraries(target):
    """copies all libraries of the MSYS2 installation the files of the package need, also the ones needed by other libraries"""
    done = set(name.lower() for name in os.listdir(target))
    todo = [os.path.join(root, name) for root, _, files in os.walk(target) for name in files if name.lower().endswith((".exe", ".dll"))]
    copied = 0
    while todo:
        path = todo.pop()
        for name in imported_libraries(path):
            source = os.path.join(QT_BIN, name)
            if name.lower() not in done and os.path.exists(source):
                shutil.copy2(source, os.path.join(target, name))
                done.add(name.lower())
                todo.append(os.path.join(target, name))
                copied += 1
    return copied


def source_bundle(root):
    """Zips the source of the game for computers which have to build it themselves, e.g. a Mac: all files of the
    repository which aren't ignored, with the changes which aren't committed, and the libraries of the submodules."""
    import zipfile
    listed = subprocess.run(["git", "ls-files", "-co", "--exclude-standard"], cwd=REPO, capture_output=True, text=True,
                            encoding="utf-8", errors="replace").stdout.splitlines()
    files = []
    for name in listed:
        path = os.path.join(REPO, name)
        if os.path.isdir(path):
            # a submodule: git only lists its folder
            for folder, folders, names in os.walk(path):
                folders[:] = [entry for entry in folders if entry != ".git"]
                for entry in names:
                    if entry != ".git":
                        files.append(os.path.relpath(os.path.join(folder, entry), REPO))
        elif os.path.isfile(path):
            files.append(name)
    archive = os.path.join(root, "CommanderWarsHex-source.zip")
    os.makedirs(root, exist_ok=True)
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as handle:
        for name in files:
            handle.write(os.path.join(REPO, name), "CommanderWarsHex-source/" + name.replace("\\", "/"))
        handle.write(os.path.join(REPO, "hex_balance", "mac", "README_MAC.txt"), "CommanderWarsHex-source/README_MAC.txt")
    print("source: %s (%d files, %d MB)" % (archive, len(files), os.path.getsize(archive) // (1024 * 1024)))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--source", action="store_true",
                        help="create build/package/CommanderWarsHex-source.zip instead: the source for a computer which builds "
                             "the game itself, e.g. a Mac (see hex_balance/mac/README_MAC.txt)")
    parser.add_argument("--port", type=int, default=9101, help="port of the matches, the same as the --port of the host")
    parser.add_argument("--no-zip", action="store_true", help="only create the folder")
    args = parser.parse_args()

    if args.source:
        source_bundle(os.path.join(REPO, "build", "package"))
        return
    if os.path.exists(BUILD_EXE) and (not os.path.exists(EXE) or os.path.getmtime(BUILD_EXE) > os.path.getmtime(EXE)):
        shutil.copy2(BUILD_EXE, EXE)
    if not os.path.exists(EXE):
        sys.exit("missing " + EXE)
    root = os.path.join(REPO, "build", "package")
    target = os.path.join(root, "CommanderWarsHex")
    if os.path.exists(target):
        shutil.rmtree(target)
    os.makedirs(target)

    print("copying the game")
    shutil.copy2(EXE, os.path.join(target, "Commander_Wars.exe"))
    for folder in DATA_FOLDERS:
        source = os.path.join(REPO, folder)
        if os.path.isdir(source):
            shutil.copytree(source, os.path.join(target, folder))

    print("collecting the Qt libraries")
    env = dict(os.environ)
    env["PATH"] = QT_BIN + os.pathsep + env.get("PATH", "")
    deploy = subprocess.run([os.path.join(QT_BIN, "windeployqt6.exe"), "--no-translations", "--no-compiler-runtime",
                             "--dir", target, os.path.join(target, "Commander_Wars.exe")],
                            capture_output=True, text=True, errors="replace", env=env)
    if deploy.returncode != 0:
        print(deploy.stdout[-2000:])
        print(deploy.stderr[-2000:])
        sys.exit("windeployqt failed")
    for name in EXTRA_LIBRARIES:
        source = os.path.join(QT_BIN, name)
        if os.path.exists(source):
            shutil.copy2(source, os.path.join(target, name))
    print("copied %d further libraries" % copy_libraries(target))

    # settings of the guest: the port of the match, no lobby server and its own folder
    settings_file = os.path.join(REPO, "Commander_Wars.ini")
    lines = []
    if os.path.exists(settings_file):
        with open(settings_file, "r", encoding="utf-8", errors="replace") as handle:
            lines = handle.read().splitlines()
    replaced = []
    for line in lines:
        if line.startswith("UserPath="):
            # the user path comes from the command line of join.bat
            line = "UserPath="
        elif line.startswith("GamePort="):
            line = "GamePort=%d" % args.port
        elif line.startswith("ServerAdress="):
            line = "ServerAdress=127.0.0.1"
        elif line.startswith("SecondaryServerAdress="):
            line = "SecondaryServerAdress="
        elif line.startswith("Username="):
            line = "Username=guest"
        replaced.append(line)
    with open(os.path.join(target, "settings_template.ini"), "w", encoding="utf-8") as handle:
        handle.write("\n".join(replaced) + "\n")
    with open(os.path.join(target, "join.bat"), "w", encoding="ascii", newline="\r\n") as handle:
        handle.write(JOIN_BAT.replace("__PORT__", str(args.port)))
    with open(os.path.join(target, "README.txt"), "w", encoding="utf-8", newline="\r\n") as handle:
        handle.write(README.replace("__PORT__", str(args.port)))

    size = sum(os.path.getsize(os.path.join(folder, name)) for folder, _, files in os.walk(target) for name in files)
    print("package: %s (%d MB)" % (target, size // (1024 * 1024)))
    if not args.no_zip:
        print("writing the zip")
        archive = shutil.make_archive(target, "zip", root, "CommanderWarsHex")
        print("zip: %s (%d MB)" % (archive, os.path.getsize(archive) // (1024 * 1024)))


if __name__ == "__main__":
    main()
