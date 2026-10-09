#include "coreengine/perftrace.h"
#include "resource_management/unitspritemanager.h"
#include "resource_management/movementtablemanager.h"
#include <QHash>
#include <QCoreApplication>

#include "ai/islandmap.h"

#include "coreengine/interpreter.h"
#include "coreengine/mainapp.h"

#include "game/gamemap.h"
#include "game/player.h"
#include "game/co.h"
#include "game/unitpathfindingsystem.h"

#include "3rd_party/oxygine-framework/oxygine/actor/ColorRectSprite.h"

IslandMap::IslandMap(GameMap* pMap, const QString & unitID, Player* pOwner, const QString & movementType)
      : m_pOwner(pOwner),
        m_pMap(pMap),
        m_MovementType(movementType)
{
#ifdef GRAPHICSUPPORT
    setObjectName("IslandMap");
#endif
    Interpreter::setCppOwnerShip(this);
    
    if (m_pMap != nullptr)
    {
        qint32 width = m_pMap->getMapWidth();
        qint32 heigth = m_pMap->getMapHeight();

        m_Islands.reserve(width);
        for (qint32 x = 0; x < width; x++)
        {
            m_Islands.push_back(std::vector<qint32>(heigth, UNKNOWN));
        }
        if (m_MovementType.isEmpty())
        {
            m_MovementType = UnitSpriteManager::getInstance()->getMovementType(unitID);
        }
        // The islands only depend on the terrains of the map, so they are reused until a terrain or building changes.
        // The ai drops its island maps several times each turn.
        struct CachedIslands
        {
            quint64 version{0};
            qint32 width{0};
            qint32 heigth{0};
            bool wrap{false};
            std::vector<std::vector<qint32>> islands;
        };
        static QHash<QString, CachedIslands> islandCache;
        static const bool useCache = !qEnvironmentVariableIsSet("COW_NO_ISLAND_CACHE");
        const quint64 version = MovementTableManager::getTerrainLayoutVersion();
        const QString key = QString::number(reinterpret_cast<quintptr>(pMap)) + ";" +
                            QString::number(reinterpret_cast<quintptr>(pOwner)) + ";" + m_MovementType +
                            // islands with a given movement type are built with the movement of this unit
                            (movementType.isEmpty() ? QString() : ";" + unitID);
        if (useCache)
        {
            auto iter = islandCache.constFind(key);
            if (iter != islandCache.constEnd() &&
                iter->version == version &&
                iter->width == width &&
                iter->heigth == heigth &&
                iter->wrap == HexGrid::wraps())
            {
                m_Islands = iter->islands;
                return;
            }
        }
        PerfTrace::mark("ai: island build " + m_MovementType);
        // the unit is only needed to build the islands
        spUnit pUnit = MemoryManagement::create<Unit>(unitID, pOwner, false, pMap);
        pUnit->setIgnoreUnitCollision(true);
        // The costs of movement types like hovercrafts are calculated by a script for each step.
        // They depend on the field which gets entered and on the kind of terrain the unit comes from,
        // so each field only needs to be asked once for each kind of terrain next to it.
        QHash<QString, bool> enterCache;
        auto canEnter = [this, &pUnit, &enterCache](qint32 x, qint32 y, qint32 curX, qint32 curY)
        {
            Terrain* pCurrent = m_pMap->getTerrain(curX, curY);
            const QString key = QString::number(x) + ";" + QString::number(y) + ";" + pCurrent->getID() + ";" + pCurrent->getBaseTerrainID();
            auto iter = enterCache.constFind(key);
            if (iter != enterCache.constEnd())
            {
                return iter.value();
            }
            // the modifiers of co's and weather never change if a field can be entered
            const bool enter = pUnit->getBaseMovementCosts(x, y, curX, curY) > 0;
            enterCache.insert(key, enter);
            return enter;
        };
        qint32 currentIsland = 0;
        // flood fill: each field is visited once, instead of running a path finding over the whole map for each island
        std::vector<QPoint> openFields;
        const qint32 neighbourCount = HexGrid::neighbourCount();
        Interpreter* pInterpreter = Interpreter::getInstance();
        for (qint32 x = 0; x < width; x++)
        {
            // Pumping the event loop is only safe outside a script call. Building an
            // island map from a js ai hook is otherwise a hard assert, and re-entering
            // the event loop there would be wrong even without one.
            if (!pInterpreter->getInJsCall())
            {
                pInterpreter->threadProcessEvents();
            }
            for (qint32 y = 0; y < heigth; y++)
            {
                if (m_Islands[x][y] < 0 && pUnit->canMoveOver(x, y))
                {
                    m_Islands[x][y] = currentIsland;
                    openFields.push_back(QPoint(x, y));
                    while (!openFields.empty())
                    {
                        const QPoint current = openFields.back();
                        openFields.pop_back();
                        for (qint32 i = 0; i < neighbourCount; ++i)
                        {
                            const qint32 nextX = HexGrid::wrapX(current.x() + HexGrid::neighbours[i].x());
                            const qint32 nextY = current.y() + HexGrid::neighbours[i].y();
                            if (nextX >= 0 && nextX < width &&
                                nextY >= 0 && nextY < heigth &&
                                m_Islands[nextX][nextY] < 0 &&
                                canEnter(nextX, nextY, current.x(), current.y()))
                            {
                                m_Islands[nextX][nextY] = currentIsland;
                                openFields.push_back(QPoint(nextX, nextY));
                            }
                        }
                    }
                    currentIsland++;
                }
            }
        }
        PerfTrace::mark("ai: island built");
        if (useCache)
        {
            CachedIslands entry;
            entry.version = version;
            entry.width = width;
            entry.heigth = heigth;
            entry.wrap = HexGrid::wraps();
            entry.islands = m_Islands;
            islandCache.insert(key, entry);
        }
    }
}

