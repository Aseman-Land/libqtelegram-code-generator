#include "telegramcoregenerator.h"

#include <QFile>
#include <QDebug>

TelegramCoreGenerator::TelegramCoreGenerator(const QString &dst) :
    AbstractGenerator(),
    m_dst(dst)
{
}

TelegramCoreGenerator::~TelegramCoreGenerator()
{
}

void TelegramCoreGenerator::extract(const QString &data)
{
    QMap<QString, QList<GeneratorTypes::FunctionStruct> > functions = extractFunctions(data);
    writeHeader(functions);
    writeCpp(functions);
}

void TelegramCoreGenerator::writeHeader(const QMap<QString, QList<GeneratorTypes::FunctionStruct> > &functions)
{
    QFile file(":/embeds/telegramcore.h");
    file.open(QFile::ReadOnly);

    QString headerOrgData = file.readAll();

    QString result = headerOrgData;
    QString methodsResult;
    QString signalsResult;
    QString errorsResult;
    QString eventsResult;
    QString answersEventsResult;
    QString errorsEventsResult;

    QMapIterator<QString, QList<GeneratorTypes::FunctionStruct> > i(functions);
    while(i.hasNext())
    {
        i.next();
        if(!methodsResult.isEmpty())
            methodsResult += "\n";
        if(!signalsResult.isEmpty())
            signalsResult += "\n";
        if(!answersEventsResult.isEmpty())
            answersEventsResult += "\n";
        if(!errorsEventsResult.isEmpty())
            errorsEventsResult += "\n";
        if(!errorsResult.isEmpty())
            errorsResult += "\n";

        const QString &name = i.key();
        const QList<GeneratorTypes::FunctionStruct> &types = i.value();
        foreach(const GeneratorTypes::FunctionStruct &t, types)
        {
            if(!methodsResult.isEmpty())
                methodsResult += "\n";
            if(!signalsResult.isEmpty())
                signalsResult += "\n";
            if(!answersEventsResult.isEmpty())
                answersEventsResult += "\n";
            if(!errorsEventsResult.isEmpty())
                errorsEventsResult += "\n";
            if(!errorsResult.isEmpty())
                errorsResult += "\n";

            QString functionName = name.toLower() + t.functionName[0].toUpper() + t.functionName.mid(1);
            QString arguments;

            const QList<GeneratorTypes::ArgStruct> &args = t.type.args;
            foreach(const GeneratorTypes::ArgStruct &arg, args)
            {
                if(arg.isFlag)
                    continue;

                if(arg.type.constRefrence)
                    arguments += QString("const %1 &%2").arg(arg.type.name, arg.argName);
                else
                    arguments += QString("%1 %2").arg(arg.type.name, arg.argName);
                arguments += ", ";
            }

            QString returnArg;
            if(t.returnType.constRefrence)
                returnArg = QString("const %1 &result").arg(t.returnType.name);
            else
                returnArg = QString("%1 result").arg(t.returnType.name);

            methodsResult += QString("virtual qint64 %1(%2Callback<%3 > callBack = 0);").arg(functionName, arguments, t.returnType.name);
            signalsResult += QString("void %1Answer(qint64 msgId, %2);").arg(functionName, returnArg);
            errorsResult += QString("void %1Error(qint64 msgId, qint32 errorCode, const QString &errorText);").arg(functionName);
            answersEventsResult += QString("virtual void on%1Answer(qint64 msgId, %2);").arg(classCaseType(functionName), returnArg);
            errorsEventsResult += QString("virtual void on%1Error(qint64 msgId, qint32 errorCode, const QString &errorText);").arg(classCaseType(functionName));
        }
    }

    eventsResult += answersEventsResult + "\n\n\n";
    eventsResult += errorsEventsResult;

    result.replace("/*! === methods === !*/", shiftSpace(methodsResult, 1));
    result.replace("/*! === signals === !*/", shiftSpace(signalsResult, 1) + "\n\n" + shiftSpace(errorsResult, 1));
    result.replace("/*! === events === !*/", shiftSpace(eventsResult, 1));


    QFile outFile(m_dst + "/telegramcore.h");
    outFile.open(QFile::WriteOnly);
    outFile.write(result.toUtf8());
    outFile.close();
}

