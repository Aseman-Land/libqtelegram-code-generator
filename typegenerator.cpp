#include "typegenerator.h"

#include <QStringList>
#include <QRegExp>
#include <QMap>
#include <QFile>
#include <QDir>
#include <QDebug>

TypeGenerator::TypeGenerator(const QString &dest, bool inlineMode) :
    m_dst(dest),
    m_inlineMode(inlineMode)
{

}

TypeGenerator::~TypeGenerator()
{

}

QString TypeGenerator::typeToFetchFunction(const QString &arg, const QString &type, const GeneratorTypes::ArgStruct &argStruct, bool forcePointer)
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
        baseResult += "    " + innerType + " type;\n    " + typeToFetchFunction("type", innerType, argStruct, false) + "\n";
        baseResult += QString("    %1.append(type);\n}").arg(arg);
    }
    else
    {
        if(forcePointer)
            baseResult += QString("if(!%1) %1 = new %2();\n%1->fetch(in);").arg(arg, type);
        else
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
    QString result = "LQTG_FETCH_LOG;\nqint64 x = in->fetchInt();\nswitch(x) {\n";

    QSet<QString> addedCodes;
    foreach(const GeneratorTypes::TypeStruct &t, types)
    {
        if(addedCodes.contains(t.typeCode)) continue; else addedCodes.insert(t.typeCode);
        result += QString("case %1: {\n").arg(t.typeName);

        QString fetchPart;
        foreach(const GeneratorTypes::ArgStruct &arg, t.args)
        {
            if(arg.flagDedicated)
                continue;

            bool forcePointer = (name == arg.type.name);
            fetchPart += typeToFetchFunction("m_" + cammelCaseType(arg.argName), arg.type.name, arg, forcePointer) + "\n";
        }

        fetchPart += QString("m_classType = static_cast<%1ClassType>(x);\nreturn true;\n").arg(classCaseType(name));
        result += shiftSpace(fetchPart, 1);
        result += "}\n    break;\n\n";
    }

    result += "default:\n    LQTG_FETCH_ASSERT;\n    return false;\n}\n";
    return result;
}

QString TypeGenerator::typeToPushFunction(const QString &arg, const QString &type, const GeneratorTypes::ArgStruct &argStruct, bool forcePointer)
{
    QString result;
    QString baseResult;

    if(type == "qint32")
        baseResult += "out->appendInt(" + arg + ");";
    else
    if(type == "bool")
        baseResult += "out->appendBool(" + arg + ");";
    else
    if(type == "qint64")
        baseResult += "out->appendLong(" + arg + ");";
    else
    if(type == "qreal")
        baseResult += "out->appendDouble(" + arg + ");";
    else
    if(type == "QByteArray")
        baseResult += "out->appendBytes(" + arg + ");";
    else
    if(type == "QString")
        baseResult += "out->appendQString(" + arg + ");";
    else
    if(type.contains("QList<"))
    {
        QString innerType = type.mid(6,type.length()-7);
        baseResult += "out->appendInt(CoreTypes::typeVector);\n";

        GeneratorTypes::ArgStruct tmpArg = argStruct;
        tmpArg.flagName.clear();

        baseResult += "out->appendInt(" + arg + ".count());\n";
        baseResult += "for (qint32 i = 0; i < " + arg + ".count(); i++) {\n";
        baseResult += "    " + typeToPushFunction(arg + "[i]", innerType, tmpArg, false) + "\n}";
    }
    else
    {
        if(forcePointer)
            baseResult += QString("if(%1) %1->push(out);\nelse %2().push(out);").arg(arg, type);
        else
            baseResult += arg + ".push(out);";
    }

    if(!argStruct.flagName.isEmpty())
    {
        result += QString("if(m_%1 & 1<<%2) ").arg(argStruct.flagName).arg(argStruct.flagValue);
        baseResult = "{\n" + shiftSpace(baseResult, 1) + "}";
    }

    result += baseResult;
    return result;
}

