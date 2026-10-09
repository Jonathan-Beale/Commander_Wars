#include <QDataStream>
#include <QDirIterator>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QDir>
#include <QFile>

#include "coreengine/mainapp.h"
#include "coreengine/settings.h"
#include "coreengine/gameconsole.h"

#include "game/gamemap.h"
#include "game/terrain.h"
#include "game/unit.h"
#include "game/building.h"
#include "game/player.h"
#include "game/co.h"

#include "ai/coreai.h"

void GameMap::extendMap(const QString mapFile, GameEnums::Directions direction)
{
    QString file = mapFile;
    if (!QFile::exists(file))
    {
        file = oxygine::Resource::RCC_PREFIX_PATH + mapFile;
    }
    if (QFile::exists(file) &&
        (direction == GameEnums::Directions_North ||
         direction == GameEnums::Directions_South ||
         direction == GameEnums::Directions_East ||
         direction == GameEnums::Directions_West))
    {
        Mainapp::getInstance()->pauseRendering();
        {
            spGameMap pMap = MemoryManagement::create<GameMap>(file, true, false, false);
            qint32 endWidth = 0;
            qint32 endHeight = 0;
            qint32 offsetX = 0;
            qint32 offsetY = 0;
            if (direction == GameEnums::Directions_North)
            {
                resizeMap(0, pMap->getMapHeight(), 0, 0);
                endWidth = getMapWidth();
                endHeight = pMap->getMapHeight();
            }
            else if (direction == GameEnums::Directions_South)
            {
                offsetY = getMapHeight();
                resizeMap(0, 0, 0, pMap->getMapHeight());
                endWidth = getMapWidth();
                endHeight = pMap->getMapHeight();
            }
            else if (direction == GameEnums::Directions_East)
            {
                offsetX = getMapWidth();
                resizeMap(0, 0, pMap->getMapWidth(), 0);
                endWidth = pMap->getMapWidth();
                endHeight = getMapHeight();
            }
            else if (direction == GameEnums::Directions_West)
            {
                resizeMap(pMap->getMapWidth(), 0, 0, 0);
                endWidth = pMap->getMapWidth();
                endHeight = getMapHeight();
            }
            for (qint32 y = 0; y < endHeight; ++y)
            {
                for (qint32 x = 0; x < endWidth; ++x)
                {
                    qint32 targetX = x + offsetX;
                    qint32 targetY = y + offsetY;
                    if (pMap->onMap(x, y))
                    {
                        spTerrain pTerrain = pMap->m_fields[y][HexGrid::wrapX(x)];
                        pMap->m_fields[y][HexGrid::wrapX(x)] = spTerrain();
                        m_fields[targetY][HexGrid::wrapX(targetX)]->detachAndRemove();
                        pTerrain->setX(targetX);
                        pTerrain->setY(targetY);
                        m_fields[targetY][HexGrid::wrapX(targetX)] = pTerrain;
                        m_rowSprites[targetY]->addChild(pTerrain);
                        pTerrain->setPosition(HexGrid::fieldToScreen(targetX, targetY, m_imagesize));
                        pTerrain->setMapForExtending(this);
                        Unit* pUnit = pTerrain->getUnit();
                        if (pUnit != nullptr)
                        {
                            pUnit->setAiMode(pUnit->getAiMode());
                        }
                    }
                    else
                    {
                        spTerrain pTerrain = Terrain::createTerrain(GameMap::PLAINS, targetX, targetY, "", this);
                        m_rowSprites[y]->addChild(pTerrain);
                        m_fields[targetY][HexGrid::wrapX(targetX)] = pTerrain;
                        m_rowSprites[targetY]->addChild(pTerrain);
                        pTerrain->setPosition(HexGrid::fieldToScreen(targetX, targetY, m_imagesize));
                    }
                }
            }
        }
        updateSprites(-1, -1, true);
        for (qint32 i = 0; i < m_players.size(); ++i)
        {
            m_players[i]->loadVisionFields();
            CoreAI* pAi = dynamic_cast<CoreAI*>(m_players[i]->getBaseGameInput());
            if (pAi != nullptr)
            {
                pAi->resetToTurnStart();
            }
        }
        registerMapAtInterpreter();
        m_gameRules->resetFogSprites();
        m_gameRules->createWeatherSprites();
        m_GameScript->init();
        Mainapp::getInstance()->continueRendering();
    }
}

void GameMap::newMap(qint32 width, qint32 heigth, qint32 playerCount, const QString baseTerrain)
{
    Mainapp::getInstance()->pauseRendering();
    clearMap();

    m_fields.reserve(heigth);
    m_rowSprites.reserve(heigth);
    for (qint32 y = 0; y < heigth; y++)
    {
        m_fields.push_back(std::vector<spTerrain>(width, spTerrain()));
        auto pActor = MemoryManagement::create<oxygine::Actor>();
        pActor->setPriority(static_cast<qint32>(Mainapp::ZOrder::Terrain) + y);
        m_rowSprites.push_back(pActor);
        addChild(pActor);
        for (qint32 x = 0; x < width; x++)
        {
            spTerrain pTerrain = Terrain::createTerrain(baseTerrain, x, y, "", this);
            m_rowSprites[y]->addChild(pTerrain);
            m_fields[y][HexGrid::wrapX(x)] = pTerrain;
            pTerrain->setPosition(HexGrid::fieldToScreen(x, y, m_imagesize));
        }
    }
    // add two players to a default map :)
    for (qint32 i = 0; i < playerCount; i++)
    {
        m_players.append(MemoryManagement::create<Player>(this));
        m_players[m_players.size() - 1]->init();
        if (i == 0)
        {
            m_players[i]->setBaseGameInput(BaseGameInputIF::createAi(this, GameEnums::AiTypes::AiTypes_Human));
            m_players[i]->setControlType(GameEnums::AiTypes::AiTypes_Human);
        }
        else
        {
            m_players[i]->setBaseGameInput(BaseGameInputIF::createAi(this, GameEnums::AiTypes::AiTypes_Normal));
            m_players[i]->setControlType(GameEnums::AiTypes::AiTypes_Normal);
        }
    }
    updateSprites(-1, -1, true);
    centerMap(width / 2, heigth / 2);
    Mainapp::getInstance()->continueRendering();
}

