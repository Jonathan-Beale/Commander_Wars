#include "resource_management/movementtablemanager.h"
#include "game/actionperformer.h"
#include "coreengine/perftrace.h"
#include "game/gameanimation/gameanimationfactory.h"
#include "ai/coreai.h"
#include "ai/dummyai.h"
#include "ai/trainingdatagenerator.h"

#include "coreengine/mainapp.h"

#include "menue/gamemenue.h"

#include "objects/dialogs/dialogconnecting.h"

#include "resource_management/fontmanager.h"

#include "3rd_party/oxygine-framework/oxygine/actor/Stage.h"

ActionPerformer::ActionPerformer(GameMap* pMap, GameMenue* pMenu)
    : m_pMenu(pMenu),
      m_pMap(pMap)
{
    Interpreter::setCppOwnerShip(this);
    m_delayedActionPerformedTimer.setSingleShot(true);
    connect(&m_delayedActionPerformedTimer, &QTimer::timeout, this, &ActionPerformer::delayedActionPerformed, Qt::QueuedConnection);
    connect(this, &ActionPerformer::sigPerformAction, this, &ActionPerformer::performAction, Qt::QueuedConnection);
    connect(this, &ActionPerformer::sigActionPerformed, this, &ActionPerformer::simultaneousOnActionPerformed, Qt::QueuedConnection);
}

bool ActionPerformer::getActionRunning() const
{
    return m_actionRunning;
}

void ActionPerformer::setSyncCounter(qint64 counter)
{
    m_syncCounter = counter;
}

bool ActionPerformer::requiresMapHash() const
{
    // other computers and the ai process run their own copy of the match which needs to be checked.
    return (m_pMenu != nullptr && m_pMenu->getNetworkInterface() != nullptr) ||
           Settings::getInstance()->getSpawnAiProcess() ||
           Settings::getInstance()->getAiSlave();
}