QString TypeGenerator::pushFunction(const QString &name, const QList<GeneratorTypes::TypeStruct> &types)
{
    Q_UNUSED(name)
    QString result = "out->appendInt(m_classType);\nswitch(m_classType) {\n";

    QSet<QString> addedCodes;
    foreach(const GeneratorTypes::TypeStruct &t, types)
    {
        if(addedCodes.contains(t.typeCode)) continue; else addedCodes.insert(t.typeCode);
        result += QString("case %1: {\n").arg(t.typeName);

        QString fetchPart;
        foreach(const GeneratorTypes::ArgStruct &arg, t.args)
        {
            if(arg.flagDedicated)
                continue;

            bool forcePointer = (name == arg.type.name);
            fetchPart += typeToPushFunction("m_" + cammelCaseType(arg.argName), arg.type.name, arg, forcePointer) + "\n";
        }

        fetchPart += "return true;\n";
        result += shiftSpace(fetchPart, 1);
        result += "}\n    break;\n\n";
    }

    result += "default:\n    return false;\n}\n";
    return result;
}

QString TypeGenerator::streamReadFunction(const QString &name, const QList<GeneratorTypes::TypeStruct> &types)
{
    QString result = QString("uint type = 0;\nstream >> type;\n"
                             "item.setClassType(static_cast<%1::%1ClassType>(type));\n"
                             "switch(type) {\n").arg(name);

    QSet<QString> addedCodes;
    foreach(const GeneratorTypes::TypeStruct &t, types)
    {
        if(addedCodes.contains(t.typeCode)) continue; else addedCodes.insert(t.typeCode);
        result += QString("case %2::%1: {\n").arg(t.typeName, name);

        QString fetchPart;
        foreach(const GeneratorTypes::ArgStruct &arg, t.args)
        {
            if(arg.flagDedicated)
                continue;

            fetchPart += QString("%1 m_%2;\nstream >> m_%2;\n"
                                 "item.set%3(m_%2);\n").arg(arg.type.name, arg.argName, classCaseType(arg.argName));
        }

        result += shiftSpace(fetchPart, 1);
        result += "}\n    break;\n";
    }

    result += "}\nreturn stream;\n";
    return result;
}

QString TypeGenerator::streamWriteFunction(const QString &name, const QList<GeneratorTypes::TypeStruct> &types)
{
    Q_UNUSED(name)
    QString result = "stream << static_cast<uint>(item.classType());\nswitch(item.classType()) {\n";

    QSet<QString> addedCodes;
    foreach(const GeneratorTypes::TypeStruct &t, types)
    {
        if(addedCodes.contains(t.typeCode)) continue; else addedCodes.insert(t.typeCode);
        result += QString("case %1::%2:\n").arg(name, t.typeName);

        QString fetchPart;
        foreach(const GeneratorTypes::ArgStruct &arg, t.args)
        {
            if(arg.flagDedicated)
                continue;

            fetchPart += QString("stream << item.%1();\n").arg(cammelCaseType(arg.argName));
        }

        result += shiftSpace(fetchPart, 1);
        result += "    break;\n";
    }

    result += "}\nreturn stream;\n";
    return result;
}

QString TypeGenerator::debugFunction(const QString &name, const QList<GeneratorTypes::TypeStruct> &types)
{
    QString result = "QDebugStateSaver saver(debug);\nQ_UNUSED(saver)\n"
                     "debug.nospace() << \"Telegram." + name + "(\";\n"
                     "switch(item.classType()) {\n";

    QSet<QString> addedCodes;
    foreach(const GeneratorTypes::TypeStruct &t, types)
    {
        if(addedCodes.contains(t.typeCode)) continue; else addedCodes.insert(t.typeCode);
        result += QString("case %1::%2:\n").arg(name, t.typeName);

        QString fetchPart;
        fetchPart += "debug.nospace() << \"classType: " + t.typeName + "\";\n";
        foreach(const GeneratorTypes::ArgStruct &arg, t.args)
        {
            if(arg.flagDedicated)
                continue;

            fetchPart += QString("debug.nospace() << \", %1: \" << item.%1();\n").arg(cammelCaseType(arg.argName));
        }

        result += shiftSpace(fetchPart, 1);
        result += "    break;\n";
    }

    result += "}\ndebug.nospace() << \")\";\nreturn debug;\n";
    return result;
}

