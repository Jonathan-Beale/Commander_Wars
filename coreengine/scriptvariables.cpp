#include <algorithm>
#include <vector>
#include "coreengine/scriptvariables.h"
#include "coreengine/interpreter.h"
#include "coreengine/memorymanagement.h"

ScriptVariables::ScriptVariables()
{
#ifdef GRAPHICSUPPORT
    setObjectName("ScriptVariables");
#endif
    Interpreter::setCppOwnerShip(this);
}

void ScriptVariables::serializeObject(QDataStream& pStream) const
{
    serializeObject(pStream, false);
}

void ScriptVariables::serializeObject(QDataStream& pStream, bool forHash) const
{
    pStream << getVersion();
    if (forHash)
    {
        // The variables are hashed sorted by their name. The order they got created in differs between the computers of a
        // network match: reading a variable creates it and e.g. the ai and the info of the cursor only read on one computer.
        std::vector<ScriptVariable*> modified;
        for (qint32 i = 0; i < m_Variables.size(); i++)
        {
            if (m_Variables[i]->getModified())
            {
                modified.push_back(m_Variables[i].get());
            }
        }
        std::sort(modified.begin(), modified.end(), [](ScriptVariable* pLeft, ScriptVariable* pRight)
        {
            return pLeft->getId() < pRight->getId();
        });
        for (auto* pVariable : modified)
        {
            pVariable->serializeObject(pStream);
        }
        pStream << static_cast<qint32>(modified.size());
    }
    else
    {
        pStream << static_cast<qint32>(m_Variables.size());
        for (qint32 i = 0; i < m_Variables.size(); i++)
        {
            m_Variables[i]->serializeObject(pStream);
        }
    }
}

void ScriptVariables::deserializeObject(QDataStream& pStream)
{
    qint32 version = 0;
    pStream >> version;
    qint32 size = 0;
    pStream >> size;
    m_Variables.clear();
    for (qint32 i = 0; i < size; i++)
    {
        spScriptVariable pVar = MemoryManagement::create<ScriptVariable>();
        pVar->deserializeObject(pStream);
        m_Variables.append(pVar);
    }
}

ScriptVariable* ScriptVariables::createVariable(const QString & id)
{
    spScriptVariable pVar = getSpVariable(id);
    if (pVar.get() == nullptr)
    {
        pVar = MemoryManagement::create<ScriptVariable>(id);
        m_Variables.append(pVar);
    }
    return pVar.get();
}

void ScriptVariables::deleteVariable(const QString & id)
{
    for (qint32 i = 0; i < m_Variables.size(); i++)
    {
        if (m_Variables[i]->getId() == id)
        {
            m_Variables.removeAt(i);
        }
    }
}

spScriptVariable ScriptVariables::getSpVariable(const QString & id)
{
    for (qint32 i = 0; i < m_Variables.size(); i++)
    {
        if (m_Variables[i]->getId() == id)
        {
            return m_Variables[i];
        }
    }
    return spScriptVariable();
}

ScriptVariable* ScriptVariables::getVariable(const QString & id)
{
    return getSpVariable(id).get();
}

void ScriptVariables::clear()
{
    m_Variables.clear();
}