void GameMap::changeMap(qint32 width, qint32 heigth, qint32 playerCount)
{
    Mainapp::getInstance()->pauseRendering();
    qint32 currentHeigth = getMapHeight();
    qint32 currentWidth = getMapWidth();
    qint32 currentPlayerCount = getPlayerCount();

    if (width > currentWidth)
    {
        for (qint32 y = 0; y < currentHeigth; y++)
        {
            for (qint32 x = currentWidth; x < width; x++)
            {
                spTerrain pTerrain = Terrain::createTerrain(GameMap::PLAINS, x, y, "", this);
                m_rowSprites[y]->addChild(pTerrain);
                m_fields[y].push_back(pTerrain);
                pTerrain->setPosition(HexGrid::fieldToScreen(x, y, m_imagesize));
            }
        }
    }
    else if (width < currentWidth)
    {
        for (qint32 y = 0; y < currentHeigth; y++)
        {
            while (width < m_fields[y].size())
            {
                m_fields[y][m_fields[y].size() - 1]->detachAndRemove();
                m_fields[y].pop_back();
            }
        }
    }

    if (heigth > currentHeigth)
    {
        for (qint32 y = currentHeigth; y < heigth; y++)
        {
            m_fields.push_back(std::vector<spTerrain>());
            auto pActor = MemoryManagement::create<oxygine::Actor>();
            pActor->setPriority(static_cast<qint32>(Mainapp::ZOrder::Terrain) + y);
            m_rowSprites.push_back(pActor);
            addChild(pActor);
            for (qint32 x = 0; x < width; x++)
            {
                spTerrain pTerrain = Terrain::createTerrain(GameMap::PLAINS, x, y, "", this);
                m_rowSprites[y]->addChild(pTerrain);
                m_fields[y].push_back(pTerrain);
                pTerrain->setPosition(HexGrid::fieldToScreen(x, y, m_imagesize));
            }
        }
    }
    else if (heigth < currentHeigth)
    {
        while (heigth < m_fields.size())
        {
            for (qint32 x = 0; x < m_fields[m_fields.size() - 1].size(); x++)
            {
                m_fields[m_fields.size() - 1][HexGrid::wrapX(x)]->detachAndRemove();
            }
            m_fields[m_fields.size() - 1].clear();
            m_fields.pop_back();
        }
    }

    if (playerCount > currentPlayerCount)
    {
        while (playerCount > m_players.size())
        {
            m_players.append(MemoryManagement::create<Player>(this));
            m_players[m_players.size() - 1]->init();
        }
    }
    else if (playerCount < currentPlayerCount)
    {
        while (playerCount < m_players.size())
        {
            for (qint32 y = 0; y < heigth; y++)
            {
                for (qint32 x = 0; x < width; x++)
                {
                    Terrain* pTerrain = getTerrain(x, y);
                    Building* pBuilding = pTerrain->getBuilding();
                    if (pBuilding != nullptr &&
                        pBuilding->getOwner() == m_players.last().get())
                    {
                        pBuilding->setOwner(nullptr);
                    }
                    Unit* pUnit = pTerrain->getUnit();
                    if (pUnit != nullptr &&
                        pUnit->getOwner() == m_players.last().get())
                    {
                        pTerrain->setUnit(spUnit());
                    }
                }
            }

            m_players.removeLast();
        }
    }
    updateSprites(-1, -1, true);
    centerMap(width / 2, heigth / 2);
    Mainapp::getInstance()->continueRendering();
}

