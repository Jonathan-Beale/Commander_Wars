#include <QFileInfo>

#include "coreengine/virtualpaths.h"

#include "resource_management/weaponmanager.h"

#include "game/unit.h"

#include "modding/csvtableimporter.h"

float WeaponManager::getBaseDamage(const QString & weaponID, Unit* pDefender)
{
    if (!weaponID.isEmpty())
    {
        Interpreter* pInterpreter = Interpreter::getInstance();
        QString function1 = "getBaseDamage";
        QJSValueList args({JsThis::getJsThis(pDefender)});
        QJSValue erg = pInterpreter->doFunction(weaponID, function1, args);
        if (erg.isNumber())
        {
            return erg.toNumber();
        }
        else
        {
            return -1.0f;
        }
    }
    else
    {
        return -1.0f;
    }
}

float WeaponManager::getEnviromentDamage(const QString & weaponID, const QString & terrainID)
{
    // The damage tables of the weapons don't change. The ai asks for each field of the map for each unit,
    // so the results are stored instead of calling the script each time.
    static thread_local QHash<QString, QHash<QString, float>> cache;
    static thread_local Interpreter* pCacheInterpreter = nullptr;
    Interpreter* pInterpreter = Interpreter::getInstance();
    if (pCacheInterpreter != pInterpreter)
    {
        // the scripts were loaded again
        cache.clear();
        pCacheInterpreter = pInterpreter;
    }
    auto & weaponCache = cache[weaponID];
    auto iter = weaponCache.constFind(terrainID);
    if (iter != weaponCache.constEnd())
    {
        return iter.value();
    }
    QString function1 = "getEnviromentDamage";
    QJSValueList args({terrainID});
    QJSValue erg = pInterpreter->doFunction(weaponID, function1, args);
    float damage = -1.0f;
    if (erg.isNumber())
    {
        damage = erg.toNumber();
    }
    weaponCache.insert(terrainID, damage);
    return damage;
}

QString WeaponManager::getWeaponName(const QString & id)
{
    return getName(id);
}

void WeaponManager::loadAll()
{
    RessourceManagement<WeaponManager>::loadAll();
    // import csv tables
    // clean up temp folder
    Interpreter* pInterpreter = Interpreter::getInstance();
    QTemporaryDir tempDir = Settings::getInstance()->newTempDir();
    QStringList data;
    QFile file(VirtualPaths::find("resources/scripts/weapons/weapon_csv_import.txt"));
    if (file.open(QIODevice::ReadOnly))
    {
        QTextStream stream(&file);
        QString jsHeader = stream.readAll();
        QStringList searchPaths = getSearchPaths();
        for (auto & path : searchPaths)
        {
            QString jsData = CsvTableImporter::ImportCsvTable(path + "/weapontable.csv", jsHeader, "damageTable", data);
            if (!jsData.isEmpty())
            {
                m_loadedRessources.append(data);
                pInterpreter->loadScript(jsData, tempDir.path() + "/weaponttable.js");
            }
        }
    }
    else
    {
        CONSOLE_PRINT("Failed to open file " + file.fileName(), GameConsole::eERROR);
    }
}
