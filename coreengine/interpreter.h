#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <unordered_map>
#include "coreengine/perftrace.h"
#include <QHash>
#include <QObject>
#include <QQmlEngine>
#include <QVector>
#include <QThread>
#include <list>
#include <QCoreApplication>
#include <QElapsedTimer>
#include "coreengine/jsthis.h"

using spQObject = std::shared_ptr<QObject>;

class ScriptFunctionSource;
using spScriptFunctionSource = std::shared_ptr<ScriptFunctionSource>;

class Interpreter;
using spInterpreter = std::shared_ptr<Interpreter>;
/**
 * @brief The Interpreter class java-script interpreter with easy access functions
 */
class Interpreter final : public QJSEngine
{
    Q_OBJECT
    struct JsThisData
    {
        QObject* pObject{nullptr};
        JsThis* pThis{nullptr};
    };
public:
    static Interpreter* getInstance()
    {
        Q_ASSERT(m_pInstance->thread() == QThread::currentThread());
        return m_pInstance.get();
    }
    static bool exists()
    {
        return m_pInstance.get() != nullptr;
    }
    static Interpreter* createInstance(QObject* pParent);
    virtual ~Interpreter();
    static void release();

    static void setCppOwnerShip(QObject* object);
    /**
     * @brief getRuntimeData
     * @return
     */
    QString getRuntimeData();
    /**
     * @brief reloadInterpreter
     * @param runtime
     */
    static bool reloadInterpreter(QString runtime);

    bool getInJsCall() const;
    void trackJsObject(spQObject pObj);

    template<typename _TType, template<typename T> class _TVectorList>
    QJSValue arraytoJSValue(const _TVectorList<_TType> & array)
    {
        QJSValue jsArray = newArray(array.size());
        for (qint32 i = 0; i < array.size(); i++)
        {
            if constexpr (std::is_same<_TType, QPoint>::value)
            {
                jsArray.setProperty(i, toScriptValue(array[i]));
            }
            else
            {
                jsArray.setProperty(i, array[i]);
            }
        }
        return jsArray;
    }

    void threadProcessEvents()
    {
        Q_ASSERT(m_inJsCall == 0);
        // The ai calls this inside of its loops, several thousand times for each action, to keep the game responsive.
        // Asking the event loop is slow compared to the loops, so it's only done every few milliseconds.
        if (m_processEventsTimer.isValid() && m_processEventsTimer.elapsed() < 4)
        {
            return;
        }
        QCoreApplication::processEvents(QEventLoop::ProcessEventsFlag::AllEvents, 5);
        m_processEventsTimer.start();
    }

signals:
    void sigNetworkGameFinished(qint32 value, QString id);
public slots:
    bool openScript(const QString & script, bool setup);
    bool loadScript(const QString & content, const QString & script);
    inline QJSValue doFunction(const QString & func, const QJSValueList& args = QJSValueList())
    {
        clearJsStack();
        QJSValue funcPointer = globalObject().property(func);
        ++m_inJsCall;
        QJSValue ret = funcPointer.call(args);
        exitJsCall();
        if (ret.isError())
        {
            printError(ret);
        }
        return ret;
    }
    inline QJSValue doFunction(const QString & obj, const QString & func, const QJSValueList& args = QJSValueList())
    {
        QJSValue ret;
        clearJsStack();
        QJSValue objPointer = globalObject().property(obj);
        if (objPointer.isObject())
        {
            QJSValue funcPointer = objPointer.property(func);
            if (funcPointer.isCallable())
            {
                // startup benchmarking: only the outermost script calls are measured
                const bool trace = PerfTrace::enabled() && m_inJsCall == 0;
                QElapsedTimer traceTimer;
                if (trace)
                {
                    traceTimer.start();
                }
                ++m_inJsCall;
                ret = funcPointer.call(args);
                exitJsCall();
                if (trace)
                {
                    PerfTrace::addJsCall(func, traceTimer.nsecsElapsed());
                }
                if (ret.isError())
                {
                    printError(ret);
                }
            }
        }
        return ret;
    }
    /**
     * @brief isDefaultFunction
     * @return true if obj.func is the same function as baseObj.func, e.g. a co which doesn't replace a hook of the base co.
     * Hooks which return a neutral value in the base object don't need to be called for such objects.
     * The result is cached, cause the script objects don't change while the interpreter exists.
     */
    inline bool isDefaultFunction(const QString & obj, const QString & baseObj, const QString & func)
    {
        auto & objCache = m_defaultFunctionCache[func][baseObj];
        auto iter = objCache.constFind(obj);
        if (iter != objCache.constEnd())
        {
            return iter.value();
        }
        const QJSValue baseFunction = globalObject().property(baseObj).property(func);
        const QJSValue function = globalObject().property(obj).property(func);
        const bool isDefault = baseFunction.isCallable() && function.strictlyEquals(baseFunction);
        objCache.insert(obj, isDefault);
        return isDefault;
    }
    /**
     * @brief doString immediately interprates the string with the javascript-interpreter
     * @param task string parsed to the interpreter
     */
    QJSValue doString(const QString & task);
    /**
     * @brief pushInt
     * @param value
     */
    void pushInt(const QString & name, qint32 value);
    void pushDouble(const QString & name, double value);
    void pushString(const QString & name, const QString & value);
    void pushObject(const QString & name, QObject* object);
    inline QJSValue newQObject(QObject* object)
    {
        return QJSEngine::newQObject(object);
    }
    void deleteObject(const QString & name);

