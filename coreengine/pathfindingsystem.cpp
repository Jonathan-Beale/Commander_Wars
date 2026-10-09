#include <algorithm>
#include "QTime"

#include "coreengine/interpreter.h"
#include "coreengine/pathfindingsystem.h"
#include "coreengine/hexgrid.h"

const qint32 PathFindingSystem::infinite = std::numeric_limits<qint32>::max();

bool operator<(const PathFindingSystem::Node& pNodeLhs, const PathFindingSystem::Node& pNodeRhs)
{
    return (pNodeLhs.totalCost < pNodeRhs.totalCost) || // cheaper end cost
            // same end cost but currently cheaper cost
            (pNodeLhs.currentCosts < pNodeRhs.currentCosts && pNodeLhs.totalCost == pNodeRhs.totalCost) ||
            // same
            (pNodeLhs.distance < pNodeRhs.distance && pNodeLhs.currentCosts == pNodeRhs.currentCosts && pNodeLhs.totalCost == pNodeRhs.totalCost) ||
            (pNodeLhs.queueOrder < pNodeRhs.queueOrder && pNodeLhs.distance == pNodeRhs.distance &&
             pNodeLhs.currentCosts == pNodeRhs.currentCosts && pNodeLhs.totalCost == pNodeRhs.totalCost);
}

PathFindingSystem::PathFindingSystem(qint32 startX, qint32 startY,
                                     qint32 width, qint32 heigth)
    : m_StartPoint(startX, startY),
    m_width(width),
    m_height(heigth),
    m_costs(width * heigth, infinite),
    m_DirectionMap(width * heigth, Directions::Unknown),
    m_movecosts(width * heigth, std::array<qint32, Directions::Max>({infinite, infinite, infinite, infinite, infinite, infinite}))
{
#ifdef GRAPHICSUPPORT
    setObjectName("PathFindingSystem");
#endif
    Interpreter::setCppOwnerShip(this);
}

void PathFindingSystem::setStartPoint(qint32 startX, qint32 startY)
{
    m_StartPoint = QPoint(HexGrid::wrapX(startX), startY);
}

void PathFindingSystem::setFinishNode(qint32 x, qint32 y)
{
    x = HexGrid::wrapX(x);
    m_FinishNode = x + y  * m_width;
    m_FinishNodeX = x;
    m_FinishNodeY = y;
}

void PathFindingSystem::explore()
{
    if (m_StartPoint.x() < 0 || m_StartPoint.y() < 0
        || m_StartPoint.x() >= m_width || m_StartPoint.y() >= m_height)
    {
        oxygine::handleErrorPolicy(oxygine::ep_show_error, "PathFindingSystem::explore invalid start point");
        return;
    }
    qint32 neighboursIndex = getIndex(m_StartPoint.x(), m_StartPoint.y());
    qint64 queueOrder = 0;
    m_OpenList.push_back(Node(m_StartPoint.x(), m_StartPoint.y(), neighboursIndex, 0, 0,
                              m_StartPoint.x(), m_StartPoint.y(), 0, queueOrder++));
    qint32 remainingCosts;
    qint32 neighboursX = -1;
    qint32 neighboursY = -1;
    qint32 fieldCost = -1;
    qint32 neighboursCosts = -1;
    // explore till we reached the end
    while (!m_OpenList.empty())
    {
        // get current node and pop it
        std::pop_heap(m_OpenList.begin(), m_OpenList.end(), [](const Node& lhs, const Node& rhs)
        {
            return rhs < lhs;
        });
        Node pCurrent = m_OpenList.back();
        m_OpenList.pop_back();
        if (pCurrent.index < 0 || m_costs[pCurrent.index] != infinite)
        {
            if (pCurrent.index < 0)
            {
                oxygine::handleErrorPolicy(oxygine::ep_show_error, "PathFindingSystem::explore unknown search error");
            }
            // already searched item
            continue;
        }
        // still the best node?
        qint32 currentCost = pCurrent.currentCosts;
        m_costs[pCurrent.index] = currentCost;
        m_DirectionMap[pCurrent.index] = static_cast<Directions>(getMoveDirection(pCurrent.prevNodeX, pCurrent.prevNodeY,
                                                                                  pCurrent.x, pCurrent.y));
        if (finished(pCurrent.x, pCurrent.y, pCurrent.totalCost))
        {
            setFinishNode(pCurrent.x, pCurrent.y);
            break;
        }
        const qint32 neighbourCount = HexGrid::neighbourCount();
        for (qint32 i = 0; i < neighbourCount; i++)
        {
            // calculate neighbour node data
            neighboursX = HexGrid::wrapX(pCurrent.x + HexGrid::neighbours[i].x());
            neighboursY = pCurrent.y + HexGrid::neighbours[i].y();
            if (neighboursX < 0 || neighboursX >= m_width ||
                neighboursY < 0 || neighboursY >= m_height)
            {
                continue;
            }
            neighboursIndex = getIndex(neighboursX, neighboursY);
            fieldCost = m_costs[neighboursIndex];
            if (fieldCost != infinite)
            {
                // skip searched fields
                continue;
            }
            // get field costs from index
            neighboursCosts = getCosts(neighboursIndex, neighboursX, neighboursY, pCurrent.x, pCurrent.y, currentCost);
            if (neighboursCosts >= 0) // passable?
            {
                // costs to reach this field
                qint32 newCosts = neighboursCosts + currentCost;
                remainingCosts = getRemainingCost(neighboursX, neighboursY, newCosts);
                // usable path to target?
                if (remainingCosts >= 0)
                {
                    qint32 totalCost = newCosts + remainingCosts;
                    // node we want to insert
                    Node workNode = Node(neighboursX, neighboursY, neighboursIndex, totalCost, newCosts,
                                         pCurrent.x, pCurrent.y,
                                         HexGrid::distance(neighboursX - m_StartPoint.x(), neighboursY - m_StartPoint.y()),
                                         queueOrder++);
                    m_OpenList.push_back(workNode);
                    std::push_heap(m_OpenList.begin(), m_OpenList.end(), [](const Node& lhs, const Node& rhs)
                    {
                        return rhs < lhs;
                    });
                }
            }
        }
    }
}