void GameMap::resizeMap(qint32 left, qint32 top, qint32 right, qint32 bottom)
{
    Mainapp::getInstance()->pauseRendering();
    qint32 currentHeigth = getMapHeight();
    if (left + getMapWidth() > 0)
    {
        if (left > 0)
        {
            for (qint32 i = 0; i < left; i++)
            {
                for (qint32 y = 0; y < currentHeigth; y++)
                {
                    spTerrain pTerrain = Terrain::createTerrain(GameMap::PLAINS, -1, -1, "", this);
                    m_rowSprites[y]->addChild(pTerrain);
                    m_fields[y].insert(m_fields[y].begin(), pTerrain);
                }
            }
        }
        else if (left < 0)
        {
            for (qint32 i = 0; i < -left; i++)
            {
                for (qint32 y = 0; y < currentHeigth; y++)
                {
                    m_fields[y][HexGrid::wrapX(0)]->detachAndRemove();
                    m_fields[y].erase(m_fields[y].begin());
                }
            }
        }
    }
    if (right + getMapWidth() > 0)
    {
        if (right > 0)
        {
            for (qint32 i = 0; i < right; i++)
            {
                for (qint32 y = 0; y < currentHeigth; y++)
                {
                    spTerrain pTerrain = Terrain::createTerrain(GameMap::PLAINS, -1, -1, "", this);
                    m_rowSprites[y]->addChild(pTerrain);
                    m_fields[y].push_back(pTerrain);
                }
            }
        }
        else if (right < 0)
        {
            for (qint32 i = 0; i < -right; i++)
            {
                for (qint32 y = 0; y < currentHeigth; y++)
                {
                    m_fields[y].back()->detachAndRemove();
                    m_fields[y].pop_back();
                }
            }
        }
    }
    qint32 currentWidth = getMapWidth();
    if (top + getMapHeight() > 0)
    {
        if (top > 0)
        {
            for (qint32 i = 0; i < top; i++)
            {
                m_fields.insert(m_fields.begin(), std::vector<spTerrain>());
                auto pActor = MemoryManagement::create<oxygine::Actor>();
                m_rowSprites.push_back(pActor);
                addChild(pActor);
                for (qint32 x = 0; x < currentWidth; x++)
                {
                    spTerrain pTerrain = Terrain::createTerrain(GameMap::PLAINS, -1, -1, "", this);
                    m_rowSprites[i]->addChild(pTerrain);
                    m_fields[0].push_back(pTerrain);
                }
            }
        }
        else if (top < 0)
        {
            for (qint32 i = 0; i < -top; i++)
            {
                for (qint32 x = 0; x < currentWidth; x++)
                {
                    m_fields[0][HexGrid::wrapX(x)]->detachAndRemove();
                }
                m_fields.erase(m_fields.begin());
                m_rowSprites[0]->detachAndRemove();
                m_rowSprites.erase(m_rowSprites.cbegin());
            }
        }
    }
    if (bottom + getMapHeight() > 0)
    {
        if (bottom > 0)
        {
            for (qint32 i = 0; i < bottom; i++)
            {
                m_fields.push_back(std::vector<spTerrain>());
                auto pActor = MemoryManagement::create<oxygine::Actor>();
                m_rowSprites.push_back(pActor);
                addChild(pActor);
                for (qint32 x = 0; x < currentWidth; x++)
                {
                    spTerrain pTerrain = Terrain::createTerrain(GameMap::PLAINS, -1, -1, "", this);
                    m_rowSprites[i]->addChild(pTerrain);
                    m_fields.back().push_back(pTerrain);
                }
            }
        }
        else if (bottom < 0)
        {
            for (qint32 i = 0; i < -bottom; i++)
            {
                for (qint32 x = 0; x < currentWidth; x++)
                {
                    m_fields.back()[x]->detachAndRemove();
                }
                m_fields.pop_back();
                m_rowSprites[m_rowSprites.size() - 1]->detachAndRemove();
                m_rowSprites.pop_back();
            }
        }
    }
    currentWidth = getMapWidth();
    currentHeigth = getMapHeight();
    for (qint32 y = 0; y < currentHeigth; y++)
    {
        m_rowSprites[y]->setPriority(static_cast<qint32>(Mainapp::ZOrder::Terrain) + y);
        for (qint32 x = 0; x < currentWidth; x++)
        {
            spTerrain pTerrain = m_fields[y][HexGrid::wrapX(x)];
            spUnit pUnit = pTerrain->getSpUnit();
            spBuilding pBuilding = pTerrain->getSpBuilding();
            pTerrain->setPosition(HexGrid::fieldToScreen(x, y, m_imagesize));
            pTerrain->setX(x);
            pTerrain->setY(y);
            pTerrain->setPriority(pTerrain->getMapTerrainDrawPriority());
            pTerrain->setUnit(pUnit);
        }
    }
    for (qint32 y = 0; y < currentHeigth; y++)
    {
        for (qint32 x = 0; x < currentWidth; x++)
        {
            spBuilding pBuilding = m_fields[y][HexGrid::wrapX(x)]->getSpBuilding();
            if (pBuilding.get() != nullptr)
            {
                qint32 buildX = pBuilding->Building::getX();
                qint32 buildY = pBuilding->Building::getY();
                if (buildX - pBuilding->getBuildingWidth() + 1 < 0 ||
                    buildY - pBuilding->getBuildingHeigth() + 1 < 0)
                {
                    m_fields[y][HexGrid::wrapX(x)]->setBuilding(spBuilding());
                }
                else
                {
                    m_fields[buildY][HexGrid::wrapX(buildX)]->setBuilding(pBuilding);
                }
            }
        }
    }
    updateSprites(-1, -1, true);
    centerMap(currentWidth / 2, currentHeigth / 2);
    Mainapp::getInstance()->continueRendering();
}

void GameMap::flipX()
{
    Mainapp::getInstance()->pauseRendering();
    qint32 currentHeigth = getMapHeight();
    qint32 currentWidth = getMapWidth();
    for (qint32 y = 0; y < currentHeigth; y++)
    {
        for (qint32 x = currentWidth / 2; x < currentWidth; x++)
        {
            spTerrain currentTerrain = m_fields[y][HexGrid::wrapX(x)];
            currentTerrain->detachAndRemove();
            spTerrain flipTerrain = m_fields.at(y)[currentWidth - x - 1];
            spTerrain pTerrain = Terrain::createTerrain(flipTerrain->getTerrainID(), x, y, flipTerrain->getBaseTerrainID(), this);
            m_rowSprites[y]->addChild(pTerrain);
            m_fields[y][HexGrid::wrapX(x)] = pTerrain;
            pTerrain->setPosition(HexGrid::fieldToScreen(x, y, m_imagesize));

            spBuilding pCurrentBuilding = flipTerrain->getSpBuilding();
            if (flipTerrain->getBuilding() != nullptr)
            {
                spBuilding pBuilding = MemoryManagement::create<Building>(pCurrentBuilding->getBuildingID(), this);
                pBuilding->setOwner(pCurrentBuilding->getOwner());
                pTerrain->setBuilding(pBuilding);
            }

            spUnit pCurrentUnit = flipTerrain->getSpUnit();
            if (pCurrentUnit.get() != nullptr)
            {
                spUnit pUnit = MemoryManagement::create<Unit>(pCurrentUnit->getUnitID(), pCurrentUnit->getOwner(), false, this);
                pTerrain->setUnit(pUnit);
                pUnit->setAiMode(pCurrentUnit->getAiMode());
            }
        }
    }
    updateSprites(-1, -1, true);
    centerMap(getMapWidth() / 2, getMapHeight() / 2);
    Mainapp::getInstance()->continueRendering();
}

