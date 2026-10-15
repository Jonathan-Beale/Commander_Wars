#include "coreengine/hexgrid.h"
#include "coreengine/interpreter.h"
#include "coreengine/audiomanager.h"

#include "game/gamemap.h"
#include "game/cursor.h"
#include "game/createoutline.h"

#include "menue/basegamemenu.h"

#include "resource_management/objectmanager.h"

Cursor::Cursor(GameMap* pMap)
    : m_pMap{pMap}
{
#ifdef GRAPHICSUPPORT
    setObjectName("Cursor");
#endif
    Interpreter::setCppOwnerShip(this);
    changeCursor("cursor+default");
    setPriority(static_cast<qint32>(Mainapp::ZOrder::Cursor));
    m_cursorRangeOutline = MemoryManagement::create<oxygine::Actor>();
    addChild(m_cursorRangeOutline);
}

void Cursor::changeCursor(const QString & spriteID, qint32 xOffset, qint32 yOffset, float scale)
{    
    if (m_areaOutline)
    {
        resetCursorRangeOutline();
        m_areaOutline = false;
    }
    if (HexGrid::enabled() && spriteID == "cursor+missile")
    {
        // The cursor of area effects like the missile is an image of the area on the square grid.
        // On the hex grid the fields of the area are outlined instead: the scale of the image is the range of the area.
        changeCursor("cursor+default");
        addCursorRangeOutline(qRound(scale), QColor(255, 60, 60));
        m_areaOutline = true;
        return;
    }
    ObjectManager* pObjectManager = ObjectManager::getInstance();
    if (m_CurrentCursor.get() != nullptr)
    {
        m_CurrentCursor->detachAndRemove();
        m_CurrentCursor.reset();
    }
    oxygine::ResAnim* pAnim = pObjectManager->getResAnim(spriteID);
    m_CurrentCursor = MemoryManagement::create<oxygine::Sprite>();
    if (pAnim != nullptr)
    {
        if (pAnim->getTotalFrames() > 1)
        {
            oxygine::spTween tween = oxygine::createTween(oxygine::TweenAnim(pAnim), oxygine::timeMS(pAnim->getTotalFrames() * 200), -1);
            m_CurrentCursor->addTween(tween);
        }
        else
        {
            m_CurrentCursor->setResAnim(pAnim);
        }
    }
    m_CurrentCursor->setScale(scale * static_cast<float>(GameMap::getImageSize()) / static_cast<float>(GameMap::defaultImageSize));
    m_CurrentCursor->setPosition(xOffset, yOffset);
    addChild(m_CurrentCursor);    
}

void Cursor::setMapPoint(qint32 x, qint32 y)
{    
    if (m_pMap != nullptr)
    {
        m_onMap = m_pMap->onMap(x, y);
        if (m_onMap)
        {
            // play tick sound when changing the field
            if ((x != m_MapPointX) ||
                (y != m_MapPointY))
            {
                Mainapp::getInstance()->getAudioManager()->playSound("switchfield.wav");
                m_MapPointX = x;
                m_MapPointY = y;
                setPosition(HexGrid::fieldToScreen(x, y, GameMap::getImageSize()));
                // provide cursor move signal
                emit sigCursorMoved(m_MapPointX, m_MapPointY);
            }
        }
    }
}

void Cursor::updatePosition(qint32 mousePosX, qint32 mousePosY)
{    
    if (m_pMap != nullptr)
    {
        auto* pMenu = m_pMap->getMenu();
        if (pMenu != nullptr)
        {
            const QPoint field = HexGrid::screenToField(mousePosX, mousePosY, GameMap::getImageSize());
            setMapPoint(field.x(), field.y());
        }
    }
}

GameMap *Cursor::getMap() const
{
    return m_pMap;
}

void Cursor::addCursorRangeOutline(qint32 range, QColor color)
{
    CreateOutline::addCursorRangeOutline(m_cursorRangeOutline, "cursor+border", range, color);
}

void Cursor::resetCursorRangeOutline()
{    
    m_cursorRangeOutline->removeChildren();    
}
