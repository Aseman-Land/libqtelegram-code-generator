#ifndef LQTGTYPEGENERATOR_H
#define LQTGTYPEGENERATOR_H

#include "abstractgenerator.h"

#include <QStringList>
#include <QRegExp>
#include <QMap>
#include <QSet>

class LqtgTypeGenerator : public AbstractGenerator
{
public:
    LqtgTypeGenerator(const QString &dest);
    ~LqtgTypeGenerator();

    void extract(const QString &data);

protected:
    void writeTypeHeader(const QString &name, const QList<GeneratorTypes::TypeStruct> &types);
    void writeTypeClass(const QString &name, const QList<GeneratorTypes::TypeStruct> &types);
    void writeType(const QString &name, const QList<GeneratorTypes::TypeStruct> &types);
    void writePri(const QStringList &types);
    void writeMainHeader(const QStringList &types);
    void copyEmbeds();

private:
    QString m_dst;
};

#endif // LQTGTYPEGENERATOR_H
