#include "typegenerator.h"

#include <QStringList>
#include <QRegExp>
#include <QMap>
#include <QFile>
#include <QDir>
#include <QDebug>

TypeGenerator::TypeGenerator(const QString &dest) :
    m_dst(dest)
{

}

TypeGenerator::~TypeGenerator()
{

}

QString TypeGenerator::typeToFetchFunction(const QString &arg, const QString &type, const GeneratorTypes::ArgStruct &argStruct)
{
    QString result;
    QString baseResult;

    if(type == "qint32")
        baseResult += arg + " = in->fetchInt();";
    else
    if(type == "bool")
        baseResult += arg + " = in->fetchBool();";
    else
    if(type == "qint64")
        baseResult += arg + " = in->fetchLong();";
    else
    if(type == "qreal")
        baseResult += arg + " = in->fetchDouble();";
    else
    if(type == "QByteArray")
        baseResult += arg + " = in->fetchBytes();";
    else
    if(type == "QString")
        baseResult += arg + " = in->fetchQString();";
    else
    if(type.contains("QList<"))
    {
        QString innerType = type.mid(6,type.length()-7);
        baseResult += "if(in->fetchInt() != (qint32)CoreTypes::typeVector) return false;\n";

        baseResult += "qint32 " + arg + "_length = in->fetchInt();\n";
        baseResult += arg + ".clear();\n";
        baseResult += "for (qint32 i = 0; i < " + arg + "_length; i++) {\n";
        baseResult += "    " + innerType + " type;\n    " + typeToFetchFunction("type", innerType, argStruct) + "\n";
        baseResult += QString("    %1.append(type);\n}").arg(arg);
    }
    else
    {
        baseResult += arg + ".fetch(in);";
    }

    if(!argStruct.flagName.isEmpty())
    {
        result += QString("if(m_%1 & 1<<%2) ").arg(argStruct.flagName).arg(argStruct.flagValue);
        baseResult = "{\n" + shiftSpace(baseResult, 1) + "}";
    }

    result += baseResult;
    return result;
}

QString TypeGenerator::fetchFunction(const QString &name, const QList<GeneratorTypes::TypeStruct> &types)
{
    QString result = "LQTG_FETCH_LOG;\nint x = in->fetchInt();\nswitch(x) {\n";

    foreach(const GeneratorTypes::TypeStruct &t, types)
    {
        result += QString("case %1: {\n").arg(t.typeName);

        QString fetchPart;
        foreach(const GeneratorTypes::ArgStruct &arg, t.args)
        {
            if(arg.flagDedicated)
                continue;

            fetchPart += typeToFetchFunction("m_" + cammelCaseType(arg.argName), arg.type.name, arg) + "\n";
        }

        fetchPart += QString("m_classType = static_cast<%1Type>(x);\nreturn true;\n").arg(classCaseType(name));
        result += shiftSpace(fetchPart, 1);
        result += "}\n    break;\n\n";
    }

    result += "default:\n    LQTG_FETCH_ASSERT;\n    return false;\n}\n";
    return result;
}

QString TypeGenerator::typeToPushFunction(const QString &arg, const QString &type, const GeneratorTypes::ArgStruct &argStruct)
{
    QString result;
    if(type == "qint32")
        result += "out->appendInt(" + arg + ");";
    else
    if(type == "bool")
        result += "out->appendBool(" + arg + ");";
    else
    if(type == "qint64")
        result += "out->appendLong(" + arg + ");";
    else
    if(type == "qreal")
        result += "out->appendDouble(" + arg + ");";
    else
    if(type == "QByteArray")
        result += "out->appendBytes(" + arg + ");";
    else
    if(type == "QString")
        result += "out->appendQString(" + arg + ");";
    else
    if(type.contains("QList<"))
    {
        QString innerType = type.mid(6,type.length()-7);
        result += "out->appendInt(CoreTypes::typeVector);\n";

        result += "out->appendInt(" + arg + ".count());\n";
        result += "for (qint32 i = 0; i < " + arg + ".count(); i++) {\n";
        result += "    " + typeToPushFunction(arg + "[i]", innerType, argStruct) + "\n}";
    }
    else
    {
        result += arg + ".push(out);";
    }

    return result;
}