void ActionPerformer::performAction(spGameAction pGameAction, bool fromAiPipe)
{
    if (qEnvironmentVariableIsSet("COW_CLEAR_MOVE_COSTS_EACH_ACTION"))
    {
        MovementTableManager::clearBaseMovementPointsCache();
    }
    MovementTableManager::movementStateChanged();
    CONSOLE_PRINT("Start running action " + pGameAction->getActionID(), GameConsole::eDEBUG);
    if (simultaneousIntercept(pGameAction))
    {
        return;
    }
    PerfTrace::mark("act: perform " + pGameAction->getActionID());
    // the hash stores the whole map. It's only needed to detect matches running out of sync.
    QByteArray mapHash;
    if (requiresMapHash())
    {
        mapHash = m_pMap->getMapHash();
    }
    PerfTrace::mark("act: hashed");
    if (m_exit)
    {
        if (m_pMenu != nullptr)
        {
            emit m_pMenu->sigVictory(-1);
        }
        return;
    }
    else if (m_actionRunning)
    {
        CONSOLE_PRINT("Ignoring action request cause an action is currently performed", GameConsole::eWARNING);
        return;
    }
    else if (m_mapHash.length() > 0 && m_mapHash != mapHash)
    {
        CONSOLE_PRINT("An interaction changed the map state outside performing actions.", GameConsole::eERROR);
    }
    m_actionRunning = true;
    bool autosave = true;
    if (m_pMenu != nullptr)
    {
        m_pMenu->setSaveAllowed(false);
    }
    Mainapp::getInstance()->pauseRendering();
    if (m_multiplayerSyncData.m_waitingForSyncFinished && m_pMenu != nullptr)
    {
        // a parked action must release the gate for the resubmit
        m_actionRunning = false;
        m_pMenu->setSaveAllowed(true);
        if (m_multiplayerSyncData.m_postSyncAction.get() != nullptr)
        {
            CONSOLE_PRINT("Replacing an already parked action while waiting for the join sync.", GameConsole::eWARNING);
        }
        m_multiplayerSyncData.m_postSyncAction = pGameAction;
        spDialogConnecting pDialogConnecting = MemoryManagement::create<DialogConnecting>(tr("Waiting for Players/Observers to join..."), 1000 * 60 * 5);
        m_pMenu->addChild(pDialogConnecting);
        connect(pDialogConnecting.get(), &DialogConnecting::sigCancel, m_pMenu, &GameMenue::exitGame, Qt::QueuedConnection);
        connect(m_pMenu, &GameMenue::sigSyncFinished, pDialogConnecting.get(), &DialogConnecting::connected, Qt::QueuedConnection);
    }
    else if (pGameAction.get() != nullptr)
    {
        Player* pCurrentPlayer = m_pMap->getCurrentPlayer();
        auto* baseGameInput = pCurrentPlayer->getBaseGameInput();
        bool proxyAi = baseGameInput != nullptr && (baseGameInput->getAiType() == GameEnums::AiTypes_ProxyAi || fromAiPipe || dynamic_cast<DummyAi*>(baseGameInput) != nullptr);
        CONSOLE_PRINT("GameMenue::performAction " + pGameAction->getActionID() + " at X: " + QString::number(pGameAction->getTarget().x())
                      + " at Y: " + QString::number(pGameAction->getTarget().y()) +
                      " is proxy ai " + QString::number(proxyAi), GameConsole::eDEBUG);
        bool multiplayer = false;
        if (m_pMenu != nullptr)
        {
            multiplayer = m_pMenu->getIsMultiplayer(pGameAction);
        }
        bool asyncMatch =  false;
        bool invalidHash = false;
        if (proxyAi)
        {
            CONSOLE_PRINT("Validating game state", GameConsole::eDEBUG);
            asyncMatch = m_syncCounter + 1 != pGameAction->getSyncCounter();
            invalidHash = pGameAction->getMapHash() != mapHash;
        }
        const bool abortedByAsync = (asyncMatch || invalidHash) && multiplayer;
        if (asyncMatch || invalidHash)
        {
            if (asyncMatch)
            {
                CONSOLE_PRINT("Async found cause action sync counter " + QString::number(pGameAction->getSyncCounter()) + " doesn't match map sync counter " + QString::number(m_syncCounter + 1), GameConsole::eDEBUG);
            }
            else
            {
                CONSOLE_PRINT("Async found cause map hash is different from action map hash", GameConsole::eDEBUG);
            }
            if (multiplayer &&
                m_pMenu != nullptr)
            {
                autosave = false;
                m_pMenu->doResyncGame();
            }
        }
        if (!abortedByAsync)
        {
            // perform action
            GlobalUtils::seed(pGameAction->getSeed());
            GlobalUtils::setUseSeed(true);
            if (proxyAi)
            {
                m_syncCounter = pGameAction->getSyncCounter();
            }
            else if (!pGameAction->getIsLocal())
            {
                ++m_syncCounter;
            }
            CONSOLE_PRINT("Updated sync counter to " + QString::number(m_syncCounter), GameConsole::eDEBUG);
            m_pStoredAction.reset();
            m_pMap->getGameRules()->pauseRoundTime();
            if (!pGameAction->getIsLocal() &&
                baseGameInput != nullptr &&
                !Settings::getInstance()->getAiSlave() &&
                baseGameInput->getAiType() != GameEnums::AiTypes_ProxyAi)
            {
                pGameAction = doMultiTurnMovement(pGameAction);
            }
            Unit * pMoveUnit = pGameAction->getTargetUnit();
            doTrapping(pGameAction);
            // send action to other players if needed
            pGameAction->setSyncCounter(m_syncCounter);
            pGameAction->setMapHash(mapHash);
            pGameAction->setRoundTimerTime(m_pMap->getGameRules()->getRoundTimer()->remainingTime());
            if (!pGameAction->getIsLocal() &&
                !fromAiPipe &&
                m_pMenu != nullptr &&
                m_pMenu->getGameStarted())
            {
                CONSOLE_PRINT("Sending action to ai pipe", GameConsole::eDEBUG);
                emit sigAiProcesseSendAction(pGameAction);
            }
            if (requiresForwarding(pGameAction))
            {
                CONSOLE_PRINT("Sending action to other players with sync counter " + QString::number(m_syncCounter), GameConsole::eDEBUG);
                QByteArray data;
                QDataStream stream(&data, QIODevice::WriteOnly);
                stream.setVersion(QDataStream::Version::Qt_6_5);
                stream << m_pMap->getCurrentPlayer()->getPlayerID();
                pGameAction->serializeObject(stream);
                emit m_pMenu->getNetworkInterface()->sig_sendData(0, data, NetworkInterface::NetworkSerives::Game, true);
            }
            else if (multiplayer)
            {
                m_pMap->getGameRules()->getRoundTimer()->setInterval(pGameAction->getRoundTimerTime());
            }

            // record action if required
            if (m_pMenu != nullptr)
            {
                m_pMenu->getReplayRecorder().recordAction(pGameAction);
            }
            if (pMoveUnit != nullptr)
            {
                pMoveUnit->setMultiTurnPath(pGameAction->getMultiTurnPath());
            }

            if (baseGameInput != nullptr)
            {
                baseGameInput->centerCameraOnAction(pGameAction.get());
            }
            PerfTrace::mark("act: run " + pGameAction->getActionID());
            pGameAction->perform();
            PerfTrace::mark("act: ran");
            // clean up the action
            m_pCurrentAction = pGameAction;
            pGameAction.reset();
            skipAnimations(false);
            PerfTrace::mark("act: animations skipped");
        }
        else
        {
            // a dropped action must still release the gate
            m_actionRunning = false;
            if (m_pMenu != nullptr)
            {
                m_pMenu->setSaveAllowed(true);
            }
        }
        if (pCurrentPlayer != m_pMap->getCurrentPlayer() &&
            m_pMenu != nullptr &&
            autosave)
        {
            auto* baseGameInput = m_pMap->getCurrentPlayer()->getBaseGameInput();
            if (baseGameInput != nullptr &&
                baseGameInput->getAiType() == GameEnums::AiTypes_Human)
            {
                m_pMenu->autoSaveMap();
            }
        }
    }
    Mainapp::getInstance()->continueRendering();
}

bool ActionPerformer::requiresForwarding(const spGameAction & pGameAction) const
{
    Player* pCurrentPlayer = m_pMap->getCurrentPlayer();
    auto* baseGameInput = pCurrentPlayer->getBaseGameInput();
    return m_pMenu != nullptr &&
           m_pMenu->getIsMultiplayer(pGameAction) &&
           baseGameInput != nullptr &&
           baseGameInput->getAiType() != GameEnums::AiTypes_ProxyAi &&
                                         !pGameAction->getIsLocal();
}

void ActionPerformer::setActionRunning(bool newActionRunning)
{
    m_actionRunning = newActionRunning;
}

bool ActionPerformer::getExit() const
{
    return m_exit;
}

void ActionPerformer::setExit(bool newExit)
{
    m_exit = newExit;
}

