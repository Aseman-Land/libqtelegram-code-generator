#include "typegenerator.h"
#include "functiongenerator.h"

#include <QFile>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    if(app.arguments().count() < 3)
        return 0;

    const QString schemaPath = app.arguments().at(1);
    const QString destPath = app.arguments().at(2);

    QFile file(schemaPath);
    if(!file.open(QFile::ReadOnly))
        return -1;

    QDir().mkpath(destPath);

    const QString &data = file.readAll();

    TypeGenerator(destPath + "/types").extract(data);
    FunctionGenerator(destPath + "/functions").extract(data);

    return 0;
}
