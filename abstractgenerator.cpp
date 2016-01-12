#include "abstractgenerator.h"

QString AbstractGenerator::shiftSpace(const QString &str, int level)
{
    QString space;
    for(int i=0; i<level; i++)
        space += "    ";

    return space + QString(str).replace("\n", "\n" + space).trimmed() + "\n";
}

QString AbstractGenerator::fixDeniedNames(const QString &str)
{
    if(str == "long")
        return "longValue";
    else
    if(str == "private")
        return "privateValue";
    else
    if(str == "public")
        return "publicValue";
    else
        return str;
}

QString AbstractGenerator::fixTypeName(const QString &str)
{
    QString result = str;
    for(int i=0; i<result.length(); i++)
    {
        QChar ch = result[i];
        if(ch == '.')
        {
            result.remove(i,1);
            result[i] = result[i].toUpper();
        }
    }

    return result;
}

QString AbstractGenerator::cammelCaseType(const QString &str)
{
    QString result = fixTypeName(str);
    for(int i=0; i<result.length(); i++)
    {
        QChar ch = result[i];
        if(ch == '_')
        {
            result.remove(i,1);
            result[i] = result[i].toUpper();
        }
    }

    return result;
}

QString AbstractGenerator::classCaseType(const QString &str)
{
    QString result = cammelCaseType(str);
    if(!result.isEmpty())
        result[0] = result[0].toUpper();

    return result;
}

GeneratorTypes::QtTypeStruct AbstractGenerator::translateType(const QString &type, bool addNameSpace, const QString &prePath, const QString &postPath)
{
    GeneratorTypes::QtTypeStruct result;
    result.originalType = type;

    if(type == "true")
    {
        result.name = "bool";
        result.defaultValue = "false";
    }
    else
    if(type == "int")
    {
        result.name = "qint32";
        result.includes << "#include <QtGlobal>";
        result.defaultValue = "0";
    }
    else
    if(type == "Bool")
    {
        result.name = "bool";
        result.defaultValue = "false";
    }
    else
    if(type == "long")
    {
        result.name = "qint64";
        result.defaultValue = "0";
        result.includes << "#include <QtGlobal>";
    }
    else
    if(type == "double")
    {
        result.name = "qreal";
        result.defaultValue = "0";
        result.includes << "#include <QtGlobal>";
    }
    else
    if(type == "string")
    {
        result.name = "QString";
        result.includes << "#include <QString>";
        result.constRefrence = true;
    }
    else
    if(type == "bytes")
    {
        result.name = "QByteArray";
        result.includes << "#include <QByteArray>";
        result.constRefrence = true;
    }
    else
    if(type.contains("Vector<"))
    {
        GeneratorTypes::QtTypeStruct innerType = translateType(type.mid(7,type.length()-8), addNameSpace, prePath);

        result.includes << "#include <QList>";
        result.includes << innerType.includes;
        result.name = QString("QList<%1>").arg(innerType.name);
        result.constRefrence = true;
        result.isList = true;
    }
    else
    {
        result.name = QString(addNameSpace? "Types::" : "") + classCaseType(type);
        result.includes << QString("#include \"%1\"").arg(prePath + classCaseType(type).toLower() + postPath + ".h");
        result.constRefrence = true;
        result.qtgType = true;
    }

    return result;
}