spGameAction ActionPerformer::doMultiTurnMovement(spGameAction pGameAction)
{
    if (m_pMenu != nullptr &&
        m_pMenu->getGameStarted() &&
        !Settings::getInstance()->getAiSlave() &&
        pGameAction.get() != nullptr &&
        (pGameAction->getActionID() == CoreAI::ACTION_NEXT_PLAYER ||
         pGameAction->getActionID() == CoreAI::ACTION_SWAP_COS))
    {
        CONSOLE_PRINT("Check and update multiTurnMovement" + pGameAction->getActionID(), GameConsole::eDEBUG);

        // check for units that have a multi turn available
        qint32 heigth = m_pMap->getMapHeight();
        qint32 width = m_pMap->getMapWidth();
        Player* pPlayer = m_pMap->getCurrentPlayer();
        for (qint32 y = 0; y < heigth; y++)
        {
            for (qint32 x = 0; x < width; x++)
            {
                Unit* pUnit = m_pMap->getTerrain(x, y)->getUnit();
                if (pUnit != nullptr)
                {
                    if ((pUnit->getOwner() == pPlayer) &&
                        (pUnit->getHasMoved() == false))
                    {
                        QVector<QPoint> currentMultiTurnPath = pUnit->getMultiTurnPath();
                        if (currentMultiTurnPath.size() > 0)
                        {
                            qint32 movepoints = pUnit->getMovementpoints(pUnit->getPosition());
                            if (movepoints > 0)
                            {
                                // shorten path
                                UnitPathFindingSystem pfs(m_pMap, pUnit, pPlayer);
                                pfs.setMovepoints(pUnit->getFuel());
                                pfs.explore();
                                auto newPath = pfs.getClosestReachableMovePath(currentMultiTurnPath, movepoints);
                                if (newPath.size() > 0)
                                {
                                    CONSOLE_PRINT("Replacing action with multiTurnMovement action", GameConsole::eDEBUG);
                                    // replace current action with auto moving none moved units
                                    m_pStoredAction = pGameAction;
                                    spGameAction multiTurnMovement = MemoryManagement::create<GameAction>(CoreAI::ACTION_WAIT, m_pMap);
                                    if (pUnit->getActionList().contains(CoreAI::ACTION_HOELLIUM_WAIT))
                                    {
                                        multiTurnMovement->setActionID(CoreAI::ACTION_HOELLIUM_WAIT);
                                    }
                                    multiTurnMovement->setTarget(pUnit->getPosition());
                                    multiTurnMovement->setMovepath(newPath, pfs.getCosts(newPath));
                                    QVector<QPoint> multiTurnPath;
                                    // still some path ahead?
                                    multiTurnPath = QVector<QPoint>();
                                    if (currentMultiTurnPath.size() > newPath.size())
                                    {
                                        for (qint32 i = 0; i <= currentMultiTurnPath.size() - newPath.size(); i++)
                                        {
                                            multiTurnPath.append(currentMultiTurnPath[i]);
                                        }
                                    }
                                    multiTurnMovement->setMultiTurnPath(multiTurnPath);
                                    return multiTurnMovement;
                                }
                            }
                        }
                        else if (pUnit->getActionList().contains(CoreAI::ACTION_CAPTURE))
                        {
                            spGameAction multiTurnMovement = MemoryManagement::create<GameAction>(CoreAI::ACTION_CAPTURE, m_pMap);
                            multiTurnMovement->setTarget(pUnit->getPosition());
                            if (multiTurnMovement->canBePerformed())
                            {
                                CONSOLE_PRINT("Replacing action with capture action", GameConsole::eDEBUG);
                                m_pStoredAction = pGameAction;
                                return multiTurnMovement;
                            }
                        }
                    }
                }
            }
        }
    }
    return pGameAction;
}

void ActionPerformer::centerMapOnAction(GameAction* pGameAction)
{
    if (m_pMenu != nullptr)
    {
        CONSOLE_PRINT("centerMapOnAction()", GameConsole::eDEBUG);
        Unit* pUnit = pGameAction->getTargetUnit();
        Player* pPlayer = m_pMenu->getCurrentViewPlayer();

        QPoint target = pGameAction->getTarget();
        if (pUnit != nullptr &&
            !pUnit->isStealthed(pPlayer))
        {
            const auto & path = pGameAction->getMovePath();
            for (const auto & point : path)
            {
                if (pPlayer->getFieldVisible(point.x(), point.y()))
                {
                    target = point;
                    break;
                }
            }
        }

        if (m_pMap->onMap(target.x(), target.y()) &&
            pPlayer->getFieldVisible(target.x(), target.y()) &&
            (pUnit == nullptr ||
             !pUnit->isStealthed(pPlayer)))
        {
            m_pMap->centerMap(target.x(), target.y());
        }
    }
}

void ActionPerformer::skipAnimations(bool postAnimation)
{
    CONSOLE_PRINT("skipping Animations", GameConsole::eDEBUG);
    Mainapp::getInstance()->pauseRendering();
    if (GameAnimationFactory::getAnimationCount() > 0)
    {
        GameAnimationFactory::skipAllAnimations();
    }
    if (GameAnimationFactory::getAnimationCount() == 0 && !postAnimation)
    {
        CONSOLE_PRINT("GameMenue -> emitting animationsFinished()", GameConsole::eDEBUG);
        emit GameAnimationFactory::getInstance()->animationsFinished();
    }
    Mainapp::getInstance()->continueRendering();
}

