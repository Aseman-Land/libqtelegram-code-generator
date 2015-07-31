#include "functiongenerator.h"

#include <QDebug>
#include <QFile>
#include <QDir>

FunctionGenerator::FunctionGenerator(const QString &dest) :
    m_dst(dest)
{
}

FunctionGenerator::~FunctionGenerator()
{
}

QString FunctionGenerator::typeToFetchFunction(const QString &arg, const QString &type, const GeneratorTypes::ArgStruct &argStruct)
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
        baseResult += QString("if(in->fetchInt() != (qint32)TL_Vector) return %1;\n").arg(arg);

        baseResult += "qint32 " + arg + "_length = in->fetchInt();\n";
        baseResult += arg + ".clear();\n";
        baseResult += "for (qint32 i = 0; i < " + arg + "_length; i++) {\n";
        baseResult += "    " + innerType + " type;\n    " + typeToFetchFunction("type", innerType, argStruct) + "\n";
        baseResult += QString("    %1.append(type);\n}").arg(arg);
    }
    else
    {
        baseResult += QString("if(!" + arg + ".fetch(in)) return %1;").arg("result");
    }

    if(!argStruct.flagName.isEmpty())
    {
        result += QString("if(m_%1 & 1<<%2) ").arg(argStruct.flagName).arg(argStruct.flagValue);
        baseResult = "{\n" + shiftSpace(baseResult, 1) + "}";
    }

    result += baseResult;
    return result;
}

QString FunctionGenerator::fetchFunction(const QString &clssName, const QString &fncName, const GeneratorTypes::ArgStruct &arg)
{
    Q_UNUSED(clssName)
    Q_UNUSED(fncName)

    QString result;
    result += QString("%1 %2;\n").arg(arg.type.name, arg.argName);
    result += typeToFetchFunction(cammelCaseType(arg.argName), arg.type.name, arg) + "\n";
    result += QString("return %1;").arg(arg.argName);
    result = shiftSpace(result, 1);
    return result;
}

QString FunctionGenerator::typeToPushFunction(const QString &arg, const QString &type, const GeneratorTypes::ArgStruct &argStruct)
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
        result += "if(!" + arg + ".push(out)) return false;";
    }

    return result;
}

QString FunctionGenerator::pushFunction(const QString &clssName, const QString &fncName, const QList<GeneratorTypes::ArgStruct> &types)
{
    QString result;
    result += QString("out->appendInt(fnc%1%2);\n").arg(clssName, classCaseType(fncName));
    foreach(const GeneratorTypes::ArgStruct &arg, types)
    {
        if(!arg.flagName.isEmpty())
            result += QString("if(%1 & 1<<%2) ").arg(arg.flagName).arg(arg.flagValue);
        result += typeToPushFunction(cammelCaseType(arg.argName), arg.type.name, arg) + "\n";
    }

    result += "return true;\n";
    result = shiftSpace(result, 1);

    return result;
}

void FunctionGenerator::extract(const QString &data)
{
    QDir().mkpath(m_dst);

    QMap<QString, QList<GeneratorTypes::FunctionStruct> > types;
    const QStringList &lines = QString(data).split("\n",QString::SkipEmptyParts);
    bool functions = false;
    foreach(const QString &line, lines)
    {
        const QString &l = line.trimmed();
        if(l == "---functions---")
        {
            functions = true;
            continue;
        }
        if(!functions)
            continue;

        const QStringList &parts = l.split(" ", QString::SkipEmptyParts);
        if(parts.count() < 3)
            continue;

        const QString signature = parts.first();
        const int signKeyIndex = signature.indexOf("#");
        const QString name = signature.mid(0,signKeyIndex);
        const int pointIndex = name.indexOf(".");
        if(pointIndex < 1)
            continue;

        const QString fileName = name.mid(0, pointIndex);
        const QString className = classCaseType(fileName);
        const QString functionName = name.mid(pointIndex+1);
        const QString code = signature.mid(signKeyIndex+1);
        QString returnType = QString(parts.last()).remove(";");
        if(returnType == "Updates")
            returnType = "UpdatesType";
        const QStringList args = parts.mid(1, parts.count()-3);

        GeneratorTypes::FunctionStruct fnc;
        foreach(const QString &str, args)
        {
            GeneratorTypes::ArgStruct arg;
            int splitterIdx = str.indexOf(":");

            arg.argName = fixDeniedNames(str.mid(0,splitterIdx));

            QString typePart = str.mid(splitterIdx+1);
            if(typePart == "#")
                typePart = "int";

            int ifIdx = typePart.indexOf("?");
            bool hasIf = (ifIdx != -1);
            arg.type = translateType(hasIf? typePart.mid(ifIdx+1) : typePart, false, "types/");
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

            fnc.type.args << arg;
        }

        fnc.type.typeName = "fnc" + classCaseType(name);
        fnc.type.typeCode = "0x" + code;
        fnc.className = className;
        fnc.functionName = functionName;
        fnc.returnType = translateType(returnType, false, "types/");

        types[className] << fnc;
    }

    QMapIterator<QString, QList<GeneratorTypes::FunctionStruct> > i(types);
    while(i.hasNext())
    {
        i.next();
        const QString &name = i.key();
        const QList<GeneratorTypes::FunctionStruct> &types = i.value();
        writeType(name, types);
    }

    writePri(types.keys());
    writeMainHeader(types.keys());
    copyEmbeds();
}

