#include <QFileInfo>
#include <QDirIterator>

#include "coreengine/virtualpaths.h"

#include "resource_management/movementtablemanager.h"

#include "modding/csvtableimporter.h"

#include "game/unit.h"
#include "game/terrain.h"
#include "game/gamemap.h"

MovementTableManager::MovementTableManager()
    : RessourceManagement<MovementTableManager>("",
                                                "/scripts/movementtables")
{
    Interpreter::setCppOwnerShip(this);
#ifdef GRAPHICSUPPORT
    setObjectName("MovementTableManager");
#endif
}

void MovementTableManager::loadAll()
{
    RessourceManagement<MovementTableManager>::loadAll();
    // import csv tables
    // clean up temp folder
    Interpreter* pInterpreter = Interpreter::getInstance();
    QTemporaryDir tempDir = Settings::getInstance()->newTempDir();
    QStringList data;
    QFile file(VirtualPaths::find("resources/scripts/movementtables/movement_csv_import.txt"));
    if (file.open(QIODevice::ReadOnly))
    {
        QTextStream stream(&file);
        QString jsHeader = stream.readAll();
        QStringList searchPaths = getSearchPaths();
        for (auto & path : searchPaths)
        {
            QString jsData = CsvTableImporter::ImportCsvTable(path + "/movementtables.csv", jsHeader, "movementpointsTable", data);
            if (!jsData.isEmpty())
            {
                m_loadedRessources.append(data);
                pInterpreter->loadScript(jsData, tempDir.path() + "/movementtable.js");
            }
        }
    }
    else
    {
        CONSOLE_PRINT("Failed to open file " + file.fileName(), GameConsole::eERROR);
    }
    m_loadedRessources.sort();
}

QString MovementTableManager::getMovementName(const QString & id)
{
    return getName(id);
}

QStringList MovementTableManager::getMovementTypes()
{
    return getLoadedRessources();
}

qint32 MovementTableManager::getBaseMovementPoints(const QString & movementID, Terrain* pTerrain, Terrain* pCurrentTerrain, Unit* pUnit, bool trapChecking)
{
    QMutexLocker locker(&m_baseCostCacheMutex);
    BaseCostCache & cache = m_baseCostCache[movementID];
    if (cache.supportsCache < 0)
    {
        // COW_NO_MOVE_COST_CACHE switches the cache off to compare timings
        static const bool cacheDisabled = qEnvironmentVariableIsSet("COW_NO_MOVE_COST_CACHE");
        cache.supportsCache = (!cacheDisabled && getSupportsFastPfs(movementID)) ? 1 : 0;
    }
    const QPair<quintptr, quintptr> key(reinterpret_cast<quintptr>(pTerrain),
                                        reinterpret_cast<quintptr>(pUnit != nullptr ? pUnit->getOwner() : nullptr) | (trapChecking ? 1u : 0u));
    if (cache.supportsCache == 1)
    {
        auto iter = cache.costs.constFind(key);
        if (iter != cache.costs.constEnd())
        {
            return iter.value();
        }
    }
    Interpreter* pInterpreter = Interpreter::getInstance();
    QString function1 = "getMovementpoints";
    QJSValueList args({JsThis::getJsThis(pTerrain),
                       JsThis::getJsThis(pUnit),
                       JsThis::getJsThis(pCurrentTerrain),
                       trapChecking,
                       GameMap::getMapJsThis(pCurrentTerrain->getMap())});
    QJSValue ret = pInterpreter->doFunction(movementID, function1, args);
    qint32 costs = -1;
    if (ret.isNumber())
    {
        costs = ret.toInt();
    }
    if (cache.supportsCache == 1)
    {
        cache.costs.insert(key, costs);
    }
    return costs;
}

void MovementTableManager::clearBaseMovementPointsCache()
{
    m_movementStateVersion.fetchAndAddRelaxed(1);
    if (!created())
    {
        return;
    }
    MovementTableManager* pManager = getInstance();
    QMutexLocker locker(&pManager->m_baseCostCacheMutex);
    for (auto & cache : pManager->m_baseCostCache)
    {
        cache.costs.clear();
    }
}

void MovementTableManager::clearBaseMovementPointsCache(const Terrain* pTerrain)
{
    m_movementStateVersion.fetchAndAddRelaxed(1);
    if (!created())
    {
        return;
    }
    MovementTableManager* pManager = getInstance();
    QMutexLocker locker(&pManager->m_baseCostCacheMutex);
    const quintptr terrainKey = reinterpret_cast<quintptr>(pTerrain);
    for (auto & cache : pManager->m_baseCostCache)
    {
        cache.costs.removeIf([terrainKey](const auto & item)
        {
            return item.key().first == terrainKey;
        });
    }
}

bool MovementTableManager::getSupportsFastPfs(const QString & movementID)
{
    Interpreter* pInterpreter = Interpreter::getInstance();
    QString function1 = "getSupportsFastPfs";
    QJSValue ret = pInterpreter->doFunction(movementID, function1);
    if (ret.isBool())
    {
        return ret.toBool();
    }
    else
    {
        return true;
    }
}
