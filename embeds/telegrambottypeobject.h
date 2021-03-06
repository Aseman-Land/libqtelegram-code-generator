// This file generated by libqtelegram-code-generator.
// You can download it from: https://github.com/Aseman-Land/libqtelegram-code-generator
// DO NOT EDIT THIS FILE BY HAND -- YOUR CHANGES WILL BE OVERWRITTEN

#ifndef TELEGRAMTYPEOBJECT_H
#define TELEGRAMTYPEOBJECT_H

#include <QCryptographicHash>
#include <QVariant>
#include <QMap>

#include "libqtelegram_global.h"

class LIBQTELEGRAMSHARED_EXPORT TelegramBotTypeObject
{
public:
    struct Null { };
    static const Null null;

    TelegramBotTypeObject();
    TelegramBotTypeObject(const Null&);
    ~TelegramBotTypeObject();

    virtual QByteArray getHash(QCryptographicHash::Algorithm alg) const = 0;

    bool error() const { return mError; }
    bool isNull() const { return mNull; }
    bool operator==(bool stt) { return mNull != stt; }
    bool operator!=(bool stt) { return !operator ==(stt); }

    static qint64 constructedCount() {
        return mConstructedCount;
    }

    virtual QMap<QString, QVariant> toMap() const = 0;
    virtual void fromMap(const QMap<QString, QVariant> &map) = 0;

protected:
    void setError(bool stt) { mError = stt; }
    void setNull(bool stt) { mNull = stt; }

private:
    bool mError;
    bool mNull;
    static qint64 mConstructedCount;
};

#endif // TELEGRAMTYPEOBJECT_H
