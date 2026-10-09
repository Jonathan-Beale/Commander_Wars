#include "menue/basemenu.h"

Basemenu::Basemenu()
    : m_onEnterTimer(this)
{
    connect(&m_onEnterTimer, &QTimer::timeout, this, &Basemenu::onEnter);
    m_onEnterTimer.setSingleShot(true);
    // the automation hooks of init.js are called after this delay. COW_MENU_ENTER_DELAY_MS shortens it for automated runs.
    bool ok = false;
    qint32 delay = qEnvironmentVariableIntValue("COW_MENU_ENTER_DELAY_MS", &ok);
    if (!ok || delay < 0)
    {
        delay = 10000;
    }
    m_onEnterTimer.start(delay);
}

bool Basemenu::getFocused() const
{
    return m_Focused;
}

void Basemenu::setFocused(bool Focused)
{
    m_Focused = Focused;
}

void Basemenu::showCustomDialog(const QString & jsName, const QString & uiXml, const QString & confirmText, bool contentString)
{
    spCustomDialog pCustomDialog = MemoryManagement::create<CustomDialog>(jsName, uiXml, this, confirmText, contentString);
    addChild(pCustomDialog);
}