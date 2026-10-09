#pragma once

#include <QObject>

#include <deque>
#include <vector>

#include "game/gameaction.h"
#include "objects/base/label.h"
#include "game/gamemap.h"

class GameMenue;

class ActionPerformer final : public QObject
{
    Q_OBJECT
public:
    struct ConnectingInfo
    {
        ConnectingInfo(quint64 socketId)
            : m_socketId(socketId)
        {
        }
        quint64 m_socketId;
        bool m_syncSend{false};
    };

    struct SyncData
    {
        bool m_waitingForSyncFinished{false};
        spGameAction m_postSyncAction{nullptr};
        QVector<bool> m_lockedPlayers;
        QVector<ConnectingInfo> m_connectingSocketInfos;
    };

    explicit ActionPerformer(GameMap* pMap, GameMenue* pMenu = nullptr);
    /**
     * @brief getSyncData
     * @return
     */
    SyncData & getSyncData()
    {
        return m_multiplayerSyncData;
    }
    /**
     * @brief setSyncCounter
     * @param counter
     */
    void setSyncCounter(qint64 counter);
    /**
     * @brief setActionRunning
     * @param newActionRunning
     */
    void setActionRunning(bool newActionRunning);
    Q_INVOKABLE bool getExit() const;
    void setExit(bool newExit);
    /**
     * @brief centerMapOnAction
     * @param pGameAction
     */
    void centerMapOnAction(GameAction* pGameAction);
    /**
     * @brief actionPerformed
     */
    void actionPerformed();
    /**
     * @brief performAction performs the given action and deletes it afterwards.
     * @param pGameAction
     */
    Q_INVOKABLE void performAction(spGameAction pGameAction, bool fromAiPipe = false);
    /**
     * @brief isTrap
     * @param pAction
     * @param pMoveUnit
     * @param currentPoint
     * @param previousPoint
     * @return
     */
    bool isTrap(const QString & function, spGameAction pAction, Unit* pMoveUnit, QPoint currentPoint, QPoint previousPoint, qint32 moveCost);
    /**
     * @brief finsihActionPerformed
     */
    void finishActionPerformed();
    /**
     * @brief skipAnimations
     */
    void skipAnimations(bool postAnimation);
    /**
     * @brief doMultiTurnMovement
     * @param pGameAction
     */
    spGameAction doMultiTurnMovement(spGameAction pGameAction);
    /**
     * @brief delayedActionPerformed
     */
    Q_INVOKABLE void delayedActionPerformed();
    /**
     * @brief getActionRunning
     * @return
     */
    Q_INVOKABLE bool getActionRunning() const;
    /**
     * @brief getSyncCounter
     * @return
     */
    Q_INVOKABLE qint64 getSyncCounter() const;
    /**
     * @brief nextTurnPlayerTimeout
     */
    Q_INVOKABLE void nextTurnPlayerTimeout();
    /**
     * @brief getSimultaneousTurns if the simultaneous turns prototype is active (game rule GAMERULE_SIMULTANEOUS_TURNS).
     * In this mode all players act during the same day. Each player queues orders. A round executes the next order
     * of each player as soon as every player has an order queued or ended the day. The player executing the first
     * order rotates each round. Orders which became impossible are cancelled. The day ends when all players ended it.
     */
    Q_INVOKABLE bool getSimultaneousTurns() const;
    /**
     * @brief simultaneousAiMayAct if the ai of the given player is currently asked for its next order
     */
    bool simultaneousAiMayAct(Player* pPlayer) const;
    /**
     * @brief getSimultaneousInfo
     * @return executed orders, cancelled orders, attacks changed to moves and rounds since the game start separated by ';'
     */
    Q_INVOKABLE QString getSimultaneousInfo() const;
signals:
    void sigPerformAction(spGameAction pGameAction, bool fromAiPipe = false);
    void sigActionPerformed();
    void sigAiProcesseSendAction(spGameAction pGameAction);

protected:
    bool requiresForwarding(const spGameAction & pGameAction) const;
    /**
     * @brief doTrapping
     * @param pGameAction
     */
    void doTrapping(spGameAction & pGameAction);
    void onTriggeringActionFinished();
private:
    enum class SimPhase
    {
        Inactive,           /**< waiting for the game start */
        InitialRollover,    /**< start of day processing of the players after the first one */
        Collecting,         /**< waiting for the next order of the players */
        Resolving,          /**< executing the orders of a round */
        Rollover,           /**< end and start of day processing of all players */
    };
    struct SimOrder
    {
        spGameAction pAction;
        /**
         * @brief pUnit unit the order was given to. Only compared, never used
         */
        Unit* pUnit{nullptr};
    };
    /**
     * @brief simultaneousIntercept
     * @return true if the action was queued as order instead of being performed
     */
    bool simultaneousIntercept(spGameAction & pGameAction);
    void simultaneousOnActionPerformed();
    void simultaneousRolloverStep();
    void simultaneousCollect();
    void simultaneousAskAi(qint32 player);
    void simultaneousResolveNext();
    /**
     * @brief simultaneousValidate
     * @return the action to perform for the order or a nullptr if the order became impossible
     */
    spGameAction simultaneousValidate(SimOrder & order, qint32 player);
    qint32 simultaneousAlivePlayers() const;
    /**
     * @brief simultaneousShowStatus updates the status line of the mode. A notice is shown for the next rounds.
     */
    void simultaneousShowStatus(const QString & notice = QString());
private:
    SimPhase m_simPhase{SimPhase::Inactive};
    std::vector<std::deque<SimOrder>> m_simQueues;
    std::vector<bool> m_simEnded;
    std::vector<qint32> m_simRoundOrder;
    qint32 m_simRoundIndex{0};
    qint32 m_simFirstPlayer{0};
    qint32 m_simAskedPlayer{-1};
    /**
     * @brief m_simAiBusy an ai is deciding. It processes events while thinking, so nothing else of the round logic may run meanwhile.
     */
    bool m_simAiBusy{false};
    bool m_simCollectPending{false};
    spLabel m_simLabel;
    QString m_simNotice;
    qint32 m_simNoticeRound{0};
    qint32 m_simRolloverRemaining{0};
    qint32 m_simRoundsToday{0};
    qint32 m_simExecuted{0};
    qint32 m_simCancelled{0};
    qint32 m_simAttacksDropped{0};
    qint32 m_simRounds{0};
private:
    GameMenue* m_pMenu{nullptr};
    GameMap* m_pMap{nullptr};
    spGameAction m_pStoredAction{nullptr};
    spGameAction m_pCurrentAction{nullptr};
    bool m_actionRunning{false};
    bool m_noTimeOut{false};
    bool m_finishedPerformed{false};
    qint64 m_syncCounter{0};
    SyncData m_multiplayerSyncData;
    QTimer m_delayedActionPerformedTimer{this};
    bool m_exit{false};
    QByteArray m_mapHash;
    /**
     * @brief requiresMapHash if the hash of the whole map needs to be created for each action
     */
    bool requiresMapHash() const;
};