QString TypeGenerator::pushFunction(const QString &name, const QList<GeneratorTypes::TypeStruct> &types)
{
    Q_UNUSED(name)
    QString result = "out->appendInt(m_classType);\nswitch(m_classType) {\n";

    foreach(const GeneratorTypes::TypeStruct &t, types)
    {
        result += QString("case %1: {\n").arg(t.typeName);

        QString fetchPart;
        foreach(const GeneratorTypes::ArgStruct &arg, t.args)
        {
            if(arg.flagDedicated)
                continue;

            fetchPart += typeToPushFunction("m_" + cammelCaseType(arg.argName), arg.type.name, arg) + "\n";
        }

        fetchPart += "return true;\n";
        result += shiftSpace(fetchPart, 1);
        result += "}\n    break;\n\n";
    }

    result += "default:\n    return false;\n}\n";
    return result;
}

void TypeGenerator::writeTypeHeader(const QString &name, const QList<GeneratorTypes::TypeStruct> &types)
{
    const QString &clssName = classCaseType(name);

    QString result;
    result += QString("class LIBQTELEGRAMSHARED_EXPORT %1 : public TelegramTypeObject\n{\npublic:\n"
                      "    enum %1Type {\n").arg(clssName);

    QString defaultType;
    QMap<QString, QMap<QString,GeneratorTypes::ArgStruct> > properties;
    for(int i=0; i<types.count(); i++)
    {
        const GeneratorTypes::TypeStruct &t = types[i];
        if(defaultType.isEmpty())
            defaultType = t.typeName;
        else
            if(t.typeName.contains("Empty") || t.typeName.contains("Invalid"))
                defaultType = t.typeName;

        result += "        " + t.typeName + " = " + t.typeCode;
        if(i < types.count()-1)
            result += ",\n";
        else
            result += "\n    };\n\n";

        for(int j=0; j<t.args.length(); j++)
        {
            const GeneratorTypes::ArgStruct &arg = t.args[j];
            if(properties.contains(arg.argName) && properties[arg.argName].contains(arg.type.name))
                continue;

            properties[arg.argName][arg.type.name] = arg;
        }
    }

    result += QString("    %1(%1Type classType = %2, InboundPkt *in = 0);\n").arg(clssName, defaultType);
    result += QString("    %1(InboundPkt *in);\n    %1(const Null&);\n    virtual ~%1();\n\n").arg(clssName);

    QString privateResult = "private:\n";
    QString includes = "#include \"telegramtypeobject.h\"\n\n#include <QMetaType>\n";
    QSet<QString> addedIncludes;

    QMapIterator<QString, QMap<QString,GeneratorTypes::ArgStruct> > pi(properties);
    while(pi.hasNext())
    {
        pi.next();
        const QMap<QString,GeneratorTypes::ArgStruct> &hash = pi.value();
        QMapIterator<QString,GeneratorTypes::ArgStruct> mi(hash);
        while(mi.hasNext())
        {
            mi.next();
            const GeneratorTypes::ArgStruct &arg = mi.value();
            QString argName = arg.argName;
            if(properties[pi.key()].count() > 1 && arg.type.name.toLower() != arg.argName.toLower())
                argName = arg.argName + "_" + QString(arg.type.originalType).remove(classCaseType(arg.argName));

            const QString &cammelCase = cammelCaseType(argName);
            const QString &classCase = classCaseType(argName);
            const GeneratorTypes::QtTypeStruct &type = arg.type;
            const QString inputType = type.constRefrence? "const " + type.name + " &" : type.name + " ";

            result += QString("    void set%1(%2%3);\n").arg(classCase, inputType, cammelCase);
            result += QString("    %2 %1() const;\n\n").arg(cammelCase, type.name);

            foreach(const QString &inc, arg.type.includes)
            {
                if(addedIncludes.contains(inc))
                    continue;

                includes += inc + "\n";
                addedIncludes.insert(inc);
            }

            if(!arg.flagDedicated)
                privateResult += QString("    %1 m_%2;\n").arg(type.name, cammelCase);
        }
    }
    privateResult += QString("    %1Type m_classType;\n").arg(clssName);

    result += QString("    void setClassType(%1Type classType);\n    %1Type classType() const;\n\n").arg(clssName);
    result += "    bool fetch(InboundPkt *in);\n    bool push(OutboundPkt *out) const;\n\n";
    result += QString("    bool operator ==(const %1 &b) const;\n\n").arg(clssName);
    result += "    bool operator==(bool stt) const { return isNull() != stt; }\n"
              "    bool operator!=(bool stt) const { return !operator ==(stt); }\n\n";
    result += privateResult + QString("};\n\nQ_DECLARE_METATYPE(%1)\n\n").arg(clssName);

    result = includes + "\n" + result;
    result = QString("#ifndef LQTG_TYPE_%1\n#define LQTG_TYPE_%1\n\n").arg(clssName.toUpper()) + result;
    result += QString("#endif // LQTG_TYPE_%1\n").arg(clssName.toUpper());

    QFile file(m_dst + "/" + clssName.toLower() + ".h");
    if(!file.open(QFile::WriteOnly))
        return;

    file.write(GENERATOR_SIGNATURE("//"));
    file.write(result.toUtf8());
    file.close();
}