void ActionPerformer::finishActionPerformed()
{
    CONSOLE_PRINT("Doing post action update", GameConsole::eDEBUG);
    m_finishedPerformed = true;
    if (m_pCurrentAction.get() != nullptr)
    {
        Unit* pUnit = m_pCurrentAction->getMovementTarget();
        if (pUnit != nullptr)
        {
            pUnit->postAction(m_pCurrentAction.get());
        }
        m_pMap->getCurrentPlayer()->postAction(m_pCurrentAction.get());
        m_pMap->getGameScript()->actionDone(m_pCurrentAction);
    }
    skipAnimations(true);
    if (m_pMenu != nullptr)
    {
        m_pMenu->updateQuickButtons();
    }
}

void ActionPerformer::actionPerformed()
{
    PerfTrace::mark("act: performed slot");
    Mainapp::getInstance()->pauseRendering();
    if (m_pMenu != nullptr &&
        m_pMap != nullptr)
    {
        if (GameAnimationFactory::getAnimationCount() == 0)
        {
            CONSOLE_PRINT("Action performed", GameConsole::eDEBUG);
            PerfTrace::mark("act: post action");
            if (!m_finishedPerformed)
            {
                finishActionPerformed();
            }
            PerfTrace::mark("act: sync animations");
            if (Settings::getInstance()->getSyncAnimations())
            {
                m_pMap->syncUnitsAndBuildingAnimations();
            }
            PerfTrace::mark("act: update game info");
            if (m_pMenu != nullptr)
            {
                m_pMenu->updateGameInfo();
            }
            if (GameAnimationFactory::getAnimationCount() == 0)
            {
                CONSOLE_PRINT("Action finished", GameConsole::eDEBUG);
                // the action is done: results of movement hooks which were stored while it ran are outdated
                MovementTableManager::movementStateChanged();
                PerfTrace::mark("act: finish kill units");
                m_pMap->killDeadUnits();
                PerfTrace::mark("act: finish check victory");
                m_pMap->getGameRules()->checkVictory();
                PerfTrace::mark("act: finish fog vision");
                m_pMap->getGameRules()->createFogVision();
                PerfTrace::mark("act: finish training data");
                m_pMap->getTrainingDataGenerator()->onActionDone(m_pCurrentAction.get());
                PerfTrace::mark("act: finish hash");
                m_pCurrentAction.reset();
                m_actionRunning = false;
                m_finishedPerformed = false;
                CONSOLE_PRINT("Storing current map hash", GameConsole::eDEBUG);
                m_mapHash = requiresMapHash() ? m_pMap->getMapHash() : QByteArray();
                PerfTrace::mark("act: finish rest");
                GlobalUtils::setUseSeed(false);
                if (m_pMenu != nullptr &&
                    m_pMenu->getIndespawningMode())
                {
                    m_pMenu->setSaveAllowed(true);
                    m_pMenu->doDespawnSlave();
                }
                else if (m_exit &&
                         m_pMenu != nullptr)
                {
                    CONSOLE_PRINT("ActionPerformer state is exiting game. Emitting exit", GameConsole::eDEBUG);
                    emit m_pMenu->sigVictory(-1);
                }
                else if (!m_pMap->getGameRules()->getVictory())
                {
                    if (!m_pMap->anyPlayerAlive() &&
                        m_pMenu != nullptr)
                    {
                        CONSOLE_PRINT("Forcing exiting the game cause no player is alive", GameConsole::eDEBUG);
                        emit m_pMenu->sigExitGame();
                    }
                    else if (m_pMap->getCurrentPlayer()->getIsDefeated())
                    {
                        onTriggeringActionFinished();
                        CONSOLE_PRINT("Triggering next player cause current player is defeated", GameConsole::eDEBUG);
                        spGameAction pAction = MemoryManagement::create<GameAction>(CoreAI::ACTION_NEXT_PLAYER, m_pMap);
                        performAction(pAction);
                    }
                    else if (m_pStoredAction.get() != nullptr)
                    {
                        onTriggeringActionFinished();
                        performAction(m_pStoredAction);
                    }
                    else
                    {
                        if (m_pMap->getCurrentPlayer()->getBaseGameInput()->getAiType() != GameEnums::AiTypes_ProxyAi)
                        {
                            m_pMap->getGameRules()->resumeRoundTime();
                        }
                        if (m_noTimeOut &&
                            !Settings::getInstance()->getAiSlave())
                        {
                            onTriggeringActionFinished();
                            spGameAction pAction = MemoryManagement::create<GameAction>(CoreAI::ACTION_NEXT_PLAYER, m_pMap);
                            performAction(pAction);
                            m_noTimeOut = false;
                        }
                        else
                        {
                            CONSOLE_PRINT("emitting sigActionPerformed()", GameConsole::eDEBUG);
                            quint32 delay = Settings::getInstance()->getPauseAfterAction();
                            if (delay == 0)
                            {
                                onTriggeringActionFinished();
                                PerfTrace::mark("act: done");
                                emit sigActionPerformed();
                            }
                            else
                            {
                                m_delayedActionPerformedTimer.start(std::chrono::seconds(delay));
                            }
                        }
                    }
                }
                else if (m_pMenu != nullptr)
                {
                    auto winner = m_pMap->getGameRules()->getVictoryTeam();
                    CONSOLE_PRINT("Game already won  retriggering victory for team " + QString::number(winner), GameConsole::eDEBUG);
                    emit m_pMenu->sigVictory(winner);
                }
            }
            else
            {
                CONSOLE_PRINT("Skipping action performed cause finishActionPerformed added animations", GameConsole::eDEBUG);
            }
        }
        else
        {
            CONSOLE_PRINT("Animation finish error. Cause following animations are still active", GameConsole::eERROR);
            GameAnimationFactory::printActiveAnimations();
        }
    }
    else
    {
        CONSOLE_PRINT("Skipping action performed", GameConsole::eDEBUG);
    }
    Mainapp::getInstance()->continueRendering();
}