void TelegramCoreGenerator::writeCpp(const QMap<QString, QList<GeneratorTypes::FunctionStruct> > &functions)
{
    QFile file(":/embeds/telegramcore.cpp");
    file.open(QFile::ReadOnly);

    QString headerOrgData = file.readAll();

    QString result = headerOrgData;
    QString constructorsResult;
    QString methodsResult;

    QMapIterator<QString, QList<GeneratorTypes::FunctionStruct> > i(functions);
    while(i.hasNext())
    {
        i.next();
        if(!constructorsResult.isEmpty())
            constructorsResult += "\n";
        if(!methodsResult.isEmpty())
            methodsResult += "\n";

        const QString &name = i.key();
        const QList<GeneratorTypes::FunctionStruct> &types = i.value();
        foreach(const GeneratorTypes::FunctionStruct &t, types)
        {
            if(!constructorsResult.isEmpty())
                constructorsResult += "\n";
            if(!methodsResult.isEmpty())
                methodsResult += "\n";

            QString functionName = name.toLower() + t.functionName[0].toUpper() + t.functionName.mid(1);
            QString classCase = classCaseType(functionName);
            QString arguments;
            QString argumentNames;
            QString signatures;

            const QList<GeneratorTypes::ArgStruct> &args = t.type.args;
            foreach(const GeneratorTypes::ArgStruct &arg, args)
            {
                if(arg.isFlag)
                    continue;

                signatures += ",";
                if(!argumentNames.isEmpty())
                    argumentNames += ", ";

                argumentNames += arg.argName;
                signatures += arg.type.name;
                if(arg.type.constRefrence)
                    arguments += QString("const %1 &%2").arg(arg.type.name, arg.argName);
                else
                    arguments += QString("%1 %2").arg(arg.type.name, arg.argName);
                arguments += ", ";
            }

            QString returnArg;
            if(t.returnType.constRefrence)
                returnArg = QString("const %1 &result").arg(t.returnType.name);
            else
                returnArg = QString("%1 result").arg(t.returnType.name);

            QString nullResult = t.returnType.constRefrence? QString("%1 = %2()").arg(returnArg, t.returnType.name) :
                                                             QString("%1 = 0").arg(returnArg);
            QString callBackError = QString("%1;\nif(callBack)\n    callBack(0, result, apiError());").arg(nullResult);
            QString methodInner = QString("if(!mApi) {\n%4    return 0;\n}\n"
                                          "qint64 msgId = mApi->%1(%2);\n"
                                          "if(msgId)\n    callBackPush<%3 >(msgId, callBack);\n"
                                          "else {\n%4}\nreturn msgId;").arg(functionName, argumentNames, t.returnType.name, shiftSpace(callBackError,1));

            QString answersInnter = QString("callBackCall<%1 >(msgId, result);\n"
                                            "Q_EMIT %2Answer(msgId, result);")
                                  .arg(t.returnType.name, functionName);

            QString errorsInnter = QString("%1;\nCallbackError error;\nerror.errorCode = errorCode;\n"
                                           "error.errorText = errorText;\nerror.null = false;\n"
                                           "callBackCall<%2 >(msgId, result, error);\n"
                                           "Q_EMIT %3Error(msgId, errorCode, errorText);")
                                  .arg(nullResult, t.returnType.name, functionName);

            constructorsResult += QString("connect(mApi, SIGNAL(%1Answer(qint64%3)), this, SLOT(on%2Answer(qint64%3)));\n"
                                          "connect(mApi, SIGNAL(%1Error(qint64,QString,QString)), this, SLOT(0n%2Error(qint64,QString,QString)));\n")
                                .arg(functionName, classCase, signatures);
            methodsResult += QString("qint64 TelegramCore::%1(%2Callback<%4 > callBack) {\n%3}\n\n").arg(functionName, arguments, shiftSpace(methodInner, 1), t.returnType.name)
                           + QString("void TelegramCore::on%1Answer(qint64 msgId, %2) {\n%3}\n\n").arg(classCase, returnArg, shiftSpace(answersInnter, 1))
                           + QString("void TelegramCore::on%1Error(qint64 msgId, qint32 errorCode, const QString &errorText) {\n%2}\n").arg(classCase, shiftSpace(errorsInnter, 1));
        }
    }

    result.replace("/*! === constructor === !*/", shiftSpace(constructorsResult, 1));
    result.replace("/*! === methods === !*/", methodsResult);


    QFile outFile(m_dst + "/telegramcore.cpp");
    outFile.open(QFile::WriteOnly);
    outFile.write(result.toUtf8());
    outFile.close();
}