void TypeGenerator::writeTypeClass(const QString &name, const QList<GeneratorTypes::TypeStruct> &types)
{
    const QString &clssName = classCaseType(name);

    QString result;
    result += QString("#include \"%1.h\"\n").arg(clssName.toLower()) +
        "#include \"core/inboundpkt.h\"\n"
        "#include \"core/outboundpkt.h\"\n"
        "#include \"../coretypes.h\"\n\n";

    QString resultTypes;
    QString resultEqualOperator = "return m_classType == b.m_classType";

    QString defaultType;
    QMap<QString, QMap<QString,GeneratorTypes::ArgStruct> > properties;
    for(int i=0; i<types.count(); i++)
    {
        const GeneratorTypes::TypeStruct &t = types[i];
        if(defaultType.isEmpty())
            defaultType = t.typeName;
        else
            if(t.typeName.contains("Empty") || t.typeName.contains("Invalid"))
                defaultType = t.typeName;

        for(int j=0; j<t.args.length(); j++)
        {
            const GeneratorTypes::ArgStruct &arg = t.args[j];
            if(properties.contains(arg.argName) && properties[arg.argName].contains(arg.type.name))
                continue;

            properties[arg.argName][arg.type.name] = arg;
        }
    }

    QList<GeneratorTypes::TypeStruct> modifiedTypes = types;

    QString functions;
    QMapIterator<QString, QMap<QString,GeneratorTypes::ArgStruct> > pi(properties);
    while(pi.hasNext())
    {
        pi.next();
        const QMap<QString,GeneratorTypes::ArgStruct> &hash = pi.value();
        QMapIterator<QString,GeneratorTypes::ArgStruct> mi(hash);
        while(mi.hasNext())
        {
            mi.next();
            const GeneratorTypes::ArgStruct &arg = mi.value();
            QString argName = arg.argName;
            if(properties[pi.key()].count() > 1 && arg.type.name.toLower() != arg.argName.toLower())
            {
                argName = arg.argName + "_" + QString(arg.type.originalType).remove(classCaseType(arg.argName));

                GeneratorTypes::ArgStruct newArg = arg;
                newArg.argName = argName;

                for(int i=0; i<modifiedTypes.count(); i++)
                {
                    GeneratorTypes::TypeStruct &ts = modifiedTypes[i];
                    int idx = ts.args.indexOf(arg);
                    if(idx != -1)
                        ts.args[idx] = newArg;
                }
            }

            const QString &cammelCase = cammelCaseType(argName);
            const QString &classCase = classCaseType(argName);
            const GeneratorTypes::QtTypeStruct &type = arg.type;
            const QString inputType = type.constRefrence? "const " + type.name + " &" : type.name + " ";

            if(!arg.flagDedicated)
            {
                if(!type.defaultValue.isEmpty())
                    resultTypes += QString("    m_%1(%2),\n").arg(cammelCase, type.defaultValue);
                resultEqualOperator += QString(" &&\n       m_%1 == b.m_%1").arg(cammelCase);
            }

            if(arg.flagDedicated)
            {
                functions += QString("void %1::set%2(%3%4) {\n    if(%4) m_%5 = (m_%5 | (1<<%6));\n"
                                     "    else m_%5 = (m_%5 & ~(1<<%6));\n}\n\n").arg(clssName, classCase, inputType, cammelCase, arg.flagName).arg(arg.flagValue);
                functions += QString("%3 %1::%2() const {\n    return (m_%4 & 1<<%5);\n}\n\n").arg(clssName, cammelCase, type.name, arg.flagName).arg(arg.flagValue);
            }
            else
            {
                functions += QString("void %1::set%2(%3%4) {\n    m_%4 = %4;\n}\n\n").arg(clssName, classCase, inputType, cammelCase);
                functions += QString("%3 %1::%2() const {\n    return m_%2;\n}\n\n").arg(clssName, cammelCase, type.name);
            }
        }
    }
    resultEqualOperator += ";";

    result += QString("%1::%1(%1Type classType, InboundPkt *in) :\n").arg(clssName);
    result += resultTypes + QString("    m_classType(classType)\n");;
    result += "{\n    if(in) fetch(in);\n}\n\n";
    result += QString("%1::%1(InboundPkt *in) :\n").arg(clssName);
    result += resultTypes + QString("    m_classType(%1)\n").arg(defaultType);
    result += "{\n    fetch(in);\n}\n\n";
    result += QString("%1::%1(const Null &null) :\n    TelegramTypeObject(null),\n").arg(clssName);
    result += resultTypes + QString("    m_classType(%1)\n").arg(defaultType);
    result += "{\n}\n\n";
    result += QString("%1::~%1() {\n}\n\n").arg(clssName);
    result += functions;
    result += QString("bool %1::operator ==(const %1 &b) const {\n%2}\n\n").arg(clssName, shiftSpace(resultEqualOperator, 1));

    result += QString("void %1::setClassType(%1::%1Type classType) {\n    m_classType = classType;\n}\n\n").arg(clssName);
    result += QString("%1::%1Type %1::classType() const {\n    return m_classType;\n}\n\n").arg(clssName);
    result += QString("bool %1::fetch(InboundPkt *in) {\n%2}\n\n").arg(clssName, shiftSpace(fetchFunction(name, modifiedTypes), 1));
    result += QString("bool %1::push(OutboundPkt *out) const {\n%2}\n\n").arg(clssName, shiftSpace(pushFunction(name, modifiedTypes), 1));

    QFile file(m_dst + "/" + clssName.toLower() + ".cpp");
    if(!file.open(QFile::WriteOnly))
        return;

    file.write(GENERATOR_SIGNATURE("//"));
    file.write(result.toUtf8());
    file.close();
}