void ActionPerformer::onTriggeringActionFinished()
{
    if (m_pMenu != nullptr)
    {
        CONSOLE_PRINT("ActionPerformer::onTriggeringActionFinished", GameConsole::eDEBUG);
        m_pMenu->setSaveAllowed(true);
        if (m_pMenu->getSaveMap())
        {
            m_pMenu->doSaveMap();
        }
        m_pMenu->sendOnlineInfo();
    }
}

void ActionPerformer::nextTurnPlayerTimeout()
{
    auto* input = m_pMap->getCurrentPlayer()->getBaseGameInput();
    if (input != nullptr &&
        !Settings::getInstance()->getAiSlave())
    {
        if (input->getAiType() == GameEnums::AiTypes_Human)
        {
            if (m_pCurrentAction.get() == nullptr)
            {
                spGameAction pAction = MemoryManagement::create<GameAction>(CoreAI::ACTION_NEXT_PLAYER, m_pMap);
                performAction(pAction);
            }
            else
            {
                m_noTimeOut = true;
            }
        }
        else if (input->getAiType() != GameEnums::AiTypes_Human &&
                 input->getAiType() != GameEnums::AiTypes_ProxyAi &&
                 input->getAiType() != GameEnums::AiTypes_Closed)
        {
            m_noTimeOut = true;
        }
    }
}

qint64 ActionPerformer::getSyncCounter() const
{
    return m_syncCounter;
}

