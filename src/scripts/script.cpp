#include "script.h"

QString Script::Function::build()
{
    QString concat = "function " + name + "()";
    for(QString line : lines)
        concat.append("\n" + line);
    concat.append("\nend");
    return concat;
}

Script::Function* Script::getFunction(QString name)
{
    auto iter = functions.find(name);
    if(iter != functions.end())
        return &iter.value();
    else
        return nullptr;
}

bool Script::addPriorityVariable(QString name)
{
    if(variables.contains(name))
    {
        priorityVariables.append(name);
        return true;
    }
    else
        return false;
}

bool Script::addPriorityFunction(QString name)
{
    if(functions.contains(name))
    {
        priorityFunctions.append(name);
        return true;
    }
    else
        return false;
}

void Script::writeToFile(QFile& file)
{
    file.open(QIODevice::WriteOnly);

    if(!file.isOpen())
        return;

    QTextStream out(&file);

    out << "-- The following script has been generated by the ProcLevelDesigner.\n";

    // Write priority variables
    for(QString name : priorityVariables)
        out << variables.find(name).value().build() << "\n";

    // Write priority functions
    for(QString name : priorityFunctions)
        out << functions.find(name).value().build() << "\n";

    // Write remaining variables
    for(QMap<QString,Variable>::iterator iter = variables.begin(); iter != variables.end(); iter++)
    {
        if(!priorityVariables.contains(iter.key()))
            out << iter.value().build() << "\n";
    }

    // Write all functions
    for(QMap<QString,Function>::iterator iter = functions.begin(); iter != functions.end(); iter++)
    {
        if(!priorityFunctions.contains(iter.key()))
            out << iter.value().build() << "\n";
    }

    out << returnStatement << "\n";

    out << "-- End of generated script.\n";

    out.flush();
    file.flush();
    file.close();
}