void TypeGenerator::writeType(const QString &name, const QList<GeneratorTypes::TypeStruct> &types)
{
    writeTypeHeader(name, types);
    writeTypeClass(name, types);
}

void TypeGenerator::writePri(const QStringList &types)
{
    QString result = "\n";
    QString headers = "HEADERS += \\\n    $$PWD/types.h \\\n    $$PWD/telegramtypeobject.h \\\n";
    QString sources = "SOURCES += \\\n    $$PWD/telegramtypeobject.cpp \\\n";
    for(int i=0; i<types.count(); i++)
    {
        const QString &t = classCaseType(types[i]);
        const bool last = (i == types.count()-1);

        headers += QString(last? "    $$PWD/%1.h\n" : "    $$PWD/%1.h \\\n").arg(t.toLower());
        sources += QString(last? "    $$PWD/%1.cpp\n" : "    $$PWD/%1.cpp \\\n").arg(t.toLower());
    }

    result += headers + "\n";
    result += sources + "\n";
//    result = result.replace("$$PWD","telegram/types");

    QFile file(m_dst + "/types.pri");
    if(!file.open(QFile::WriteOnly))
        return;

    file.write(GENERATOR_SIGNATURE("#"));
    file.write(result.toUtf8());
    file.close();
}

void TypeGenerator::writeMainHeader(const QStringList &types)
{
    QString result = "\n#include \"telegramtypeobject.h\"\n";
    for(int i=0; i<types.count(); i++)
    {
        const QString &t = classCaseType(types[i]);
        result += QString("#include \"%1.h\"\n").arg(t.toLower());
    }

    QFile file(m_dst + "/types.h");
    if(!file.open(QFile::WriteOnly))
        return;

    file.write(GENERATOR_SIGNATURE("//"));
    file.write(result.toUtf8());
    file.close();
}

void TypeGenerator::copyEmbeds()
{
    const QString &fcpp = m_dst + "/telegramtypeobject.cpp";
    QFile::remove(fcpp);
    QFile::copy(":/embeds/telegramtypeobject.cpp", fcpp);
    QFile(fcpp).setPermissions(QFileDevice::ReadUser|QFileDevice::WriteUser|
                                  QFileDevice::ReadGroup|QFileDevice::WriteGroup);

    const QString &fh = m_dst + "/telegramtypeobject.h";
    QFile::remove(fh);
    QFile::copy(":/embeds/telegramtypeobject.h", fh);
    QFile(fh).setPermissions(QFileDevice::ReadUser|QFileDevice::WriteUser|
                                  QFileDevice::ReadGroup|QFileDevice::WriteGroup);
}

void TypeGenerator::extract(const QString &data)
{
    QDir().mkpath(m_dst);

    QMap<QString, QList<GeneratorTypes::TypeStruct> > types = extractTypes(data, QString(), QString(), "types");
    QMapIterator<QString, QList<GeneratorTypes::TypeStruct> > i(types);
    while(i.hasNext())
    {
        i.next();
        const QString &name = i.key();
        const QList<GeneratorTypes::TypeStruct> &types = i.value();
        writeType(name, types);
    }

    writePri(types.keys());
    writeMainHeader(types.keys());
    copyEmbeds();
}