void GameMap::rotateX()
{
    Mainapp::getInstance()->pauseRendering();
    qint32 currentHeigth = getMapHeight();
    qint32 currentWidth = getMapWidth();
    for (qint32 y = 0; y < currentHeigth; y++)
    {
        for (qint32 x = currentWidth / 2; x < currentWidth; x++)
        {
            spTerrain currentTerrain = m_fields[y][HexGrid::wrapX(x)];
            currentTerrain->detachAndRemove();
            spTerrain flipTerrain = m_fields[currentHeigth - y - 1][HexGrid::wrapX(currentWidth - x - 1)];
            spTerrain pTerrain = Terrain::createTerrain(flipTerrain->getTerrainID(), x, y, flipTerrain->getBaseTerrainID(), this);
            m_rowSprites[y]->addChild(pTerrain);
            m_fields[y][HexGrid::wrapX(x)] = pTerrain;
            pTerrain->setPosition(HexGrid::fieldToScreen(x, y, m_imagesize));

            spBuilding pCurrentBuilding = flipTerrain->getSpBuilding();
            if (flipTerrain->getBuilding() != nullptr)
            {
                spBuilding pBuilding = MemoryManagement::create<Building>(pCurrentBuilding->getBuildingID(), this);
                pBuilding->setOwner(pCurrentBuilding->getOwner());
                pTerrain->setBuilding(pBuilding);
            }

            spUnit pCurrentUnit = flipTerrain->getSpUnit();
            if (pCurrentUnit.get() != nullptr)
            {
                spUnit pUnit = MemoryManagement::create<Unit>(pCurrentUnit->getUnitID(), pCurrentUnit->getOwner(), false, this);
                pTerrain->setUnit(pUnit);
                pUnit->setAiMode(pCurrentUnit->getAiMode());
            }
        }
    }
    updateSprites(-1, -1, true);
    centerMap(getMapWidth() / 2, getMapHeight() / 2);
    Mainapp::getInstance()->continueRendering();
}

void GameMap::rotateX90()
{
    Mainapp::getInstance()->pauseRendering();
    qint32 currentHeigth = getMapHeight();
    qint32 currentWidth = getMapWidth();
    // upper left quarter gets rotated to upper right
    for (qint32 y = 0; y < currentHeigth / 2; y++)
    {
        for (qint32 x = 0; x < currentWidth / 2; x++)
        {
            spTerrain sourceTerrain = m_fields[y][HexGrid::wrapX(x)];
            qint32 targetX = currentWidth - x - 1;
            qint32 targetY = currentHeigth / 2 - y - 1;
            spTerrain targetTerrain = m_fields[targetY][HexGrid::wrapX(targetX)];
            targetTerrain->detachAndRemove();
            targetTerrain = Terrain::createTerrain(sourceTerrain->getTerrainID(), targetX, targetY, sourceTerrain->getBaseTerrainID(), this);
            m_rowSprites[targetY]->addChild(targetTerrain);
            m_fields[targetY][HexGrid::wrapX(targetX)] = targetTerrain;
            targetTerrain->setPosition(HexGrid::fieldToScreen(targetX, targetY, m_imagesize));

            spBuilding pCurrentBuilding = sourceTerrain->getSpBuilding();
            if (sourceTerrain->getBuilding() != nullptr)
            {
                spBuilding pBuilding = MemoryManagement::create<Building>(pCurrentBuilding->getBuildingID(), this);
                pBuilding->setOwner(pCurrentBuilding->getOwner());
                targetTerrain->setBuilding(pBuilding);
            }

            spUnit pCurrentUnit = sourceTerrain->getSpUnit();
            if (pCurrentUnit.get() != nullptr)
            {
                spUnit pUnit = MemoryManagement::create<Unit>(pCurrentUnit->getUnitID(), pCurrentUnit->getOwner(), false, this);
                targetTerrain->setUnit(pUnit);
                pUnit->setAiMode(pCurrentUnit->getAiMode());
            }
        }
    }
    // lower right quarter gets rotated to lower left
    qint32 startY = GlobalUtils::roundUp(static_cast<float>(currentHeigth) / 2.0f);
    qint32 startX = GlobalUtils::roundUp(static_cast<float>(currentWidth) / 2.0f);
    for (qint32 y = startY; y < currentHeigth; y++)
    {
        for (qint32 x = startX; x < currentWidth; x++)
        {
            spTerrain sourceTerrain = m_fields[y][HexGrid::wrapX(x)];
            qint32 targetX = currentWidth - x - 1;
            qint32 targetY = currentHeigth - 1 - (y - startY);
            spTerrain targetTerrain = m_fields[targetY][HexGrid::wrapX(targetX)];
            targetTerrain->detachAndRemove();
            targetTerrain = Terrain::createTerrain(sourceTerrain->getTerrainID(), targetX, targetY, sourceTerrain->getBaseTerrainID(), this);
            m_rowSprites[targetY]->addChild(targetTerrain);
            m_fields[targetY][HexGrid::wrapX(targetX)] = targetTerrain;
            targetTerrain->setPosition(HexGrid::fieldToScreen(targetX, targetY, m_imagesize));

            spBuilding pCurrentBuilding = sourceTerrain->getSpBuilding();
            if (sourceTerrain->getBuilding() != nullptr)
            {
                spBuilding pBuilding = MemoryManagement::create<Building>(pCurrentBuilding->getBuildingID(), this);
                pBuilding->setOwner(pCurrentBuilding->getOwner());
                targetTerrain->setBuilding(pBuilding);
            }

            spUnit pCurrentUnit = sourceTerrain->getSpUnit();
            if (pCurrentUnit.get() != nullptr)
            {
                spUnit pUnit = MemoryManagement::create<Unit>(pCurrentUnit->getUnitID(), pCurrentUnit->getOwner(), false, this);
                targetTerrain->setUnit(pUnit);
                pUnit->setAiMode(pCurrentUnit->getAiMode());
            }
        }
    }

    updateSprites(-1, -1, true);
    centerMap(getMapWidth() / 2, getMapHeight() / 2);
    Mainapp::getInstance()->continueRendering();
}

