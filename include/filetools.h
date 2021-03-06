#ifndef FILETOOLS_H
#define FILETOOLS_H

#include <functional>
#include <QMap>
#include <QString>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QVector>
#include <QChar>

// Used to represent an object or element that does not exist or was not found.
const QString NULL_ELEMENT = "NULL_ELEMENT";
const QString NULL_OBJECT = "NULL_OBJECT";

// File extension used by data files.
const QString DAT_EXT = ".dat";

// Constants used to find objects, elements and data files
const QString DAT_QUEST = "quest";

// Quest Database
const QString DAT_DATABASE = "project_db";
const QString OBJ_MAP = "map";
const QString ELE_ID = "id";
const QString ELE_DESCRIPTION = "description";

// Quest Object
const QString OBJ_QUEST = "quest";
const QString ELE_TITLE_BAR = "title_bar";
const QString ELE_WRT_DIR = "write_dir";
const QString ELE_SOL_VERS = "solarus_version";

// Map objects/elements
const QString OBJ_PROPERTIES = "properties";
const QString OBJ_TILE = "tile";
const QString ELE_X = "x";
const QString ELE_Y = "y";
const QString ELE_WIDTH = "width";
const QString ELE_HEIGHT = "height";
const QString ELE_TILE_SIZE = "tile_size";
const QString ELE_WORLD = "world";
const QString ELE_TILESET = "tileset";
const QString ELE_MUSIC = "music";
const QString ELE_LAYER = "layer";
const QString ELE_PATTERN = "pattern";

// Tileset
const QString OBJ_TILE_PATTERN = "tile_pattern";
const QString ELE_DEFAULT_LAYER = "default_layer";
const QString ELE_GROUND = "ground";

// Mission Items
const QString DAT_MISSION_ITEMS = "proc_designer_data" + QString(QDir::separator()) + "mission_items";

// Key Event
const QString OBJ_KEY_EVENT = "key_event";
const QString ELE_NAME = "name";
const QString ELE_KEY_TYPE = "key_type";
const QString ELE_KEY_MESSAGE = "key_message";

// Gate
const QString OBJ_GATE = "gate";
const QString ELE_KEY_LINKS = "key_links";
const QString ELE_GATE_TYPE = "gate_type";
const QString ELE_TRIGGERED = "triggered";

// Program Preferences
const QString DAT_PREFERENCES = QDir::currentPath() + QDir::separator() + "preferences.dat";
const QString OBJ_PREFERENCES = "preferences";
const QString ELE_SOLARUS_PATH = "solarus_path";

// Object, element and value delimiters for parsing and building .dat files.
const QVector<QChar> FIND_OBJ_DELIMS =  {'{'};
const QVector<QChar> OBJ_DELIMS =       {'}'};
const QVector<QChar> ELEM_DELIMS =      {'='};
const QVector<QChar> VAL_DELIMS =       {','};


/*!
 * \brief Will copy all files in the given directory and all subdirectories into the destination directory.
 * \param sourceDir The source directory to copy.
 * \param destDir The destination directory.
 */
void copyFolder(QString sourceDir, QString destDir);

/*!
 * \brief creatFile Creates a file at the given location with the given file contents.
 * \param dirPath Path to create the file at.
 * \param fileName Name of the file.
 * \param fileContents String containing the desired contents of the file (can be empty).
 */
void writeToFile(QString dirPath, QString fileName, QString fileContents);

/*!
 * \brief ObjectData Typedef used to represent object data in a QMap.
 */
typedef QMap<QString,QString> ObjectData;

struct Object
{
public:
    Object() : data(QMap<QString,QString>()) { }
    Object(ObjectData data) : data(data) { }

    ObjectData data;

    QString find(QString element, QString defaultVal = "NULL");
    void insert(QString element, QString value);

    bool operator==(const Object& param);
    bool operator!=(const Object& param);
};

/*!
 * \brief Class representing a table of data from a .dat file.
 */