QString TypeGenerator::typeMapReadFunction(const QString &arg, const QString &type, const QString &prepend, const GeneratorTypes::ArgStruct &argStruct, bool forcePointer)
{
    Q_UNUSED(forcePointer)
    QString result;
    QString baseResult;

    const bool isList = argStruct.type.isList;

    if(type == "qint32" ||
       type == "qint64" ||
       type == "QString" ||
       type == "bool" ||
       type == "qreal" ||
       type == "QByteArray")
    {
        if(isList)
            baseResult += QString("%1.value<%2>();").arg(prepend, type);
        else
            baseResult += QString("QVariant _%4_var = %2;\n"
                                  "if( !_%4_var.isNull() ) {\n"
                                  "    _%4_var.convert( QVariant::nameToType(\"%3\") );\n"
                                  "    result.set%1( _%4_var.value<%3>() );\n"
                                  "}\n").arg(classCaseType(arg), prepend, type, arg);
    }
    else
    if(type.contains("QList<"))
    {
        QString innerType = type.mid(6,type.length()-7);
        baseResult += QString("QList<QVariant> map_%1 = map[\"%1\"].toList();\n").arg(arg);

        baseResult += QString("%1 _%2;\n").arg(type, arg);
        baseResult += QString("for(const QVariant &var: map_%1)\n").arg(arg);
        baseResult += QString("    _%1 << ").arg(arg) + typeMapReadFunction("_type", innerType, "var", argStruct, false) + ";\n";
        baseResult += QString("result.set%1(_%2);").arg(classCaseType(arg), arg);
    }
    else
    {
        if(isList)
            baseResult += QString("%1::fromMap(%2.toMap())").arg(type, prepend);
        else
            baseResult += QString("QVariant _%4_var = %3;\n"
                                  "if( !_%4_var.isNull() )\n"
                                  "    result.set%1( %2::fromMap(_%4_var.toMap()) );\n").arg(classCaseType(arg), type, prepend, arg);
    }

    result += baseResult;
    return result;
}

QString TypeGenerator::mapReadFunction(const QString &name, const QList<GeneratorTypes::TypeStruct> &types)
{
    QString result = QString("%1 result;\n").arg(classCaseType(name));

    foreach(const GeneratorTypes::TypeStruct &t, types)
    {
        result += QString("if(map.value(\"classType\").toString() == \"%1::%2\") {\n").arg(classCaseType(name), t.typeName);
        result += QString("    result.setClassType(%1);\n").arg(t.typeName);

        QString fetchPart;
        foreach(const GeneratorTypes::ArgStruct &arg, t.args)
        {
            if(arg.isFlag)
                continue;

            const QString &argName = cammelCaseType(arg.argName);

            bool forcePointer = (name == arg.type.name);
            fetchPart += typeMapReadFunction(argName, arg.type.name, QString("map.value(\"%1\")").arg(argName), arg, forcePointer) + "\n";
        }

        fetchPart += QString("return result;\n");
        result += shiftSpace(fetchPart, 1);
        result += "}\n";
    }

    result += "return result;\n";
    return result;
}