void ActionPerformer::doTrapping(spGameAction & pGameAction)
{
    CONSOLE_PRINT("GameMenue::doTrapping", GameConsole::eDEBUG);
    QVector<QPoint> path = pGameAction->getMovePath();

    Unit * pMoveUnit = pGameAction->getTargetUnit();
    if (path.size() > 1 && pMoveUnit != nullptr)
    {
        if (pGameAction->getRequiresEmptyField())
        {
            QVector<QPoint> trapPathNotEmptyTarget = path;
            qint32 trapPathCostNotEmptyTarget = pGameAction->getCosts();
            QPoint trapPoint = path[0];
            for (qint32 i = 0; i < path.size() - 1; i++)
            {
                QPoint point = path[i];
                QPoint prevPoint = path[i + 1];
                Unit* pUnit = m_pMap->getTerrain(point.x(), point.y())->getUnit();
                if (pUnit == nullptr || pMoveUnit->getOwner()->isAlly(pUnit->getOwner()))
                {
                    if (i > 0)
                    {
                        spGameAction pTrapAction = MemoryManagement::create<GameAction>(CoreAI::ACTION_TRAP, m_pMap);
                        pTrapAction->setMovepath(trapPathNotEmptyTarget, trapPathCostNotEmptyTarget);
                        pTrapAction->writeDataInt32(trapPoint.x());
                        pTrapAction->writeDataInt32(trapPoint.y());
                        pTrapAction->setTarget(pGameAction->getTarget());
                        pGameAction = pTrapAction;
                    }
                    break;
                }
                else
                {
                    trapPoint = point;
                    qint32 moveCost = pMoveUnit->getMovementCosts(point.x(), point.y(),
                                                                  prevPoint.x(), prevPoint.y());
                    trapPathCostNotEmptyTarget -= moveCost;
                    trapPathNotEmptyTarget.removeFirst();
                }
            }
        }
        path = pGameAction->getMovePath();
        QVector<QPoint> trapPath;
        qint32 trapPathCost = 0;
        for (qint32 i = path.size() - 2; i >= 0; i--)
        {
            // check the movepath for a trap
            QPoint point = path[i];
            QPoint prevPoint = path[i];
            if (i > 0)
            {
                prevPoint = path[i - 1];
            }
            qint32 moveCost = pMoveUnit->getMovementCosts(point.x(), point.y(),
                                                          prevPoint.x(), prevPoint.y(), true);
            if (isTrap("isTrap", pGameAction, pMoveUnit, point, prevPoint, moveCost))
            {
                while (trapPath.size() > 1)
                {
                    QPoint currentPoint = trapPath[0];
                    QPoint previousPoint = trapPath[1];
                    moveCost = pMoveUnit->getMovementCosts(currentPoint.x(), currentPoint.y(),
                                                           previousPoint.x(), previousPoint.y());
                    if (isTrap("isStillATrap", pGameAction, pMoveUnit, currentPoint, previousPoint, moveCost))
                    {
                        trapPathCost -= moveCost;
                        trapPath.pop_front();
                        if (m_pMap->getTerrain(point.x(), point.y())->getUnit() != nullptr)
                        {
                            point = currentPoint;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                spGameAction pTrapAction = MemoryManagement::create<GameAction>(CoreAI::ACTION_TRAP, m_pMap);
                pTrapAction->setMovepath(trapPath, trapPathCost);
                pMoveUnit->getOwner()->addVisionField(point.x(), point.y(), 1, true);
                pTrapAction->writeDataInt32(point.x());
                pTrapAction->writeDataInt32(point.y());
                pTrapAction->setTarget(pGameAction->getTarget());
                pGameAction = pTrapAction;
                break;
            }
            else
            {
                trapPath.push_front(point);
                qint32 x = pMoveUnit->Unit::getX();
                qint32 y = pMoveUnit->Unit::getY();
                if (point.x() != x ||
                    point.y() != y)
                {
                    QPoint previousPoint = path[i + 1];
                    trapPathCost += pMoveUnit->getMovementCosts(point.x(), point.y(), previousPoint.x(), previousPoint.y());
                }
            }
        }
    }
}

bool ActionPerformer::isTrap(const QString & function, spGameAction pAction, Unit* pMoveUnit, QPoint currentPoint, QPoint previousPoint, qint32 moveCost)
{

    Unit* pUnit = m_pMap->getTerrain(currentPoint.x(), currentPoint.y())->getUnit();

    Interpreter* pInterpreter = Interpreter::getInstance();
    QJSValueList args({JsThis::getJsThis(pAction.get()),
                       JsThis::getJsThis(pMoveUnit),
                       JsThis::getJsThis(pUnit),
                       currentPoint.x(),
                       currentPoint.y(),
                       previousPoint.x(),
                       previousPoint.y(),
                       moveCost,
                       GameMap::getMapJsThis(m_pMap),
                      });
    const QString obj = "ACTION_TRAP";
    QJSValue erg = pInterpreter->doFunction(obj, function, args);
    if (erg.isBool())
    {
        return erg.toBool();
    }
    return false;
}

void ActionPerformer::delayedActionPerformed()
{
    if (m_pCurrentAction.get() == nullptr && !m_exit)
    {
        onTriggeringActionFinished();
        PerfTrace::mark("act: done");
        emit sigActionPerformed();
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// simultaneous turns prototype
// ---------------------------------------------------------------------------------------------------------------------

static const char* const SIMULTANEOUS_RULE = "GAMERULE_SIMULTANEOUS_TURNS";
static constexpr qint32 SIMULTANEOUS_MAX_ROUNDS_PER_DAY = 300;

bool ActionPerformer::getSimultaneousTurns() const
{
    if (m_pMap == nullptr || m_pMenu == nullptr || Settings::getInstance()->getAiSlave())
    {
        return false;
    }
    GameRule* pRule = m_pMap->getGameRules()->getGameRule(SIMULTANEOUS_RULE);
    return pRule != nullptr && pRule->getRuleValue(0) != 0;
}

bool ActionPerformer::simultaneousAiMayAct(Player* pPlayer) const
{
    if (!getSimultaneousTurns())
    {
        return true;
    }
    // only the call of simultaneousAskAi lets the ai decide
    return m_simPhase == SimPhase::Collecting &&
           m_simAiBusy &&
           pPlayer != nullptr &&
           pPlayer->getPlayerID() == m_simAskedPlayer;
}

QString ActionPerformer::getSimultaneousInfo() const
{
    return QString::number(m_simExecuted) + ";" + QString::number(m_simCancelled) + ";" +
           QString::number(m_simAttacksDropped) + ";" + QString::number(m_simRounds);
}

void ActionPerformer::simultaneousShowStatus(const QString & notice)
{
    if (m_pMenu == nullptr || Mainapp::getInstance()->getNoUi())
    {
        return;
    }
    if (!notice.isEmpty())
    {
        m_simNotice = notice;
        m_simNoticeRound = m_simRounds;
    }
    else if (m_simRounds > m_simNoticeRound + 2)
    {
        m_simNotice.clear();
    }
    Mainapp::getInstance()->pauseRendering();
    if (m_simLabel.get() == nullptr)
    {
        constexpr qint32 width = 900;
        oxygine::TextStyle style = oxygine::TextStyle(FontManager::getMainFont24());
        style.hAlign = oxygine::TextStyle::HALIGN_LEFT;
        style.multiline = false;
        m_simLabel = MemoryManagement::create<Label>(width, true);
        m_simLabel->setStyle(style);
        m_simLabel->setPosition((oxygine::Stage::getStage()->getWidth() - width) / 2, 60);
        m_simLabel->setPriority(static_cast<qint32>(Mainapp::ZOrder::Objects));
        m_pMenu->addChild(m_simLabel);
    }
    QString text = tr("Simultaneous turns: day %1, round %2").arg(m_pMap->getCurrentDay()).arg(m_simRoundsToday + 1);
    if (m_simPhase == SimPhase::Collecting &&
        m_simAskedPlayer >= 0 &&
        dynamic_cast<CoreAI*>(m_pMap->getPlayer(m_simAskedPlayer)->getBaseGameInput()) == nullptr)
    {
        text += tr(". Player %1: give your order or end the day.").arg(m_simAskedPlayer + 1);
    }
    if (!m_simNotice.isEmpty())
    {
        text += " | " + m_simNotice;
    }
    m_simLabel->setText(text);
    Mainapp::getInstance()->continueRendering();
}

qint32 ActionPerformer::simultaneousAlivePlayers() const
{
    qint32 count = 0;
    for (qint32 i = 0; i < m_pMap->getPlayerCount(); ++i)
    {
        if (!m_pMap->getPlayer(i)->getIsDefeated())
        {
            ++count;
        }
    }
    return count;
}

bool ActionPerformer::simultaneousIntercept(spGameAction & pGameAction)
{
    if (m_simPhase != SimPhase::Collecting || pGameAction.get() == nullptr || !getSimultaneousTurns())
    {
        return false;
    }
    qint32 player = m_simAskedPlayer;
    if (player < 0)
    {
        player = m_pMap->getCurrentPlayer()->getPlayerID();
    }
    if (pGameAction->getActionID() == CoreAI::ACTION_NEXT_PLAYER)
    {
        m_simEnded[player] = true;
    }
    else
    {
        SimOrder order;
        order.pAction = pGameAction;
        order.pUnit = pGameAction->getTargetUnit();
        m_simQueues[player].push_back(order);
    }
    m_simAskedPlayer = -1;
    if (m_simAiBusy)
    {
        // simultaneousAskAi continues after the ai returned
        m_simCollectPending = true;
    }
    else
    {
        // continue outside of the call stack of the player input
        QTimer::singleShot(0, this, &ActionPerformer::simultaneousCollect);
    }
    return true;
}

void ActionPerformer::simultaneousOnActionPerformed()
{
    if (m_exit || m_actionRunning || !getSimultaneousTurns() || !m_pMenu->getGameStarted())
    {
        return;
    }
    switch (m_simPhase)
    {
        case SimPhase::Inactive:
        {
            // the first player got its start of day with the game start. The other players get it now.
            const qint32 count = m_pMap->getPlayerCount();
            m_simQueues.assign(count, std::deque<SimOrder>());
            m_simEnded.assign(count, false);
            m_simFirstPlayer = 0;
            m_simRolloverRemaining = simultaneousAlivePlayers() - 1;
            m_simPhase = SimPhase::InitialRollover;
            simultaneousRolloverStep();
            break;
        }
        case SimPhase::InitialRollover:
        case SimPhase::Rollover:
        {
            simultaneousRolloverStep();
            break;
        }
        case SimPhase::Resolving:
        {
            simultaneousResolveNext();
            break;
        }
        case SimPhase::Collecting:
        {
            break;
        }
    }
}

void ActionPerformer::simultaneousRolloverStep()
{
    if (m_simRolloverRemaining > 0)
    {
        // the normal next player action does the end and start of day processing of one player
        --m_simRolloverRemaining;
        spGameAction pAction = MemoryManagement::create<GameAction>(CoreAI::ACTION_NEXT_PLAYER, m_pMap);
        performAction(pAction);
    }
    else
    {
        for (qint32 i = 0; i < m_pMap->getPlayerCount(); ++i)
        {
            m_simEnded[i] = m_pMap->getPlayer(i)->getIsDefeated();
            m_simQueues[i].clear();
        }
        m_simRoundsToday = 0;
        m_simPhase = SimPhase::Collecting;
        simultaneousCollect();
    }
}

void ActionPerformer::simultaneousCollect()
{
    if (m_simAiBusy)
    {
        m_simCollectPending = true;
        return;
    }
    if (m_simPhase != SimPhase::Collecting || m_exit || m_simAskedPlayer >= 0)
    {
        return;
    }
    const qint32 count = m_pMap->getPlayerCount();
    bool anyOrder = false;
    for (qint32 i = 0; i < count; ++i)
    {
        const qint32 player = (m_simFirstPlayer + i) % count;
        if (m_pMap->getPlayer(player)->getIsDefeated())
        {
            m_simEnded[player] = true;
            m_simQueues[player].clear();
        }
        else if (!m_simQueues[player].empty())
        {
            anyOrder = true;
        }
        else if (!m_simEnded[player])
        {
            // this player still has to decide
            m_simAskedPlayer = player;
            m_pMap->setCurrentPlayer(player);
            if (dynamic_cast<CoreAI*>(m_pMap->getPlayer(player)->getBaseGameInput()) != nullptr)
            {
                QTimer::singleShot(0, this, [this, player]()
                {
                    simultaneousAskAi(player);
                });
            }
            else
            {
                // a human player gives the order with the normal input
                simultaneousShowStatus();
            }
            return;
        }
    }
    if (!anyOrder)
    {
        // all players ended the day
        CONSOLE_PRINT("Simultaneous turns: day " + QString::number(m_pMap->getCurrentDay()) + " finished after " +
                      QString::number(m_simRoundsToday) + " rounds. Totals: " + getSimultaneousInfo(), GameConsole::eINFO);
        m_simRolloverRemaining = simultaneousAlivePlayers();
        m_simPhase = SimPhase::Rollover;
        simultaneousRolloverStep();
        return;
    }
    m_simRoundOrder.clear();
    for (qint32 i = 0; i < count; ++i)
    {
        const qint32 player = (m_simFirstPlayer + i) % count;
        if (!m_simQueues[player].empty())
        {
            m_simRoundOrder.push_back(player);
        }
    }
    // the next round starts with the next player
    m_simFirstPlayer = (m_simFirstPlayer + 1) % count;
    m_simRoundIndex = 0;
    ++m_simRoundsToday;
    ++m_simRounds;
    m_simPhase = SimPhase::Resolving;
    simultaneousShowStatus();
    simultaneousResolveNext();
}

void ActionPerformer::simultaneousAskAi(qint32 player)
{
    if (m_simPhase != SimPhase::Collecting || m_simAskedPlayer != player || m_exit || m_simAiBusy)
    {
        return;
    }
    CoreAI* pAi = dynamic_cast<CoreAI*>(m_pMap->getPlayer(player)->getBaseGameInput());
    if (pAi != nullptr)
    {
        m_pMap->setCurrentPlayer(player);
        m_simAiBusy = true;
        // the ai emits its action directly which gets intercepted as order
        pAi->nextAction();
        m_simAiBusy = false;
    }
    if (m_simPhase == SimPhase::Collecting && m_simAskedPlayer == player)
    {
        // no order was given
        m_simEnded[player] = true;
        m_simAskedPlayer = -1;
    }
    m_simCollectPending = false;
    simultaneousCollect();
}

void ActionPerformer::simultaneousResolveNext()
{
    if (m_exit)
    {
        return;
    }
    while (m_simRoundIndex < static_cast<qint32>(m_simRoundOrder.size()))
    {
        const qint32 player = m_simRoundOrder[m_simRoundIndex];
        ++m_simRoundIndex;
        if (m_simQueues[player].empty())
        {
            continue;
        }
        SimOrder order = m_simQueues[player].front();
        m_simQueues[player].pop_front();
        m_pMap->setCurrentPlayer(player);
        spGameAction pAction = simultaneousValidate(order, player);
        if (pAction.get() != nullptr && pAction->getActionID() != order.pAction->getActionID())
        {
            simultaneousShowStatus(tr("Player %1: attack dropped, the target is gone. The unit only moved.").arg(player + 1));
        }
        if (pAction.get() == nullptr)
        {
            ++m_simCancelled;
            simultaneousShowStatus(tr("Player %1: order cancelled, it became impossible.").arg(player + 1));
            CONSOLE_PRINT("Simultaneous turns: cancelled order " + order.pAction->getActionID() + " of player " + QString::number(player), GameConsole::eDEBUG);
            continue;
        }
        ++m_simExecuted;
        // the round continues with the action performed signal
        performAction(pAction);
        return;
    }
    if (m_simRoundsToday >= SIMULTANEOUS_MAX_ROUNDS_PER_DAY)
    {
        CONSOLE_PRINT("Simultaneous turns: round limit reached. Ending the day.", GameConsole::eWARNING);
        for (qint32 i = 0; i < static_cast<qint32>(m_simEnded.size()); ++i)
        {
            m_simEnded[i] = true;
            m_simQueues[i].clear();
        }
    }
    m_simPhase = SimPhase::Collecting;
    simultaneousCollect();
}

spGameAction ActionPerformer::simultaneousValidate(SimOrder & order, qint32 player)
{
    Player* pPlayer = m_pMap->getPlayer(player);
    spGameAction pAction = order.pAction;
    if (pPlayer->getIsDefeated())
    {
        return spGameAction();
    }
    Unit* pUnit = pAction->getTargetUnit();
    if (order.pUnit != nullptr)
    {
        // the unit needs to be the same, alive and unused
        if (pUnit != order.pUnit ||
            pUnit->getOwner() != pPlayer ||
            pUnit->getHasMoved() ||
            pUnit->getHp() <= 0)
        {
            return spGameAction();
        }
    }
    // a visible enemy unit on the path blocks the whole move
    const QVector<QPoint> path = pAction->getMovePath();
    for (const auto & point : path)
    {
        if (!m_pMap->onMap(point.x(), point.y()))
        {
            return spGameAction();
        }
        Unit* pOther = m_pMap->getTerrain(point.x(), point.y())->getUnit();
        if (pOther != nullptr &&
            pOther != pUnit &&
            pPlayer->isEnemyUnit(pOther) &&
            !pOther->isStealthed(pPlayer))
        {
            return spGameAction();
        }
    }
    if (pAction->getActionID() == CoreAI::ACTION_FIRE)
    {
        // the damage of an attack is calculated when the order is given, so the attack is created again for the current state.
        // If the target is gone the unit only moves.
        pAction->startReading();
        const qint32 targetX = pAction->readDataInt32();
        const qint32 targetY = pAction->readDataInt32();
        pAction->startReading();
        bool attackPossible = false;
        if (m_pMap->onMap(targetX, targetY))
        {
            Terrain* pTargetTerrain = m_pMap->getTerrain(targetX, targetY);
            Unit* pDefender = pTargetTerrain->getUnit();
            // an empty field can still hold an attackable building or terrain
            attackPossible = (pDefender == nullptr && (pTargetTerrain->getBuilding() != nullptr || pTargetTerrain->getHp() > 0)) ||
                             (pDefender != nullptr && pPlayer->isEnemyUnit(pDefender));
        }
        spGameAction pNewAction;
        if (attackPossible)
        {
            pNewAction = MemoryManagement::create<GameAction>(CoreAI::ACTION_FIRE, m_pMap);
            pNewAction->setTarget(pAction->getTarget());
            pNewAction->setMovepath(path, pAction->getCosts());
            if (pNewAction->canBePerformed())
            {
                pNewAction->writeDataInt32(targetX);
                pNewAction->writeDataInt32(targetY);
                pNewAction->setInputStep(1);
                // calculates and stores the damage
                attackPossible = pNewAction->isFinalStep();
                if (attackPossible)
                {
                    pNewAction->startReading();
                    pNewAction->readDataInt32();
                    pNewAction->readDataInt32();
                    attackPossible = pNewAction->readDataInt32() >= 0;
                    pNewAction->startReading();
                }
            }
            else
            {
                attackPossible = false;
            }
        }
        if (!attackPossible)
        {
            if (path.size() <= 1)
            {
                // nothing left to do for this order. The unit stays available.
                return spGameAction();
            }
            ++m_simAttacksDropped;
            pNewAction = MemoryManagement::create<GameAction>(CoreAI::ACTION_WAIT, m_pMap);
            pNewAction->setTarget(pAction->getTarget());
            pNewAction->setMovepath(path, pAction->getCosts());
        }
        pAction = pNewAction;
    }
    if (!pAction->canBePerformed())
    {
        return spGameAction();
    }
    return pAction;
}
