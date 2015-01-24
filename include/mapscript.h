#ifndef MAPSCRIPT_H
#define MAPSCRIPT_H

#include <QString>
#include <QMap>

#include "door.h"
#include "switchentity.h"
#include "key.h"

/**
 * @brief Represents an individual map script. Use member functions to add to the script.
 */
class MapScript
{
public:
    class Item
    {
    public:
        Item(QString name) : name(name) { }

        QString getName() { return name; }

        virtual QString build() = 0;

    protected:
        QString name;
    };

    /**
     * @brief Represents an individual variable.
     */
    class Variable : public Item
    {
    public:
        Variable(QString name, QString value) : Item(name), value(value) { }

        virtual QString build() override final { return "local " + name + " = " + value; }

    private:
        QString value;
    };

    /**
     * @brief Represents an individual function.
     */
    class Function : public Item
    {
    public:
        Function(QString name) : Item(name) { }

        void addLine(QString line) { lines.append(line); }

        virtual QString build() override final;

    private:
        QStringList lines;
    };

    MapScript();
    virtual ~MapScript();

    Function* getFunction(QString name);

    /**
     * @brief Adds the ability for this script to handle a switch of the given name.
     * @param keyName The name of the key containing the switch.
     */
    void addSwitch(SwitchEntity switchEntity);

    /**
     * @brief Adds the ability for this script to handle a door of a given name.
     * @param gateName The name of the gate containing the door.
     * @param keys The keys required to open the gate.
     */
    void addDoor(Door door, QList<Key*> keys);

    /**
     * @brief Write this script out to the given file.
     * @param file The file to write the script into.
     */
    void writeToFile(QFile& file);

private:
    QMap<QString,Function> functions;
    QMap<QString,Variable> variables;
};

#endif // MAPSCRIPT_H