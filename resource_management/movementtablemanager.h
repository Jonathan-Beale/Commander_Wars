#ifndef MOVEMENTTABLEMANAGER_H
#define MOVEMENTTABLEMANAGER_H

#include <QHash>
#include <QMutex>

#include <QPair>

#include "resource_management/ressourcemanagement.h"

class Unit;
class Terrain;

class MovementTableManager final : public QObject, public RessourceManagement<MovementTableManager>
{
    Q_OBJECT
public:
    /**
     * @brief loadAll
     */
    virtual void loadAll() override;    /**
     * @brief getMovementName
     * @param id
     * @return
     */
    Q_INVOKABLE QString getMovementName(const QString & id);

    Q_INVOKABLE QStringList getMovementTypes();
public slots:
    qint32 getBaseMovementPoints(const QString & movementID, Terrain* pTerrain, Terrain* pCurrentTerrain, Unit* pUnit, bool trapChecking = false);
    bool getSupportsFastPfs(const QString & movementID);
public:
    /**
     * @brief clearBaseMovementPointsCache needs to be called whenever a terrain, a building or the owner of a building changes.
     * The base movement points of movement types supporting fast path finding only depend on those and on the unit owner,
     * so they're cached between the changes instead of calling the script for each field of each path search.
     */
    static void clearBaseMovementPointsCache();
    /**
     * @brief clearBaseMovementPointsCache only removes the costs of one terrain, e.g. when its building changes the owner.
     * Clearing everything for each captured building lets the scripts calculate the costs of the whole map again.
     */
    static void clearBaseMovementPointsCache(const Terrain* pTerrain);
    /**
     * @brief terrainLayoutChanged needs to be called whenever a terrain or a building of a map gets replaced, added or removed.
     * Data which only depends on the terrains of the map, like the island maps of the ai, is reused until this happens.
     */
    static void terrainLayoutChanged()
    {
        ++m_terrainLayoutVersion;
    }
    static quint64 getTerrainLayoutVersion()
    {
        return m_terrainLayoutVersion;
    }
    /**
     * @brief getMovementStateVersion changes whenever clearBaseMovementPointsCache gets called. Results of movement hooks
     * of scripts can be reused as long as it stays the same.
     */
    static quint64 getMovementStateVersion()
    {
        return m_movementStateVersion.loadRelaxed();
    }
    /**
     * @brief movementStateChanged needs to be called whenever the state of the match changes, e.g. an action was performed.
     * The base movement points only depend on terrains, buildings and their owners and are kept.
     */
    static void movementStateChanged()
    {
        m_movementStateVersion.fetchAndAddRelaxed(1);
    }
protected:
    friend MemoryManagement;
    MovementTableManager();
private:
    virtual ~MovementTableManager() = default;
    struct BaseCostCache
    {
        qint32 supportsCache{-1};
        // key: terrain and owner of the unit. The lowest bit of the owner holds the trap checking flag.
        QHash<QPair<quintptr, quintptr>, qint32> costs;
    };
    QHash<QString, BaseCostCache> m_baseCostCache;
    QMutex m_baseCostCacheMutex;
private:
    static inline quint64 m_terrainLayoutVersion{0};
    static inline QAtomicInteger<quint64> m_movementStateVersion{0};
};

#endif // MOVEMENTTABLEMANAGER_H
