#define SCHEMA_PATH "/home/bardia/scheme-29"
#define DEST_PATH "/home/bardia/Projects/Aseman/Tools/libqtelegram-ae/types/"

#include <QFile>
#include <QStringList>
#include <QRegExp>
#include <QMap>
#include <QDir>
#include <QDebug>

class QtTypeStruct
{
public:
    QtTypeStruct(): constRefrence(false){}
    QString name;
    QStringList includes;
    bool constRefrence;
    QString defaultValue;
    QString originalType;

    bool operator ==(const QtTypeStruct &b) {
        return name == b.name &&
               includes == b.includes &&
               constRefrence == b.constRefrence &&
               defaultValue == b.defaultValue &&
               originalType == b.originalType;
    }
};

class ArgStruct
{
public:
    ArgStruct() : flagValue(0) {}
    QString flagName;
    int flagValue;
    QtTypeStruct type;
    QString argName;

    bool operator ==(const ArgStruct &b) {
        return flagName == b.flagName &&
               type == b.type &&
               flagValue == b.flagValue &&
               argName == b.argName;
    }
};

class TypeStruct
{
public:
    QString typeName;
    QString typeCode;
    QList<ArgStruct> args;
};

QString shiftSpace(const QString &str, int level)
{
    QString space;
    for(int i=0; i<level; i++)
        space += "    ";

    return space + QString(str).replace("\n", "\n" + space).trimmed() + "\n";
}

QString fixDeniedNames(const QString &str)
{
    if(str == "long")
        return "longValue";
    else
        return str;
}

QString fixTypeName(const QString &str)
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

QString cammelCaseType(const QString &str)
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

QString classCaseType(const QString &str)
{
    QString result = cammelCaseType(str);
    if(!result.isEmpty())
        result[0] = result[0].toUpper();

    return result;
}

QtTypeStruct translateType(const QString &type)
{
    QtTypeStruct result;
    result.originalType = type;

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
        QtTypeStruct innerType = translateType(type.mid(7,type.length()-8));

        result.includes << "#include <QList>";
        result.includes << innerType.includes;
        result.name = QString("QList<%1>").arg(innerType.name);
        result.constRefrence = true;
    }
    else
    {
        result.name = classCaseType(type);
        result.includes << QString("#include \"%1\"").arg(result.name .toLower() + ".h");
        result.constRefrence = true;
    }

    return result;
}

QString typeToFetchFunction(const QString &arg, const QString &type, const ArgStruct &argStruct)
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
        baseResult += "if(in->fetchInt() != (qint32)TL_Vector) return false;\n";

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

QString fetchFunction(const QString &name, const QList<TypeStruct> &types)
{
    Q_UNUSED(name)
    QString result = "int x = in->fetchInt();\nswitch(x) {\n";

    foreach(const TypeStruct &t, types)
    {
        result += QString("case %1: {\n").arg(t.typeName);

        QString fetchPart;
        foreach(const ArgStruct &arg, t.args)
        {
            fetchPart += typeToFetchFunction("m_" + cammelCaseType(arg.argName), arg.type.name, arg) + "\n";
        }

        fetchPart += "return true;\n";
        result += shiftSpace(fetchPart, 1);
        result += "}\n    break;\n\n";
    }

    result += "default:\n    LQTG_FETCH_ASSERT;\n    return false;\n}\n";
    return result;
}

QString typeToPushFunction(const QString &arg, const QString &type, const ArgStruct &argStruct)
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
        result += "out->appendInt(TL_Vector);\n";

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

QString pushFunction(const QString &name, const QList<TypeStruct> &types)
{
    Q_UNUSED(name)
    QString result = "out->appendInt(m_classType);\nswitch(m_classType) {\n";

    foreach(const TypeStruct &t, types)
    {
        result += QString("case %1: {\n").arg(t.typeName);

        QString fetchPart;
        foreach(const ArgStruct &arg, t.args)
        {
            fetchPart += typeToPushFunction("m_" + cammelCaseType(arg.argName), arg.type.name, arg) + "\n";
        }

        fetchPart += "return true;\n";
        result += shiftSpace(fetchPart, 1);
        result += "}\n    break;\n\n";
    }

    result += "default:\n    return false;\n}\n";
    return result;
}