void GameMap::rotateY90()
{
    Mainapp::getInstance()->pauseRendering();
    qint32 currentHeigth = getMapHeight();
    qint32 currentWidth = getMapWidth();
    // upper left quarter gets rotated to lower left
    for (qint32 y = 0; y < currentHeigth / 2; y++)
    {
        for (qint32 x = 0; x < currentWidth / 2; x++)
        {
            spTerrain sourceTerrain = m_fields[y][HexGrid::wrapX(x)];
            qint32 targetX = currentWidth / 2 - x - 1;
            qint32 targetY = currentHeigth - y - 1;
            spTerrain targetTerrain = m_fields[targetY][HexGrid::wrapX(targetX)];
            targetTerrain->detachAndRemove();
            targetTerrain = Terrain::createTerrain(sourceTerrain->getTerrainID(), targetX, targetY, sourceTerrain->getBaseTerrainID(), this);
            m_rowSprites[targetY]->addChild(targetTerrain);
            m_fields[targetY][HexGrid::wrapX(targetX)] = targetTerrain;
            targetTerrain->setPosition(HexGrid::fieldToScreen(targetX, targetY, m_imagesize));

            spBuilding pCurrentBuilding = sourceTerrain->getSpBuilding();
            if (sourceTerrain->getBuilding() != nullptr)
            {
                spBuilding pBuilding = MemoryManagement::create<Building>(pCurrentBuilding->getBuildingID(), this);
                pBuilding->setOwner(pCurrentBuilding->getOwner());
                targetTerrain->setBuilding(pBuilding);
            }

            spUnit pCurrentUnit = sourceTerrain->getSpUnit();
            if (pCurrentUnit.get() != nullptr)
            {
                spUnit pUnit = MemoryManagement::create<Unit>(pCurrentUnit->getUnitID(), pCurrentUnit->getOwner(), false, this);
                targetTerrain->setUnit(pUnit);
                pUnit->setAiMode(pCurrentUnit->getAiMode());
            }
        }
    }
    // lower right quarter gets rotated to upper right
    qint32 startY = GlobalUtils::roundUp(static_cast<float>(currentHeigth) / 2.0f);
    qint32 startX = GlobalUtils::roundUp(static_cast<float>(currentWidth) / 2.0f);
    for (qint32 y = startY; y < currentHeigth; y++)
    {
        for (qint32 x = startX; x < currentWidth; x++)
        {
            spTerrain sourceTerrain = m_fields[y][HexGrid::wrapX(x)];
            qint32 targetX = currentWidth - 1 - (x - startX);
            qint32 targetY = currentHeigth - y - 1;
            spTerrain targetTerrain = m_fields[targetY][HexGrid::wrapX(targetX)];
            targetTerrain->detachAndRemove();
            targetTerrain = Terrain::createTerrain(sourceTerrain->getTerrainID(), targetX, targetY, sourceTerrain->getBaseTerrainID(), this);
            m_rowSprites[targetY]->addChild(targetTerrain);
            m_fields[targetY][HexGrid::wrapX(targetX)] = targetTerrain;
            targetTerrain->setPosition(HexGrid::fieldToScreen(targetX, targetY, m_imagesize));

            spBuilding pCurrentBuilding = sourceTerrain->getSpBuilding();
            if (sourceTerrain->getBuilding() != nullptr)
            {
                spBuilding pBuilding = MemoryManagement::create<Building>(pCurrentBuilding->getBuildingID(), this);
                pBuilding->setOwner(pCurrentBuilding->getOwner());
                targetTerrain->setBuilding(pBuilding);
            }

            spUnit pCurrentUnit = sourceTerrain->getSpUnit();
            if (pCurrentUnit.get() != nullptr)
            {
                spUnit pUnit = MemoryManagement::create<Unit>(pCurrentUnit->getUnitID(), pCurrentUnit->getOwner(), false, this);
                targetTerrain->setUnit(pUnit);
                pUnit->setAiMode(pCurrentUnit->getAiMode());
            }
        }
    }

    updateSprites(-1, -1, true);
    centerMap(getMapWidth() / 2, getMapHeight() / 2);
    Mainapp::getInstance()->continueRendering();
}

void GameMap::flipY()
{
    Mainapp::getInstance()->pauseRendering();
    qint32 currentHeigth = getMapHeight();
    qint32 currentWidth = getMapWidth();
    for (qint32 y = currentHeigth / 2; y < currentHeigth; y++)
    {
        for (qint32 x = 0; x < currentWidth; x++)
        {
            spTerrain currentTerrain = m_fields[y][HexGrid::wrapX(x)];
            currentTerrain->detachAndRemove();
            spTerrain flipTerrain = m_fields[currentHeigth - y - 1][HexGrid::wrapX(x)];
            spTerrain pTerrain = Terrain::createTerrain(flipTerrain->getTerrainID(), x, y, flipTerrain->getBaseTerrainID(), this);
            m_rowSprites[y]->addChild(pTerrain);
            m_fields[y][HexGrid::wrapX(x)] = pTerrain;
            pTerrain->setPosition(HexGrid::fieldToScreen(x, y, m_imagesize));

            spBuilding pCurrentBuilding = flipTerrain->getSpBuilding();
            if (flipTerrain->getBuilding() != nullptr)
            {
                spBuilding pBuilding = MemoryManagement::create<Building>(pCurrentBuilding->getBuildingID(), this);
                pBuilding->setOwner(pCurrentBuilding->getOwner());
                pTerrain->setBuilding(pBuilding);
            }

            spUnit pCurrentUnit = flipTerrain->getSpUnit();
            if (pCurrentUnit.get() != nullptr)
            {
                spUnit pUnit = MemoryManagement::create<Unit>(pCurrentUnit->getUnitID(), pCurrentUnit->getOwner(), false, this);
                pTerrain->setUnit(pUnit);
                pUnit->setAiMode(pCurrentUnit->getAiMode());
            }
        }
    }
    updateSprites(-1, -1, true);
    centerMap(getMapWidth() / 2, getMapHeight() / 2);
    Mainapp::getInstance()->continueRendering();
}