qint32 IslandMap::getIslandSize(qint32 island) const
{
    qint32 count = 0;
    for (const auto & data : std::as_const(m_Islands))
    {
        for (const auto & field : std::as_const(data))
        {
            if (field == island)
            {
                ++count;
            }
        }
    }
    return count;
}

QString IslandMap::getMovementType() const
{
    return m_MovementType;
}

void IslandMap::getValueOnIsland(qint32 island, qint32 &ownValue, qint32 & enemyValue)
{
    ownValue = 0;
    enemyValue = 0;
    
    if (m_pMap != nullptr)
    {
        qint32 width = m_pMap->getMapWidth();
        qint32 heigth = m_pMap->getMapHeight();
        for (qint32 x = 0; x < width; x++)
        {
            for (qint32 y = 0; y < heigth; y++)
            {
                if (m_Islands[HexGrid::wrapX(x)][y] == island)
                {
                    Unit* pUnit = m_pMap->getTerrain(x, y)->getUnit();
                    if (pUnit != nullptr)
                    {
                        if (pUnit->getOwner() == m_pOwner)
                        {
                            ownValue += pUnit->getCoUnitValue();
                        }
                        else if (m_pOwner->isEnemyUnit(pUnit))
                        {
                            enemyValue += pUnit->getCoUnitValue();
                        }
                    }
                }
            }
        }
    }
}

void IslandMap::show()
{
    m_info.reserve(m_Islands.size() * m_Islands[0].size());
    for (qint32 x = 0; x < m_Islands.size(); ++x)
    {
        for (qint32 y = 0; y < m_Islands[x].size(); ++y)
        {
            if (m_Islands[HexGrid::wrapX(x)][y] > UNKNOWN)
            {
                oxygine::spColorRectSprite sprite = MemoryManagement::create<oxygine::ColorRectSprite>();
                sprite->setSize(GameMap::getImageSize(), GameMap::getImageSize());
                QColor color = Qt::red;
                color.setAlphaF(0.5f);
                sprite->setColor(color);
                sprite->setPosition(x * GameMap::getImageSize(), y * GameMap::getImageSize());
                sprite->setPriority(static_cast<qint32>(Mainapp::ZOrder::MarkedFields));
                m_pMap->addChild(sprite);
                m_info.push_back(sprite);
            }
        }
    }
}

void IslandMap::hide()
{
    for (auto & field : m_info)
    {
        field->detachAndRemove();
    }
    m_info.clear();
}