void FunctionGenerator::writeTypeHeader(const QString &name, const QList<GeneratorTypes::FunctionStruct> &functions)
{
    const QString &clssName = classCaseType(name);

    QString result;
    result += "namespace Tg {\n";
    result += "namespace Functions {\n\n";
    result += QString("class LIBQTELEGRAMSHARED_EXPORT %1 : public TelegramFunctionObject\n{\npublic:\n"
                      "    enum %1Function {\n").arg(clssName);

    QString includes = "#include \"telegramfunctionobject.h\"\n";
    QString resultFnc;

    QSet<QString> addedIncludes;
    QMap<QString, QMap<QString,GeneratorTypes::ArgStruct> > properties;
    for(int i=0; i<functions.count(); i++)
    {
        const GeneratorTypes::FunctionStruct &f = functions[i];
        const GeneratorTypes::TypeStruct &t = f.type;

        result += "        " + t.typeName + " = " + t.typeCode;
        if(i < functions.count()-1)
            result += ",\n";
        else
            result += "\n    };\n\n";

        foreach(const QString &inc, f.returnType.includes)
        {
            if(addedIncludes.contains(inc))
                continue;
            includes += inc + "\n";
            addedIncludes.insert(inc);
        }

        QString fncArgs;
        for(int j=0; j<t.args.length(); j++)
        {
            const GeneratorTypes::ArgStruct &arg = t.args[j];
            foreach(const QString &inc, arg.type.includes)
            {
                if(addedIncludes.contains(inc))
                    continue;
                includes += inc + "\n";
                addedIncludes.insert(inc);
            }

            properties[f.functionName][arg.type.name] = arg;

            const QString inputType = arg.type.constRefrence? "const " + arg.type.name + " &" : arg.type.name + " ";
            fncArgs += QString(", %1%2").arg(inputType, cammelCaseType(arg.argName));
        }

        resultFnc += QString("    static bool %1(OutboundPkt *out%2);\n").arg(f.functionName, fncArgs);
        resultFnc += QString("    static %2 %1Result(InboundPkt *in);\n\n").arg(f.functionName,f.returnType.name);
    }

    result += QString("    %1();\n").arg(clssName);
    result += QString("    virtual ~%1();\n\n").arg(clssName);
    result += resultFnc;

    result += "};\n\n";
    result += "}\n}\n\n";

    result = includes + "\n" + result;
    result = QString("#ifndef LQTG_FNC_%1\n#define LQTG_FNC_%1\n\n").arg(clssName.toUpper()) + result;
    result += QString("#endif // LQTG_FNC_%1\n").arg(clssName.toUpper());

    QFile file(m_dst + "/" + clssName.toLower() + ".h");
    if(!file.open(QFile::WriteOnly))
        return;

    file.write(GENERATOR_SIGNATURE("//"));
    file.write(result.toUtf8());
    file.close();
}

