// This file generated by libqtelegram-code-generator.
// You can download it from: https://github.com/Aseman-Land/libqtelegram-code-generator
// DO NOT EDIT THIS FILE BY HAND -- YOUR CHANGES WILL BE OVERWRITTEN

#ifndef TELEGRAMCORE_H
#define TELEGRAMCORE_H

#include <QObject>
#include <QPointer>
#include <QTimerEvent>

#include <functional>
#include <memory>
#include <typeinfo>
#include <utility>

#include "telegramapi.h"
#include "telegramcore_globals.h"
#include "libqtelegram_global.h"

class TelegramApi;
class LIBQTELEGRAMSHARED_EXPORT TelegramCore : public QObject
{
    Q_OBJECT
public:
    TelegramCore(QObject *parent = 0);
    ~TelegramCore();

    class CallbackError {
    public:
        CallbackError() : errorCode(0), null(true) {}
        qint32 errorCode;
        QString errorText;
        bool null;
    };

    template<typename T>
    using Callback = std::function<void (qint64,T,CallbackError)>;

    static CallbackError apiError() {
        CallbackError error;
        error.errorCode = -1;
        error.errorText = "LIBQTELEGRAM_API_ERROR";
        error.null = false;
        return error;
    }

    static qint32 timeOut() { return mTimeOut; }
    static void setTimeOut(const qint32 &timeOut) { mTimeOut = timeOut; }

    QVariantHash lastArguments() const {
        return mLastArgs;
    }

    virtual void init() = 0;
    bool isConnected() const;

/*! === methods === !*/

Q_SIGNALS:
/*! === signals === !*/
    void error(qint64 id, qint32 errorCode, const QString &errorText, const QString &functionName);

protected Q_SLOTS:
/*! === events === !*/
    virtual void onError(qint64 id, qint32 errorCode, const QString &errorText, const QString &functionName, const QVariant &attachedData, bool &accepted);

protected:
    qint64 retry(qint64 msgId);
    void timerEvent(QTimerEvent *e);

    void setApi(TelegramApi *api);
    QPointer<TelegramApi> mApi;

    void stopTimeOut(qint64 msgId) {
        qint32 timer = mTimer.take(msgId);
        if(timer) killTimer(timer);
    }

    void startTimeOut(qint64 msgId, int timeOut) {
        stopTimeOut(msgId);
        if(!timeOut) return;
        mTimer[msgId] = startTimer(timeOut);
    }

    template<typename T>
    void callBackPush(qint64 msgId, Callback<T> callback) {
        if(!callback || mCallbacks.contains(msgId)) return;
        mCallbacks.insert(msgId, CallbackStore(std::move(callback)));
    }

    template<typename T>
    Callback<T> callBackGet(qint64 msgId);

    template<typename T>
    void callBackCall(qint64 msgId, const T &result, const CallbackError &error = CallbackError()) {
        auto cbs = mCallbacks.take(msgId);
        cbs(msgId, result, error);
    }

private:
    class CallbackStore;
    QHash<qint64, CallbackStore> mCallbacks;
    QHash<qint64, QVariantHash> mRecallArgs;
    QVariantHash mLastArgs;
    QHash<qint64, qint32> mTimer;
    static qint32 mTimeOut;

    /*! === privates === !*/
};


class TelegramCore::CallbackStore : public QObject
{
    Q_OBJECT
public:
    template<class T>
    CallbackStore(Callback<T> func) : m_argResType(&typeid(T)) {
        if (!func) return;
        m_ptrCb = std::shared_ptr<void>(reinterpret_cast<void*>(new Callback<T>(std::forward<Callback<T>>(func))), [](void* p) {
            if(p) delete reinterpret_cast<Callback<T>*>(p);
            p = nullptr;
        });
    }

    CallbackStore() : m_argResType(nullptr) {}
    CallbackStore(const CallbackStore& rhs) : m_argResType(rhs.m_argResType), m_ptrCb(rhs.m_ptrCb) {}
    CallbackStore(CallbackStore&& rhs) noexcept : m_argResType(std::move(rhs.m_argResType)), m_ptrCb(std::move(rhs.m_ptrCb)) {}
    CallbackStore& operator=(const CallbackStore& rhs) {
        if(&rhs != this) {
            m_argResType = rhs.m_argResType;
            m_ptrCb = rhs.m_ptrCb;
        }
        return *this;
    }

    CallbackStore& operator=(CallbackStore&& rhs) noexcept {
        if(&rhs != this) {
            m_argResType = std::move(rhs.m_argResType);
            m_ptrCb = std::move(rhs.m_ptrCb);
        }
        return *this;
    }

    template<typename T>
    void operator()(qint64 msgId, T&& result, const CallbackError& error) const {
        if (*this && typeid(T) == (*m_argResType)) {
            auto cb = reinterpret_cast<Callback<T>*>(m_ptrCb.get());
            if (!cb || !bool(*cb)) return;
            (*cb)(msgId, std::forward<T>(result), error);
        }
    }

    template<typename T>
    Callback<T> getCallback() const {
        if(!bool(*this) || typeid(T) != (*m_argResType)) return 0;
        QPointer<const CallbackStore> guard(this);
        return [guard] (qint64 msgId, T&& result, const CallbackError& error) {
             if(guard) {
                 guard->operator()(msgId, std::forward<T>(result), error);
             }
        };
    }

    explicit operator bool() const {
        return bool(m_ptrCb);
    }

private:
    const std::type_info* m_argResType;
    std::shared_ptr<void> m_ptrCb;
};

template<typename T>
TelegramCore::Callback<T> TelegramCore::callBackGet(qint64 msgId) {
    return mCallbacks.value(msgId).getCallback<T>();
}

#endif // TELEGRAMCORE_H