QString TypeGenerator::typeMapWriteFunction(const QString &arg, const QString &type, const QString &prepend, const GeneratorTypes::ArgStruct &argStruct, bool forcePointer)
{
    QString result;
    QString baseResult;

    const bool isList = argStruct.type.isList;

    if(type == "qint32" ||
       type == "qint64" ||
       type == "QString" ||
       type == "bool" ||
       type == "qreal" ||
       type == "QByteArray")
    {
        if(isList)
            baseResult += QString("%1 QVariant::fromValue<%3>(m_%2);").arg(prepend, arg, type);
        else {
            if(type == "qint32" ||
               type == "qint64" ||
               type == "bool" ||
               type == "qreal")
                baseResult += QString("if( %2() ) %1 QString::number(%2());").arg(prepend, arg);
            else
                baseResult += QString("if( !m_%2.isEmpty() ) %1 QVariant::fromValue<%3>(m_%2);").arg(prepend, arg, type);
        }
    }
    else
    if(type.contains("QList<"))
    {
        QString innerType = type.mid(6,type.length()-7);
        baseResult += QString("QList<QVariant> _%1;\n").arg(arg);

        baseResult += QString("for(const %1 &m__type: m_%2)\n").arg(innerType, arg);
        baseResult += "    " + typeMapWriteFunction("_type", innerType, QString("_%1 <<").arg(arg), argStruct, false) + "\n";
        baseResult += prepend + QString(" _%1;").arg(arg);;
    }
    else
    {
        if(forcePointer)
            baseResult += prepend + QString(" (m_%1 ? *m_%1 : %2()).toMap();").arg(arg, type);
        else
            baseResult += QString("if( !m_%2.isNull() ) %1 m_%2.toMap();").arg(prepend, arg);
    }

    result += baseResult;
    return result;
}

QString TypeGenerator::mapWriteFunction(const QString &name, const QList<GeneratorTypes::TypeStruct> &types)
{
    QString result = "QMap<QString, QVariant> result;\nswitch(static_cast<qint64>(m_classType)) {\n";

    QSet<QString> addedCodes;
    foreach(const GeneratorTypes::TypeStruct &t, types)
    {
        if(addedCodes.contains(t.typeCode)) continue; else addedCodes.insert(t.typeCode);
        result += QString("case %1: {\n").arg(t.typeName);
        result += QString("    result[\"classType\"] = \"%1::%2\";\n").arg(classCaseType(name), t.typeName);

        QString fetchPart;
        foreach(const GeneratorTypes::ArgStruct &arg, t.args)
        {
            if(arg.isFlag)
                continue;

            const QString &argName = cammelCaseType(arg.argName);

            bool forcePointer = (name == arg.type.name);
            fetchPart += typeMapWriteFunction(argName, arg.type.name, QString("result[\"%1\"] =").arg(argName), arg, forcePointer) + "\n";
        }

        fetchPart += QString("return result;\n");
        result += shiftSpace(fetchPart, 1);
        result += "}\n    break;\n\n";
    }

    result += "default:\n    return result;\n}\n";
    return result;
}