void GameMap::rotateY()
{
    Mainapp::getInstance()->pauseRendering();
    qint32 currentHeigth = getMapHeight();
    qint32 currentWidth = getMapWidth();
    for (qint32 y = currentHeigth / 2; y < currentHeigth; y++)
    {
        for (qint32 x = 0; x < currentWidth; x++)
        {
            spTerrain currentTerrain = m_fields[y][HexGrid::wrapX(x)];
            currentTerrain->detachAndRemove();
            spTerrain flipTerrain = m_fields[currentHeigth - y - 1][HexGrid::wrapX(currentWidth - x - 1)];
            spTerrain pTerrain = Terrain::createTerrain(flipTerrain->getTerrainID(), x, y, flipTerrain->getBaseTerrainID(), this);
            m_rowSprites[y]->addChild(pTerrain);
            m_fields[y][HexGrid::wrapX(x)] = pTerrain;
            pTerrain->setPosition(HexGrid::fieldToScreen(x, y, m_imagesize));

            spBuilding pCurrentBuilding = flipTerrain->getSpBuilding();
            if (flipTerrain->getBuilding() != nullptr)
            {
                spBuilding pBuilding = MemoryManagement::create<Building>(pCurrentBuilding->getBuildingID(), this);
                pBuilding->setOwner(pCurrentBuilding->getOwner());
                pTerrain->setBuilding(pBuilding);
            }

            spUnit pCurrentUnit = flipTerrain->getSpUnit();
            if (pCurrentUnit.get() != nullptr)
            {
                spUnit pUnit = MemoryManagement::create<Unit>(pCurrentUnit->getUnitID(), pCurrentUnit->getOwner(), false, this);
                pTerrain->setUnit(pUnit);
                pUnit->setAiMode(pCurrentUnit->getAiMode());
            }
        }
    }
    updateSprites(-1, -1, true);
    centerMap(getMapWidth() / 2, getMapHeight() / 2);
    Mainapp::getInstance()->continueRendering();
}

