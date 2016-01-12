#ifndef FUNCTIONGENERATOR_H
#define FUNCTIONGENERATOR_H

#include "abstractgenerator.h"

#include <QStringList>
#include <QRegExp>
#include <QMap>
#include <QSet>

class FunctionGenerator: public AbstractGenerator
{
public:
    FunctionGenerator(const QString &dest);
    ~FunctionGenerator();

    void extract(const QString &data);

protected:
    QString typeToFetchFunction(const QString &arg, const QString &type, const GeneratorTypes::ArgStruct &argStruct);
    QString fetchFunction(const QString &clssName, const QString &fncName, const GeneratorTypes::ArgStruct &type);
    QString typeToPushFunction(const QString &arg, const QString &type, const GeneratorTypes::ArgStruct &argStruct, bool justFlagTest);
    QString pushFunction(const QString &clssName, const QString &fncName, const QList<GeneratorTypes::ArgStruct> &types);
    void writeTypeHeader(const QString &name, const QList<GeneratorTypes::FunctionStruct> &functions);
    void writeTypeClass(const QString &name, const QList<GeneratorTypes::FunctionStruct> &functions);
    void writeType(const QString &name, const QList<GeneratorTypes::FunctionStruct> &types);
    void writePri(const QStringList &types);
    void writeMainHeader(const QStringList &types);
    void copyEmbeds();

private:
    QString m_dst;
};

#endif // FUNCTIONGENERATOR_H