void TypeGenerator::writeTypeHeader(const QString &name, const QList<GeneratorTypes::TypeStruct> &types)
{
    const QString &clssName = classCaseType(name);

    QString result;
    result += QString("class LIBQTELEGRAMSHARED_EXPORT %1 : public TelegramTypeObject\n{\npublic:\n"
                      "    enum %1ClassType {\n").arg(clssName);

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

    result += QString("    %1(%1ClassType classType = %2, InboundPkt *in = 0);\n").arg(clssName, defaultType);
    result += QString("    %1(InboundPkt *in);\n    %1(const Null&);\n"
                      "    %1(const %1 &another);\n    virtual ~%1();\n\n").arg(clssName);

    QString privateResult = "private:\n";
    QString includes = "#include \"telegramtypeobject.h\"\n\n#include <QMetaType>\n#include <QVariant>\n";
    includes +=
            "#include \"../coretypes.h\"\n\n"
            "#include <QDataStream>\n\n";

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
                argName = arg.argName + "_" + QString(arg.type.originalType).remove(classCaseType(arg.argName)).remove("<").remove(">");

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
            {
                if(type.name == clssName)
                    privateResult += QString("    %1 *m_%2;\n").arg(type.name, cammelCase);
                else
                    privateResult += QString("    %1 m_%2;\n").arg(type.name, cammelCase);
            }
        }
    }
    privateResult += QString("    %1ClassType m_classType;\n").arg(clssName);

    result += QString("    void setClassType(%1ClassType classType);\n    %1ClassType classType() const;\n\n").arg(clssName);
    result += "    bool fetch(InboundPkt *in);\n    bool push(OutboundPkt *out) const;\n\n";
    result += QString("    QMap<QString, QVariant> toMap() const;\n    static %1 fromMap(const QMap<QString, QVariant> &map);\n"
                      "    static %1 fromJson(const QString &json);\n\n").arg(clssName);
    result += QString("    bool operator ==(const %1 &b) const;\n"
                      "    %1 &operator =(const %1 &b);\n\n").arg(clssName);
    result += "    bool operator==(bool stt) const { return isNull() != stt; }\n"
              "    bool operator!=(bool stt) const { return !operator ==(stt); }\n\n";
    result += "    QByteArray getHash(QCryptographicHash::Algorithm alg = QCryptographicHash::Md5) const;\n\n";

    result += privateResult + QString("};\n\nQ_DECLARE_METATYPE(%1)\n\n").arg(clssName);

    result = includes + "\n" + result;
    result += QString("QDataStream LIBQTELEGRAMSHARED_EXPORT &operator<<(QDataStream &stream, const %1 &item);\n"
                      "QDataStream LIBQTELEGRAMSHARED_EXPORT &operator>>(QDataStream &stream, %1 &item);\n\n").arg(clssName);

    result += QString("QDebug LIBQTELEGRAMSHARED_EXPORT operator<<(QDebug debug,  const %1 &item);\n\n").arg(clssName);

    if(m_inlineMode)
        result += writeTypeClass(name, types) + "\n";

    result = QString("#ifndef LQTG_TYPE_%1\n#define LQTG_TYPE_%1\n\n").arg(clssName.toUpper()) + result;
    result += QString("#endif // LQTG_TYPE_%1\n").arg(clssName.toUpper());

    QFile file(m_dst + "/" + clssName.toLower() + ".h");
    if(!file.open(QFile::WriteOnly))
        return;

    file.write(GENERATOR_SIGNATURE("//"));
    file.write(result.toUtf8());
    file.close();
}

