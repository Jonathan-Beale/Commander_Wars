#include "coreengine/Gamepad.h"
#include <xinput.h>
#include <dinput.h>
#include <mmsystem.h>
#include <QDateTime>
#include "coreengine/mainapp.h"

Gamepad::~Gamepad() = default;

void Gamepad::update()
{
    if (!Mainapp::getInstance()->isActive())
    {
        return;
    }
    XINPUT_STATE xstate;
    qint32 result = XInputGetState(m_gamepadId, &xstate);
    if (result != ERROR_SUCCESS)
    {
        // no xbox style controller: use the first other game controller known to windows e.g. a playstation controller.
        // The button numbers follow the usual layout of those controllers:
        // 0 square, 1 cross, 2 circle, 3 triangle, 4 L1, 5 R1, 6 L2, 7 R2, 8 share, 9 options, 10 L3, 11 R3
        static qint32 joystickId = -1;
        static qint64 nextSearch = 0;
        JOYINFOEX info;
        memset(&info, 0, sizeof(info));
        info.dwSize = sizeof(info);
        info.dwFlags = JOY_RETURNALL;
        if (joystickId < 0 || joyGetPosEx(joystickId, &info) != JOYERR_NOERROR)
        {
            joystickId = -1;
            const qint64 now = QDateTime::currentMSecsSinceEpoch();
            if (now >= nextSearch)
            {
                // asking for missing controllers is slow, so only search every two seconds
                nextSearch = now + 2000;
                for (quint32 id = 0; id < 16; ++id)
                {
                    info.dwSize = sizeof(info);
                    info.dwFlags = JOY_RETURNALL;
                    if (joyGetPosEx(id, &info) == JOYERR_NOERROR)
                    {
                        joystickId = id;
                        break;
                    }
                }
            }
            if (joystickId < 0)
            {
                m_deviceState = DeviceState::Unavailable;
                return;
            }
        }
        m_deviceState = DeviceState::Available;
        auto pressed = [&info](quint32 button)
        {
            return (info.dwButtons & (1u << button)) != 0;
        };
        // the hat is given as angle in hundredths of a degree
        m_inputValues.m_hatX = 0;
        m_inputValues.m_hatY = 0;
        if ((info.dwPOV & 0xFFFF) != 0xFFFF)
        {
            const quint32 angle = info.dwPOV;
            if (angle >= 31500 || angle <= 4500)
            {
                m_inputValues.m_hatY = 1;
            }
            else if (angle >= 13500 && angle <= 22500)
            {
                m_inputValues.m_hatY = -1;
            }
            if (angle >= 22500 && angle <= 31500)
            {
                m_inputValues.m_hatX = 1;
            }
            else if (angle >= 4500 && angle <= 13500)
            {
                m_inputValues.m_hatX = -1;
            }
        }
        // axes are 0 to 65535 and grow to the right and downwards
        m_inputValues.m_leftX = getAxisValue(static_cast<qint32>(info.dwXpos) - 32768);
        m_inputValues.m_leftY = getAxisValue(static_cast<qint32>(info.dwYpos) - 32768);
        m_inputValues.m_rightX = getAxisValue(static_cast<qint32>(info.dwZpos) - 32768);
        m_inputValues.m_rightY = getAxisValue(static_cast<qint32>(info.dwRpos) - 32768);
        // the d-pad moves the mouse pointer like the left stick
        if (m_inputValues.m_hatX != 0)
        {
            m_inputValues.m_leftX = -m_inputValues.m_hatX;
        }
        if (m_inputValues.m_hatY != 0)
        {
            m_inputValues.m_leftY = -m_inputValues.m_hatY;
        }
        // the left stick and the d-pad move the pointer the same way as the right stick
        if (qAbs(m_inputValues.m_rightX) < MinCursorTilt && qAbs(m_inputValues.m_rightY) < MinCursorTilt)
        {
            m_inputValues.m_rightX = m_inputValues.m_leftX;
            m_inputValues.m_rightY = m_inputValues.m_leftY;
        }
        m_inputValues.m_leftX = 0;
        m_inputValues.m_leftY = 0;
        // L2 and R2 zoom out and in. The zoom is the vertical wheel value of handleAxes.
        m_inputValues.m_hatX = 0;
        m_inputValues.m_hatY = 0;
        if (pressed(7))
        {
            m_inputValues.m_hatY = 1;
        }
        else if (pressed(6))
        {
            m_inputValues.m_hatY = -1;
        }
        handleButtonPress(m_keys[Buttons::B], pressed(2));
        handleAxes(m_inputValues);
        const qint32 state = static_cast<qint32>(info.dwButtons);
        if (m_updateCounter == state)
        {
            return;
        }
        m_updateCounter = state;
        // cross is the left mouse click at the pointer. On the map this is the same as confirm.
        // The confirm key isn't send as well, cause both together would select twice.
        handleThumbStickPress(pressed(1), false);
        handleButtonPress(m_keys[Buttons::X], pressed(0));
        handleButtonPress(m_keys[Buttons::Y], pressed(3));
        handleButtonPress(m_keys[Buttons::Start], pressed(9));
        handleButtonPress(m_keys[Buttons::Select], pressed(8));
        // L1 and L3 select the previous unit, R1 and R3 the next one
        handleButtonPress(m_keys[Buttons::L1], pressed(4) || pressed(10));
        handleButtonPress(m_keys[Buttons::R1], pressed(5) || pressed(11));
        return;
    }
    m_deviceState = DeviceState::Available;    
    if ((xstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0)
    {
        m_inputValues.m_hatY = 1;
    }
    else if ((xstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0)
    {
        m_inputValues.m_hatY = -1;
    }
    if ((xstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0)
    {
        m_inputValues.m_hatX = 1;
    }
    else if ((xstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0)
    {
        m_inputValues.m_hatX = -1;
    }
    m_inputValues.m_leftX = getAxisValue(xstate.Gamepad.sThumbLX);
    m_inputValues.m_leftY = getAxisValue(xstate.Gamepad.sThumbLY);
    m_inputValues.m_rightX = getAxisValue(xstate.Gamepad.sThumbRX);
    m_inputValues.m_rightY = getAxisValue(xstate.Gamepad.sThumbRY);
    handleButtonPress(m_keys[Buttons::A], (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0);
    handleButtonPress(m_keys[Buttons::B], (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0);
    handleAxes(m_inputValues);

    if (m_updateCounter == xstate.dwPacketNumber)
    {
        return;
    }
    m_updateCounter = xstate.dwPacketNumber;

    handleThumbStickPress((xstate.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0, (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0);
    handleThumbStickPress((xstate.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0, (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0);
    
    handleButtonPress(m_keys[Buttons::X], (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0);
    handleButtonPress(m_keys[Buttons::Y], (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0);
    handleButtonPress(m_keys[Buttons::Start], (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0);
    handleButtonPress(m_keys[Buttons::Select], (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0);
    handleButtonPress(m_keys[Buttons::L1], xstate.Gamepad.bLeftTrigger != 0);
    handleButtonPress(m_keys[Buttons::R1], xstate.Gamepad.bRightTrigger != 0);
}

bool Gamepad::isSupported()
{
    return true;
}