class Table
{
public:
    Table();

    /*!
     * \brief Creates a new table from the given filepath and parses all existing data into memory. If no file exists, creates
     *        a new blank table associated with the given filepath.
     * \param filePath The filepath to associate with the table.
     */
    Table(QString filePath);

    virtual ~Table();

    /*!
     * \brief Sets the file path used by the table. Source data is read from here, and saved here when the table is output.
     * \param filePath The filepath.
     */
    void setFilePath(QString filePath);

    /*!
     * \brief Get the file path associated with this table.
     * \return The file path associated with this table.
     */
    QString getFilePath() const;

    /*!
     * \brief saveToDisk Writes out data in the table to the currently specified file path.
     */
    void saveToDisk();

    /*!
     * \brief Parses a given file into the table.
     * \param filePath The filepath to read into the table.
     */
    void parse(QString filePath);

    /*!
     * \brief Adds an object to the table.
     * \param name The name of the object.
     * \param object The object to add to the table.
     */
    void addObject(QString name, Object object);

    /*!
     * \brief Retrieve an object's collection of elements and their values. If multiple objects of the same name are found, returns
     *        the last object added to the table.
     * \param objectName The name of the object to retrieve from the table.
     * \return
     */
   Object* getObject(QString objectName);

    /*!
     * \brief Get the value of an element from an object. If multiple objects of the same name are found, returns
     *        the value from the last object added to the table.
     * \param objectName The name of the object.
     * \param elementName The name of the element.
     * \return
     */
    QString getElementValue(QString objectName, QString elementName);

    /*!
     * \brief getObjects Retrieves a QList of all objects.
     */
    QList<Object*> getObjects();

    /*!
     * \brief Retrieves a QList of all objects with the given name.
     * \param objectName The name of the objects to retrieve.
     * \return List of all objects with the given name.
     */
    QList<Object*> getObjectsOfName(QString objectName);

    /*!
     * \brief Searches the table for the first object with the given element set to the given value.
     * \param objectName The object name to search under.
     * \param elementName The element to search for.
     * \param value The value to search for.
     * \return
     */
    Object* getObjectWithValue(QString objectName, QString elementName, QString value);

    /*!
     * \brief Set the value of an element in an object.
     * \param objectName Name of the object.
     * \param elementName Name of the value.
     * \param value Value to set.
     * \return true if value was set correctly.
     */
    bool setElementValue(QString objectName, QString elementName, QString value);

    /*!
     * \brief Returns whether or not the table is empty.
     * \return True if table is empty, false if not.
     */
    bool isEmpty() const;

    /*!
     * \brief Clears the contents of the entire table in memory.
     */
    void clear();

    /*!
     * \brief areEqual Compares this table to the given table. Returns false if they have any differences.
     */
    bool areEqual(Table* table);

    /*!
     * \brief existsOnDisk Check if this table's filepath currently exists on the hard disk.
     * \return
     */
    bool existsOnDisk();

    bool operator==(Table& param);
    bool operator!=(Table& param);

private:

    // Reading Functions
    void beginRead();
    void findObj();
    void readObj();
    void readElements();

    // Writing Functions
    void beginWrite();
    void writeObj(QString objectName, Object object);

    /*!
     * \brief readUntil Read a given text stream until the delimiter value is found.
     * \param delim The delimiter value.
     * \return Value read until the given delimiter value.
     */
    QString readUntil(QTextStream& stream, QVector<QChar> delims);

    QString currentObjectName, currentElement, currentObject; /*!< The current object and element in the parser state. */

    Object curObjectData; /*!< Data stored by the current object. */
    QFile file;           /*!< The file currently being used for reading. */
    QTextStream in;       /*!< The stream currently being used for reading. */
    QTextStream out;      /*!< The stream currently being used for writing. */

    QString filePath;
    QMultiMap<QString, Object> objects; /*!< Map of all objects, containing a map of respective elements. */
};


#endif // FILETOOLS_H