void FunctionGenerator::writeTypeClass(const QString &name, const QList<GeneratorTypes::FunctionStruct> &functions)
{
    const QString &clssName = classCaseType(name);

    QString result;
    result += QString("#include \"%1.h\"\n").arg(clssName.toLower()) +
            "#include \"core/inboundpkt.h\"\n"
            "#include \"core/outboundpkt.h\"\n"
            "#include \"util/tlvalues.h\"\n\n";

    result += "using namespace Tg;\n\n";
    result += QString("Functions::%1::%1() {\n}\n\n").arg(clssName);
    result += QString("Functions::%1::~%1() {\n}\n\n").arg(clssName);

    QString resultFnc;

    QMap<QString, QMap<QString,GeneratorTypes::ArgStruct> > properties;
    for(int i=0; i<functions.count(); i++)
    {
        const GeneratorTypes::FunctionStruct &f = functions[i];
        const GeneratorTypes::TypeStruct &t = f.type;

        GeneratorTypes::ArgStruct retArg;
        retArg.argName = "result";
        retArg.type = f.returnType;

        const QString &pushResult = pushFunction(f.className, f.functionName, t.args);
        QString fetchResult = fetchFunction(f.className, f.functionName, retArg);
        QString fncArgs;

        for(int j=0; j<t.args.length(); j++)
        {
            const GeneratorTypes::ArgStruct &arg = t.args[j];

            properties[f.functionName][arg.type.name] = arg;
            const QString inputType = arg.type.constRefrence? "const " + arg.type.name + " &" : arg.type.name + " ";

            fncArgs += QString(", %1%2").arg(inputType, cammelCaseType(arg.argName));
        }

        resultFnc += QString("bool Functions::%3::%1(OutboundPkt *out%2) {\n%4}\n\n").arg(f.functionName, fncArgs,clssName,pushResult);
        resultFnc += QString("%2 Functions::%3::%1Result(InboundPkt *in) {\n%4}\n\n").arg(f.functionName,f.returnType.name,clssName,fetchResult);
    }

    result += resultFnc;

    QFile file(m_dst + "/" + clssName.toLower() + ".cpp");
    if(!file.open(QFile::WriteOnly))
        return;

    file.write(GENERATOR_SIGNATURE("//"));
    file.write(result.toUtf8());
    file.close();
}

void FunctionGenerator::writeType(const QString &name, const QList<GeneratorTypes::FunctionStruct> &types)
{
    writeTypeHeader(name, types);
    writeTypeClass(name, types);
}

void FunctionGenerator::writePri(const QStringList &types)
{
    QString result = "\n";
    QString headers = "HEADERS += \\\n    $$PWD/functions.h \\\n    $$PWD/telegramfunctionobject.cpp \\\n";
    QString sources = "SOURCES += \\\n    $$PWD/telegramfunctionobject.cpp \\\n";
    for(int i=0; i<types.count(); i++)
    {
        const QString &t = classCaseType(types[i]);
        const bool last = (i == types.count()-1);

        headers += QString(last? "    $$PWD/%1.h\n" : "    $$PWD/%1.h \\\n").arg(t.toLower());
        sources += QString(last? "    $$PWD/%1.cpp\n" : "    $$PWD/%1.cpp \\\n").arg(t.toLower());
    }

    result += headers + "\n";
    result += sources + "\n";

    QFile file(m_dst + "/functions.pri");
    if(!file.open(QFile::WriteOnly))
        return;

    file.write(GENERATOR_SIGNATURE("#"));
    file.write(result.toUtf8());
    file.close();
}

void FunctionGenerator::writeMainHeader(const QStringList &types)
{
    QString result = "\n#include \"telegramfunctionobject.h\"\n";
    for(int i=0; i<types.count(); i++)
    {
        const QString &t = classCaseType(types[i]);
        result += QString("#include \"%1.h\"\n").arg(t.toLower());
    }

    QFile file(m_dst + "/functions.h");
    if(!file.open(QFile::WriteOnly))
        return;

    file.write(GENERATOR_SIGNATURE("//"));
    file.write(result.toUtf8());
    file.close();
}

void FunctionGenerator::copyEmbeds()
{
    const QString &fcpp = m_dst + "/telegramfunctionobject.cpp";
    QFile::remove(fcpp);
    QFile::copy(":/embeds/telegramfunctionobject.cpp", fcpp);
    QFile(fcpp).setPermissions(QFileDevice::ReadUser|QFileDevice::WriteUser|
                                  QFileDevice::ReadGroup|QFileDevice::WriteGroup);

    const QString &fh = m_dst + "/telegramfunctionobject.h";
    QFile::remove(fh);
    QFile::copy(":/embeds/telegramfunctionobject.h", fh);
    QFile(fh).setPermissions(QFileDevice::ReadUser|QFileDevice::WriteUser|
                                  QFileDevice::ReadGroup|QFileDevice::WriteGroup);
}