void GameMap::rotate90()
{
    Mainapp::getInstance()->pauseRendering();
    qint32 currentHeigth = getMapHeight();
    qint32 currentWidth = getMapWidth();
    // upper left quarter gets rotated to upper right
    for (qint32 y = 0; y < currentHeigth / 2; y++)
    {
        for (qint32 x = 0; x < currentWidth / 2; x++)
        {
            spTerrain sourceTerrain = m_fields[y][HexGrid::wrapX(x)];
            qint32 targetX = currentWidth - y - 1;
            qint32 targetY = x;
            if (onMap(x, y))
            {
                spTerrain targetTerrain = m_fields[targetY][HexGrid::wrapX(targetX)];
                targetTerrain->detachAndRemove();
                targetTerrain = Terrain::createTerrain(sourceTerrain->getTerrainID(), targetX, targetY, sourceTerrain->getBaseTerrainID(), this);
                m_rowSprites[targetY]->addChild(targetTerrain);
                m_fields[targetY][HexGrid::wrapX(targetX)] = targetTerrain;
                targetTerrain->setPosition(HexGrid::fieldToScreen(targetX, targetY, m_imagesize));

                spBuilding pCurrentBuilding = sourceTerrain->getSpBuilding();
                if (sourceTerrain->getBuilding() != nullptr)
                {
                    spBuilding pBuilding = MemoryManagement::create<Building>(pCurrentBuilding->getBuildingID(), this);
                    pBuilding->setOwner(pCurrentBuilding->getOwner());
                    targetTerrain->setBuilding(pBuilding);
                }

                spUnit pCurrentUnit = sourceTerrain->getSpUnit();
                if (pCurrentUnit.get() != nullptr)
                {
                    spUnit pUnit = MemoryManagement::create<Unit>(pCurrentUnit->getUnitID(), pCurrentUnit->getOwner(), false, this);
                    targetTerrain->setUnit(pUnit);
                    pUnit->setAiMode(pCurrentUnit->getAiMode());
                }
            }
        }
    }

    // upper left quarter gets rotated to lower left
    for (qint32 y = 0; y < currentHeigth / 2; y++)
    {
        for (qint32 x = 0; x < currentWidth / 2; x++)
        {
            spTerrain sourceTerrain = m_fields[y][HexGrid::wrapX(x)];
            qint32 targetX = y;
            qint32 targetY = currentHeigth - x - 1;
            if (onMap(x, y))
            {
                spTerrain targetTerrain = m_fields[targetY][HexGrid::wrapX(targetX)];
                targetTerrain->detachAndRemove();
                targetTerrain = Terrain::createTerrain(sourceTerrain->getTerrainID(), targetX, targetY, sourceTerrain->getBaseTerrainID(), this);
                m_rowSprites[targetY]->addChild(targetTerrain);
                m_fields[targetY][HexGrid::wrapX(targetX)] = targetTerrain;
                targetTerrain->setPosition(HexGrid::fieldToScreen(targetX, targetY, m_imagesize));

                spBuilding pCurrentBuilding = sourceTerrain->getSpBuilding();
                if (sourceTerrain->getBuilding() != nullptr)
                {
                    spBuilding pBuilding = MemoryManagement::create<Building>(pCurrentBuilding->getBuildingID(), this);
                    pBuilding->setOwner(pCurrentBuilding->getOwner());
                    targetTerrain->setBuilding(pBuilding);
                }

                spUnit pCurrentUnit = sourceTerrain->getSpUnit();
                if (pCurrentUnit.get() != nullptr)
                {
                    spUnit pUnit = MemoryManagement::create<Unit>(pCurrentUnit->getUnitID(), pCurrentUnit->getOwner(), false, this);
                    targetTerrain->setUnit(pUnit);
                    pUnit->setAiMode(pCurrentUnit->getAiMode());
                }
            }
        }
    }

    // upper left quarter gets rotated to lower right
    for (qint32 y = 0; y < currentHeigth / 2; y++)
    {
        for (qint32 x = 0; x < currentWidth / 2; x++)
        {
            spTerrain sourceTerrain = m_fields[y][HexGrid::wrapX(x)];
            qint32 targetX = currentWidth - x - 1;
            qint32 targetY = currentHeigth - y - 1;
            if (onMap(x, y))
            {
                spTerrain targetTerrain = m_fields[targetY][HexGrid::wrapX(targetX)];
                targetTerrain->detachAndRemove();
                targetTerrain = Terrain::createTerrain(sourceTerrain->getTerrainID(), targetX, targetY, sourceTerrain->getBaseTerrainID(), this);
                m_rowSprites[targetY]->addChild(targetTerrain);
                m_fields[targetY][HexGrid::wrapX(targetX)] = targetTerrain;
                targetTerrain->setPosition(HexGrid::fieldToScreen(targetX, targetY, m_imagesize));

                spBuilding pCurrentBuilding = sourceTerrain->getSpBuilding();
                if (sourceTerrain->getBuilding() != nullptr)
                {
                    spBuilding pBuilding = MemoryManagement::create<Building>(pCurrentBuilding->getBuildingID(), this);
                    pBuilding->setOwner(pCurrentBuilding->getOwner());
                    targetTerrain->setBuilding(pBuilding);
                }

                spUnit pCurrentUnit = sourceTerrain->getSpUnit();
                if (pCurrentUnit.get() != nullptr)
                {
                    spUnit pUnit = MemoryManagement::create<Unit>(pCurrentUnit->getUnitID(), pCurrentUnit->getOwner(), false, this);
                    targetTerrain->setUnit(pUnit);
                    pUnit->setAiMode(pCurrentUnit->getAiMode());
                }
            }
        }
    }


    updateSprites(-1, -1, true);
    centerMap(getMapWidth() / 2, getMapHeight() / 2);
    Mainapp::getInstance()->continueRendering();
}