void writeTypeHeader(const QString &name, const QList<TypeStruct> &types)
{
    const QString &clssName = classCaseType(name);

    QString result;
    result += QString("class %1 : public TelegramTypeObject\n{\npublic:\n"
                      "    enum %1Type {\n").arg(clssName);

    QString defaultType;
    QMap<QString, QMap<QString,ArgStruct> > properties;
    for(int i=0; i<types.count(); i++)
    {
        const TypeStruct &t = types[i];
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
            const ArgStruct &arg = t.args[j];
            if(properties.contains(arg.argName) && properties[arg.argName].contains(arg.type.name))
                continue;

            properties[arg.argName][arg.type.name] = arg;
        }
    }

    result += QString("    %1(%1Type classType = %2, InboundPkt *in = 0);\n").arg(clssName, defaultType);
    result += QString("    %1(InboundPkt *in);\n    virtual ~%1();\n\n").arg(clssName);

    QString privateResult = "private:\n";
    QString includes = "#include \"telegramtypeobject.h\"\n";
    QSet<QString> addedIncludes;

    QMapIterator<QString, QMap<QString,ArgStruct> > pi(properties);
    while(pi.hasNext())
    {
        pi.next();
        const QMap<QString,ArgStruct> &hash = pi.value();
        QMapIterator<QString,ArgStruct> mi(hash);
        while(mi.hasNext())
        {
            mi.next();
            const ArgStruct &arg = mi.value();
            QString argName = arg.argName;
            if(properties[pi.key()].count() > 1 && arg.type.name.toLower() != arg.argName.toLower())
                argName = arg.argName + "_" + QString(arg.type.originalType).remove(classCaseType(arg.argName));

            const QString &cammelCase = cammelCaseType(argName);
            const QString &classCase = classCaseType(argName);
            const QtTypeStruct &type = arg.type;
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

            privateResult += QString("    %1 m_%2;\n").arg(type.name, cammelCase);
        }
    }
    privateResult += QString("    %1Type m_classType;\n").arg(clssName);

    result += QString("    void setClassType(%1Type classType);\n    %1Type classType() const;\n\n").arg(clssName);
    result += "    bool fetch(InboundPkt *in);\n    bool push(OutboundPkt *out) const;\n\n";
    result += QString("    bool operator ==(const %1 &b);\n\n").arg(clssName);
    result += privateResult + "};\n\n";

    result = includes + "\n" + result;
    result = QString("#ifndef LQTG_%1\n#define LQTG_%1\n\n").arg(clssName.toUpper()) + result;
    result += QString("#endif // LQTG_%1\n").arg(clssName.toUpper());

    QFile file(DEST_PATH "/" + clssName.toLower() + ".h");
    if(!file.open(QFile::WriteOnly))
        return;

    file.write(result.toUtf8());
    file.close();
}