    qint32 getGlobalInt(const QString & var);
    bool getGlobalBool(const QString & var);
    double getGlobalDouble(const QString & var);
    QString getGlobalString(const QString & var);
    QJSValue getGlobal(const QString & var);
    void setGlobal(const QString & var, const QJSValue & obj);
    /**
     * @brief exists checks if the js object exists
     * @param object
     * @return
     */
    bool exists(const QString & object)
    {
        QJSValue objPointer = globalObject().property(object);
        if (objPointer.isObject())
        {
            return true;
        }
        return false;
    }
    /**
     * @brief exists checks if the js object and function exists
     * @param object
     * @return
     */
    bool exists(const QString & object, const QString & function)
    {
        QJSValue objPointer = globalObject().property(object);
        if (objPointer.isObject())
        {
            QJSValue funcPointer = objPointer.property(function);
            if (funcPointer.isCallable())
            {
                return true;
            }
        }
        return false;
    }
    QJSValue newQJsObject(QObject* object, JsThis* me)
    {
        m_ownedObjects.push_back({object, me});
        // the first entry of an object is the one which gets removed, like searching the list from the front
        m_ownedObjectIndex.emplace(me, std::prev(m_ownedObjects.end()));
        return newQObject(object);
    }
    void deleteQJsObject(JsThis* me)
    {
        // large maps own several ten thousand objects and the ai creates and deletes point lists all the time,
        // so the entry is found with an index instead of searching the list.
        auto index = m_ownedObjectIndex.find(me);
        if (index != m_ownedObjectIndex.end())
        {
            m_ownedObjects.erase(index->second);
            m_ownedObjectIndex.erase(index);
        }
    }

private slots:
    void networkGameFinished(qint32 value, QString id);
private:
    friend class MemoryManagement;
    explicit Interpreter(QObject* pParent);    
    /**
     * @brief init
     */
    void init();
    void exitJsCall()
    {
        --m_inJsCall;
        Q_ASSERT(m_inJsCall >= 0);
    }
    void clearJsStack()
    {
        if (m_inJsCall == 0)
        {
            m_jsObjects.clear();
        }
    }
    void printError(const QString & msg);
    void printError(const QJSValue & value);
private:
    static spInterpreter m_pInstance;
    static QString m_runtimeData;
    spScriptFunctionSource m_scriptFunctionSource;
    qint32 m_inJsCall{0};
    QElapsedTimer m_processEventsTimer;
    QHash<QString, QHash<QString, QHash<QString, bool>>> m_defaultFunctionCache;
    std::vector<spQObject> m_jsObjects;
    std::list<JsThisData> m_ownedObjects;
    std::unordered_map<JsThis*, std::list<JsThisData>::iterator> m_ownedObjectIndex;
    qint32 m_jsCallCount{0};
};

#endif // INTERPRETER_H