namespace
{
    /**
     * @brief importHexMap creates the binary map next to the given text map (see hex_balance/HEX_MAP_FORMAT.md)
     * if the binary map doesn't exist or is older than the text map.
     */
    void importHexMap(const QString & jsonPath)
    {
        static const QString suffix = ".hexmap.json";
        const QString filename = jsonPath.left(jsonPath.size() - suffix.size()) + ".map";
        const QFileInfo jsonInfo(jsonPath);
        const QFileInfo mapInfo(filename);
        if (mapInfo.exists() && mapInfo.lastModified() >= jsonInfo.lastModified())
        {
            return;
        }
        QFile jsonFile(jsonPath);
        if (!jsonFile.open(QIODevice::ReadOnly))
        {
            CONSOLE_PRINT("Hex map import: failed to open " + jsonPath, GameConsole::eERROR);
            return;
        }
        QJsonParseError parseError;
        const QJsonObject root = QJsonDocument::fromJson(jsonFile.readAll(), &parseError).object();
        jsonFile.close();
        const qint32 width = root.value("width").toInt();
        const qint32 height = root.value("height").toInt();
        const qint32 playerCount = root.value("players").toInt();
        const QJsonArray rows = root.value("terrain").toArray();
        if (parseError.error != QJsonParseError::NoError || width <= 0 || height <= 0 || playerCount < 2 || rows.size() != height)
        {
            CONSOLE_PRINT("Hex map import: " + jsonPath + " is invalid. " + parseError.errorString() +
                          " The map needs width, height, at least 2 players and one terrain row per height.", GameConsole::eERROR);
            return;
        }
        CONSOLE_PRINT("Hex map import: creating " + filename, GameConsole::eINFO);
        // symbols of the terrain rows
        struct Symbol
        {
            QString terrain;
            QString building;
            qint32 owner{0};
        };
        QHash<QString, Symbol> legend = {{".", {GameMap::PLAINS, "", 0}}, {"F", {"FOREST", "", 0}}, {"M", {"MOUNTAIN", "", 0}},
                                         {"~", {"RIVER", "", 0}}, {"=", {"STREET", "", 0}}, {"B", {"BRIDGE1", "", 0}},
                                         {"S", {"SEA", "", 0}}, {"T", {GameMap::PLAINS, "TOWN", 0}},
                                         {"X", {GameMap::VOID_TERRAIN, "", 0}}};
        const QJsonObject jsonLegend = root.value("legend").toObject();
        for (auto iter = jsonLegend.constBegin(); iter != jsonLegend.constEnd(); ++iter)
        {
            Symbol symbol;
            if (iter.value().isObject())
            {
                const QJsonObject data = iter.value().toObject();
                symbol.terrain = data.value("terrain").toString(GameMap::PLAINS);
                symbol.building = data.value("building").toString();
                symbol.owner = data.value("owner").toInt();
            }
            else
            {
                symbol.terrain = iter.value().toString();
            }
            legend.insert(iter.key(), symbol);
        }
        spGameMap pMap = MemoryManagement::create<GameMap>(width, height, playerCount);
        // "layout": "rectangle" maps are written like maps of the square grid: all rows start at the same column.
        // They are moved into the hex layout when they get loaded, so their rows alternate left and right.
        // All other maps describe the hex layout directly, where each row starts half a field further right.
        // "wrap": true maps wrap around in east west direction. Their rows are written as rectangle like the ones of
        // the rectangle layout. Row y is stored turned by y / 2 columns, so all fields have the same neighbour offsets.
        const bool wrap = root.value("wrap").toBool();
        if (wrap)
        {
            pMap->getVariables()->createVariable(GameMap::HEX_LAYOUT_VARIABLE);
            pMap->getVariables()->createVariable(GameMap::HEX_WRAP_VARIABLE);
        }
        else if (root.value("layout").toString() != "rectangle")
        {
            pMap->getVariables()->createVariable(GameMap::HEX_LAYOUT_VARIABLE);
        }
        auto storageX = [wrap, width](qint32 x, qint32 y)
        {
            if (wrap && y >= 0)
            {
                x = (x - y / 2) % width;
                if (x < 0)
                {
                    x += width;
                }
            }
            return x;
        };
        auto placeBuilding = [&pMap, playerCount, &jsonPath, &storageX](const QString & id, qint32 x, qint32 y, qint32 owner)
        {
            x = storageX(x, y);
            if (!pMap->onMap(x, y) || owner < 0 || owner > playerCount)
            {
                CONSOLE_PRINT("Hex map import: " + jsonPath + " building " + id + " has an invalid position or owner", GameConsole::eERROR);
                return;
            }
            spBuilding pBuilding = MemoryManagement::create<Building>(id, pMap.get());
            if (owner > 0)
            {
                pBuilding->setOwner(pMap->getPlayer(owner - 1));
            }
            pMap->getTerrain(x, y)->setBuilding(pBuilding);
        };
        for (qint32 y = 0; y < height; ++y)
        {
            // whitespaces are only used to show the hex layout in the text
            const QStringList symbols = rows[y].toString().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (symbols.size() != width)
            {
                CONSOLE_PRINT("Hex map import: " + jsonPath + " terrain row " + QString::number(y) + " has " +
                              QString::number(symbols.size()) + " fields instead of " + QString::number(width), GameConsole::eERROR);
            }
            for (qint32 x = 0; x < width && x < symbols.size(); ++x)
            {
                const auto iter = legend.constFind(symbols[x]);
                if (iter == legend.constEnd())
                {
                    CONSOLE_PRINT("Hex map import: " + jsonPath + " unknown terrain symbol " + symbols[x], GameConsole::eERROR);
                    continue;
                }
                if (iter->terrain != GameMap::PLAINS)
                {
                    pMap->replaceTerrain(iter->terrain, storageX(x, y), y, false, false, false);
                }
                if (!iter->building.isEmpty())
                {
                    placeBuilding(iter->building, x, y, iter->owner);
                }
            }
        }
        const QJsonArray buildings = root.value("buildings").toArray();
        for (const auto & item : buildings)
        {
            const QJsonObject data = item.toObject();
            placeBuilding(data.value("id").toString(), data.value("x").toInt(-1), data.value("y").toInt(-1), data.value("owner").toInt());
        }
        const QJsonArray units = root.value("units").toArray();
        for (const auto & item : units)
        {
            const QJsonObject data = item.toObject();
            const qint32 y = data.value("y").toInt(-1);
            const qint32 x = storageX(data.value("x").toInt(-1), y);
            const qint32 owner = data.value("owner").toInt();
            if (!pMap->onMap(x, y) || owner < 1 || owner > playerCount)
            {
                CONSOLE_PRINT("Hex map import: " + jsonPath + " unit " + data.value("id").toString() + " has an invalid position or owner", GameConsole::eERROR);
                continue;
            }
            spUnit pUnit = MemoryManagement::create<Unit>(data.value("id").toString(), pMap->getPlayer(owner - 1), false, pMap.get());
            if (data.contains("hp"))
            {
                pUnit->setHp(data.value("hp").toInt(10));
            }
            pMap->getTerrain(x, y)->setUnit(pUnit);
        }
        pMap->setMapName(root.value("name").toString(jsonInfo.fileName().chopped(suffix.size())));
        pMap->setMapDescription(root.value("description").toString());
        if (root.contains("author"))
        {
            pMap->setMapAuthor(root.value("author").toString());
        }
        pMap->updateSprites();

        // written to a temporary file first, cause several game instances may import the map at the same time
        QSaveFile file(filename);
        if (!file.open(QIODevice::WriteOnly))
        {
            CONSOLE_PRINT("Hex map import: failed to open " + filename, GameConsole::eERROR);
            return;
        }
        QDataStream stream(&file);
        stream.setVersion(QDataStream::Version::Qt_6_5);
        pMap->serializeObject(stream);
        file.commit();
    }
}

void GameMap::importHexMaps()
{
    QStringList roots;
    for (const QString & root : {Settings::getInstance()->getUserPath() + "maps/", QString("maps/")})
    {
        const QString path = QDir(root).absolutePath();
        if (!roots.contains(path) && QDir(path).exists())
        {
            roots.append(path);
        }
    }
    for (const auto & root : std::as_const(roots))
    {
        QDirIterator iter(root, {"*.hexmap.json"}, QDir::Files, QDirIterator::Subdirectories);
        while (iter.hasNext())
        {
            importHexMap(iter.next());
        }
    }
}