void writeTypeClass(const QString &name, const QList<TypeStruct> &types)
{
    const QString &clssName = classCaseType(name);

    QString result;
    result += QString("#include \"%1.h\"\n").arg(clssName.toLower()) +
            "#include \"core/inboundpkt.h\"\n"
            "#include \"core/outboundpkt.h\"\n\n";

    QString resultTypes;
    QString resultEqualOperator;

    QString defaultType;
    QMap<QString, QMap<QString,ArgStruct> > properties;
    for(int i=0; i<types.count(); i++)
    {
        const TypeStruct &t = types[i];
        if(defaultType.isEmpty())
            defaultType = t.typeName;
        else
        if(t.typeName.contains("Empty") || t.typeName.contains("Invalid"))
            defaultType = t.typeName;

        for(int j=0; j<t.args.length(); j++)
        {
            const ArgStruct &arg = t.args[j];
            if(properties.contains(arg.argName) && properties[arg.argName].contains(arg.type.name))
                continue;

            properties[arg.argName][arg.type.name] = arg;
        }
    }

    QList<TypeStruct> modifiedTypes = types;

    QString functions;
    QMapIterator<QString, QMap<QString,ArgStruct> > pi(properties);
    while(pi.hasNext())
    {
        pi.next();
        const QMap<QString,ArgStruct> &hash = pi.value();
        QMapIterator<QString,ArgStruct> mi(hash);
        while(mi.hasNext())
        {
            mi.next();
            const ArgStruct &arg = mi.value();
            QString argName = arg.argName;
            if(properties[pi.key()].count() > 1 && arg.type.name.toLower() != arg.argName.toLower())
            {
                argName = arg.argName + "_" + QString(arg.type.originalType).remove(classCaseType(arg.argName));

                ArgStruct newArg = arg;
                newArg.argName = argName;

                for(int i=0; i<modifiedTypes.count(); i++)
                {
                    TypeStruct &ts = modifiedTypes[i];
                    int idx = ts.args.indexOf(arg);
                    if(idx != -1)
                        ts.args[idx] = newArg;
                }
            }

            const QString &cammelCase = cammelCaseType(argName);
            const QString &classCase = classCaseType(argName);
            const QtTypeStruct &type = arg.type;
            const QString inputType = type.constRefrence? "const " + type.name + " &" : type.name + " ";

            if(!type.defaultValue.isEmpty())
                resultTypes += QString("    m_%1(%2),\n").arg(cammelCase, type.defaultValue);
            if(resultEqualOperator.isEmpty())
                resultEqualOperator += QString("return m_%1 == b.m_%1").arg(cammelCase);
            else
                resultEqualOperator += QString(" &&\n       m_%1 == b.m_%1").arg(cammelCase);

            functions += QString("void %1::set%2(%3%4) {\n    m_%4 = %4;\n}\n\n").arg(clssName, classCase, inputType, cammelCase);
            functions += QString("%3 %1::%2() const {\n    return m_%2;\n}\n\n").arg(clssName, cammelCase, type.name);
        }
    }
    if(resultEqualOperator.isEmpty())
        resultEqualOperator += "Q_UNUSED(b);\nreturn true;";
    else
        resultEqualOperator += ";";

    result += QString("%1::%1(%1Type classType, InboundPkt *in) :\n").arg(clssName);
    result += resultTypes + QString("    m_classType(classType)\n");;
    result += "{\n    if(in) fetch(in);\n}\n\n";
    result += QString("%1::%1(InboundPkt *in) :\n").arg(clssName);
    result += resultTypes + QString("    m_classType(%1)\n").arg(defaultType);
    result += "{\n    fetch(in);\n}\n\n";
    result += functions;
    result += QString("bool %1::operator ==(const %1 &b) {\n%2}\n\n").arg(clssName, shiftSpace(resultEqualOperator, 1));

    result += QString("void %1::setClassType(%1::%1Type classType) {\n    m_classType = classType;\n}\n\n").arg(clssName);
    result += QString("%1::%1Type %1::classType() const {\n    return m_classType;\n}\n\n").arg(clssName);
    result += QString("bool %1::fetch(InboundPkt *in) {\n\n%2}\n\n").arg(clssName, shiftSpace(fetchFunction(name, modifiedTypes), 1));
    result += QString("bool %1::push(OutboundPkt *out) const {\n\n%2}\n\n").arg(clssName, shiftSpace(pushFunction(name, modifiedTypes), 1));

    QFile file(DEST_PATH "/" + clssName.toLower() + ".cpp");
    if(!file.open(QFile::WriteOnly))
        return;

    file.write(result.toUtf8());
    file.close();
}

void writeType(const QString &name, const QList<TypeStruct> &types)
{
    writeTypeHeader(name, types);
    writeTypeClass(name, types);
}

int main()
{
    QFile file(SCHEMA_PATH);
    if(!file.open(QFile::ReadOnly))
        return -1;

    QDir().mkpath(DEST_PATH);

    QMap<QString, QList<TypeStruct> > types;

    const QStringList &lines = QString(file.readAll()).split("\n",QString::SkipEmptyParts);
    foreach(const QString &line, lines)
    {
        const QString &l = line.trimmed();
        if(l == "---functions---")
            break;

        const QStringList &parts = l.split(" ", QString::SkipEmptyParts);
        if(parts.count() < 3)
            continue;

        const QString signature = parts.first();
        const int signKeyIndex = signature.indexOf("#");
        const QString name = signature.mid(0,signKeyIndex);
        const QString code = signature.mid(signKeyIndex+1);
        const QString structName = QString(parts.last()).remove(";");
        const QStringList args = parts.mid(1, parts.count()-3);

        TypeStruct type;
        foreach(const QString &str, args)
        {
            ArgStruct arg;
            int splitterIdx = str.indexOf(":");

            arg.argName = fixDeniedNames(str.mid(0,splitterIdx));

            QString typePart = str.mid(splitterIdx+1);
            if(typePart == "#")
                typePart = "int";

            int ifIdx = typePart.indexOf("?");
            bool hasIf = (ifIdx != -1);
            arg.type = translateType(hasIf? typePart.mid(ifIdx+1) : typePart);
            if(hasIf)
            {
                QString flagsPart = typePart.mid(0,ifIdx);
                int flagSplitter = flagsPart.indexOf(".");
                if(flagSplitter != -1)
                {
                    arg.flagName = flagsPart.mid(0,flagSplitter);
                    arg.flagValue = flagsPart.mid(flagSplitter+1).toInt();
                }
            }

            type.args << arg;
        }

        type.typeName = "type" + classCaseType(name);
        type.typeCode = "0x" + code;

        types[structName] << type;
    }

    QMapIterator<QString, QList<TypeStruct> > i(types);
    while(i.hasNext())
    {
        i.next();
        const QString &name = i.key();
        const QList<TypeStruct> &types = i.value();
        writeType(name, types);
    }

    return 0;
}