QString TypeGenerator::writeTypeClass(const QString &name, const QList<GeneratorTypes::TypeStruct> &types)
{
    const QString &clssName = classCaseType(name);

    QString preFnc;
    if(m_inlineMode)
        preFnc = "inline ";

    QString result;
    QString classResult;
    QString deconstructor;

    QString headers;
    headers += QString("#include \"%1.h\"\n").arg(clssName.toLower()) +
        "#include \"core/inboundpkt.h\"\n"
        "#include \"core/outboundpkt.h\"\n"
        "#include \"../coretypes.h\"\n\n"
        "#include <QDataStream>\n\n";

    QString resultTypes;
    QString resultEqualTestOperator = "return m_classType == b.m_classType";
    QString resultAssignOperator = "m_classType = b.m_classType;\n";

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
                argName = arg.argName + "_" + QString(arg.type.originalType).remove(classCaseType(arg.argName)).remove("<").remove(">");

                for(int i=0; i<modifiedTypes.count(); i++)
                {
                    GeneratorTypes::TypeStruct &ts = modifiedTypes[i];
                    for(int i=0; i<ts.args.count(); i++)
                    {
                        GeneratorTypes::ArgStruct &secArg = ts.args[i];
                        if(secArg.argName == arg.argName &&
                           secArg.type.name == arg.type.name)
                        {
                            secArg.argName = argName;
                        }
                    }
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

                if(type.name == clssName)
                {
                    resultTypes += QString("    m_%1(0),\n").arg(cammelCase);
                    resultAssignOperator += QString("set%2(b.m_%1 ? *b.m_%1 : %3(%3::null));\n").arg(cammelCase, classCase, clssName);
                }
                else
                {
                    resultAssignOperator += QString("m_%1 = b.m_%1;\n").arg(cammelCase);
                }

                resultEqualTestOperator += QString(" &&\n       m_%1 == b.m_%1").arg(cammelCase);
            }

            if(arg.flagDedicated)
            {
                functions += preFnc + QString("void %1::set%2(%3%4) {\n    if(%4) m_%5 = (m_%5 | (1<<%6));\n"
                                     "    else m_%5 = (m_%5 & ~(1<<%6));\n}\n\n").arg(clssName, classCase, inputType, cammelCase, arg.flagName).arg(arg.flagValue);
                functions += preFnc + QString("%3 %1::%2() const {\n    return (m_%4 & 1<<%5);\n}\n\n").arg(clssName, cammelCase, type.name, arg.flagName).arg(arg.flagValue);
            }
            else
            {
                QString flagPart;
                if(!arg.flagName.isEmpty())
                {
                    if(inputType.contains("QList"))
                        flagPart = QString("\n    if(%1.length()) m_%2 = (m_%2 | (1<<%3));\n"
                                           "    else m_%2 = (m_%2 & ~(1<<%3));").arg(cammelCase, arg.flagName).arg(arg.flagValue);
                    else
                    if(arg.type.qtgType || arg.type.constRefrence)
                        flagPart = QString("\n    if(!%1.isNull()) m_%2 = (m_%2 | (1<<%3));\n"
                                           "    else m_%2 = (m_%2 & ~(1<<%3));").arg(cammelCase, arg.flagName).arg(arg.flagValue);
                    else
                        flagPart = QString("\n    if(%1) m_%2 = (m_%2 | (1<<%3));\n"
                                           "    else m_%2 = (m_%2 & ~(1<<%3));").arg(cammelCase, arg.flagName).arg(arg.flagValue);
                }

                QString readInner = "%3 %1::%2() const {\n    return m_%2;\n}\n\n";
                QString setInner = "void %1::set%2(%3%4) {" + flagPart + "\n    m_%4 = %4;\n}\n\n";
                if(type.name == clssName)
                {
                    deconstructor += QString("if(m_%1) delete m_%1;\n").arg(cammelCase);
                    readInner = "%3 %1::%2() const {\n    return m_%2 ? *m_%2 : %3(%3::null);\n}\n\n";

                    QString equalCheck = "    if(%4.isNull()) {\n        if(m_%4) delete m_%4;\n"
                                "        m_%4 = 0;\n        return;\n    }";
                    setInner = "void %1::set%2(%3%4) {\n" + equalCheck + "\n    if(!m_%4) m_%4 = new " + type.name + "();" + flagPart + "\n    *m_%4 = %4;\n}\n\n";
                }
                functions += preFnc + QString(setInner).arg(clssName, classCase, inputType, cammelCase);
                functions += preFnc + QString(readInner).arg(clssName, cammelCase, type.name);
            }
        }
    }
    resultEqualTestOperator += ";";
    resultAssignOperator += "setNull(b.isNull());\n";
    resultAssignOperator = "if(&b != this) {\n" + shiftSpace(resultAssignOperator, 1) + "}\nreturn *this;\n";

    classResult += preFnc + QString("%1::%1(%1ClassType classType, InboundPkt *in) :\n").arg(clssName);
    classResult += resultTypes + QString("    m_classType(classType)\n");;
    classResult += "{\n    if(in) fetch(in);\n}\n\n";
    classResult += preFnc + QString("%1::%1(InboundPkt *in) :\n").arg(clssName);
    classResult += resultTypes + QString("    m_classType(%1)\n").arg(defaultType);
    classResult += "{\n    fetch(in);\n}\n\n";
    classResult += preFnc + QString("%1::%1(const %1 &another) :\n    TelegramTypeObject(),\n").arg(clssName);
    classResult += resultTypes + QString("    m_classType(%1)\n").arg(defaultType);
    classResult += "{\n    operator=(another);\n}\n\n";
    classResult += preFnc + QString("%1::%1(const Null &null) :\n    TelegramTypeObject(null),\n").arg(clssName);
    classResult += resultTypes + QString("    m_classType(%1)\n").arg(defaultType);
    classResult += "{\n}\n\n";
    classResult += preFnc + QString("%1::~%1() {\n%2}\n\n").arg(clssName, shiftSpace(deconstructor, 1));
    classResult += functions;
    classResult += preFnc + QString("bool %1::operator ==(const %1 &b) const {\n%2}\n\n").arg(clssName, shiftSpace(resultEqualTestOperator, 1));
    classResult += preFnc + QString("%1 &%1::operator =(const %1 &b) {\n%2}\n\n").arg(clssName, shiftSpace(resultAssignOperator, 1));

    classResult += preFnc + QString("void %1::setClassType(%1::%1ClassType classType) {\n    m_classType = classType;\n}\n\n").arg(clssName);
    classResult += preFnc + QString("%1::%1ClassType %1::classType() const {\n    return m_classType;\n}\n\n").arg(clssName);
    classResult += preFnc + QString("bool %1::fetch(InboundPkt *in) {\n%2}\n\n").arg(clssName, shiftSpace(fetchFunction(name, modifiedTypes), 1));
    classResult += preFnc + QString("bool %1::push(OutboundPkt *out) const {\n%2}\n\n").arg(clssName, shiftSpace(pushFunction(name, modifiedTypes), 1));
    classResult += preFnc + QString("QMap<QString, QVariant> %1::toMap() const {\n%2}\n\n").arg(clssName, shiftSpace(mapWriteFunction(name, modifiedTypes), 1));
    classResult += preFnc + QString("%1 %1::fromMap(const QMap<QString, QVariant> &map) {\n%2}\n\n").arg(clssName, shiftSpace(mapReadFunction(name, modifiedTypes), 1));
    classResult += preFnc + QString("%1 %1::fromJson(const QString &json) {\n    return %1::fromMap(QJsonDocument::fromJson(json.toUtf8()).toVariant().toMap());\n}\n\n").arg(clssName);
    classResult +=preFnc +  QString("QByteArray %1::getHash(QCryptographicHash::Algorithm alg) const {\n"
                                    "    QByteArray data;\n    QDataStream str(&data, QIODevice::WriteOnly);\n"
                                    "    str << *this;\n    return QCryptographicHash::hash(data, alg);\n}\n\n").arg(clssName);
    if(!m_inlineMode)
    {
        result += headers;
    }
    result += classResult;
    result += preFnc + QString("QDataStream &operator<<(QDataStream &stream, const %1 &item) {\n%2}\n\n").arg(clssName, shiftSpace(streamWriteFunction(clssName, modifiedTypes), 1));
    result += preFnc + QString("QDataStream &operator>>(QDataStream &stream, %1 &item) {\n%2}\n\n").arg(clssName, shiftSpace(streamReadFunction(clssName, modifiedTypes), 1));

    result += preFnc + QString("QDebug operator<<(QDebug debug,  const %1 &item) {\n%2}\n\n").arg(clssName, shiftSpace(debugFunction(clssName, modifiedTypes), 1) );

    if(m_inlineMode)
        QFile::remove(m_dst + "/" + clssName.toLower() + ".cpp");
    else
    {
        QFile file(m_dst + "/" + clssName.toLower() + ".cpp");
        if(file.open(QFile::WriteOnly))
        {
            file.write(GENERATOR_SIGNATURE("//"));
            file.write(result.toUtf8());
            file.close();
        }
    }

    return result;
}

void TypeGenerator::writeType(const QString &name, const QList<GeneratorTypes::TypeStruct> &types)
{
    writeTypeHeader(name, types);
    if(!m_inlineMode)
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
        if(!m_inlineMode)
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