std::vector<QPoint> PathFindingSystem::getFieldsFast(qint32 startX, qint32 startY, qint32 min, qint32 max)
{
    std::vector<QPoint> points;
    for (qint32 x = -max; x <= max; x++)
    {
        for (qint32 y = -max; y <= max; y++)
        {
            const qint32 distance = HexGrid::distance(x, y);
            if ((distance >= min) && (distance <= max))
            {
                points.push_back(QPoint(startX + x, startY + y));
            }
        }
    }
    return points;
}

std::vector<QPoint> PathFindingSystem::getAllNodePointsFast(qint32 maxRange)
{
    std::vector<QPoint> points;
    for (qint32 x = 0; x < m_width; x++)
    {
        for (qint32 y = 0; y < m_height; y++)
        {
            qint32 cost = m_costs[getIndex(x, y)];
            if (cost >= 0 && cost < maxRange)
            {
                points.push_back(QPoint(x, y));
            }
        }
    }
    return points;
}

QmlVectorPoint* PathFindingSystem::getAllQmlVectorPoints()
{
    auto* ret = MemoryManagement::createAndTrackJsObject<QmlVectorPoint>();
    for (qint32 x = 0; x < m_width; x++)
    {
        for (qint32 y = 0; y < m_height; y++)
        {
            qint32 cost = m_costs[getIndex(x, y)];
            if (cost >= 0 && cost < infinite)
            {
                ret->append(QPoint(x, y));
            }
        }
    }
    return ret;
}

std::vector<QPoint> PathFindingSystem::getTargetPathFast() const
{
    if (m_FinishNode >= 0)
    {
        return getPathFast(m_FinishNodeX, m_FinishNodeY);
    }
    return std::vector<QPoint>();
}

QPoint PathFindingSystem::getTarget() const
{
    if (m_FinishNode >= 0)
    {
        return QPoint(m_FinishNodeX, m_FinishNodeY);
    }
    return QPoint(-1, -1);
}

std::vector<QPoint> PathFindingSystem::getPathFast(qint32 x, qint32 y) const
{
    std::vector<QPoint> points;
    qint32 startCost = getTargetCosts(x, y);
    if (startCost >= 0)
    {
        qint32 curX = HexGrid::wrapX(x);
        qint32 curY = y;
        points.push_back(QPoint(curX, curY));
        while (curX != m_StartPoint.x() ||
               curY != m_StartPoint.y())
        {
            switch (m_DirectionMap[getIndex(curX, curY)])
            {
                case Directions::East:
                {
                    curX -= 1;
                    break;
                }
                case Directions::West:
                {
                    curX += 1;
                    break;
                }
                case Directions::South:
                {
                    curY -= 1;
                    break;
                }
                case Directions::North:
                {
                    curY += 1;
                    break;
                }
                case Directions::NorthEast:
                {
                    curX -= 1;
                    curY += 1;
                    break;
                }
                case Directions::SouthWest:
                {
                    curX += 1;
                    curY -= 1;
                    break;
                }
                default:
                {
                    return points;
                }
            }
            curX = HexGrid::wrapX(curX);
            points.push_back(QPoint(curX, curY));
        }
    }
    return points;
}

qint32 PathFindingSystem::getTargetCosts(qint32 x, qint32 y) const
{
    x = HexGrid::wrapX(x);
    if (x >= 0 && x < m_width &&
        y >= 0 && y < m_height)
    {
        const qint32 cost = m_costs[getIndex(x, y)];
        if (cost < infinite)
        {
            return cost;
        }
    }
    return -1;
}

bool PathFindingSystem::isReachable(qint32 x, qint32 y) const
{
    return getTargetCosts(x, y) >= 0;
}

QVector<QPoint> PathFindingSystem::getPath(qint32 x, qint32 y) const
{
    auto points = getPathFast(x, y);
    QVector<QPoint> path;
    path.reserve((points.size()));
    for (auto & point : points)
    {
        path.append(point);
    }
    return path;
}

QVector<QPoint> PathFindingSystem::getTargetPath() const
{
    auto points = getTargetPathFast();
    QVector<QPoint> path;
    path.reserve((points.size()));
    for (auto & point : points)
    {
        path.append(point);
    }
    return path;
}

QVector<QPoint> PathFindingSystem::getFields(qint32 startX, qint32 startY, qint32 min, qint32 max)
{
    auto points = getFieldsFast(startX, startY, min, max);
    QVector<QPoint> path;
    path.reserve((points.size()));
    for (auto & point : points)
    {
        path.append(point);
    }
    return path;
}

bool PathFindingSystem::hasPoints(qint32 startX, qint32 startY) const
{
    const qint32 neighbourCount = HexGrid::neighbourCount();
    for (qint32 i = 0; i < neighbourCount; ++i)
    {
        if (getTargetCosts(startX + HexGrid::neighbours[i].x(), startY + HexGrid::neighbours[i].y()) > 0)
        {
            return true;
        }
    }
    return false;
}

QVector<QPoint> PathFindingSystem::getAllNodePoints(qint32 maxRange)
{
    auto points = getAllNodePointsFast(maxRange);
    QVector<QPoint> path;
    path.reserve((points.size()));
    for (auto & point : points)
    {
        path.append(point);
    }
    return path;
}
