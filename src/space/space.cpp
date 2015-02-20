#include "space.h"

static constexpr int SIDE_MAP_TRANSPORTER_SIZE = 16; /*!< The size that every side map transporter must be. */

void SpaceReceiver::missionUpdated()
{
    space->missionUpdated();
}

Space::Space()
{
    observer = new SpaceObserver();
    receiver = nullptr;
}

Space::~Space()
{

}

// Temporary function - generates walls around an area in directions it is not linked
void generateWalls(Area& area)
{
    Grid* grid = area.getGrid();

    // Set values (wall off the area)
    if(!area.getLinkLeft())
    {
        for(int y = 0; y < grid->getHeight(); y++)
            grid->setCellTraversable(false, 0, y);
    }
    if(!area.getLinkRight())
    {
        for(int y = 0; y < grid->getHeight(); y++)
            grid->setCellTraversable(false, grid->getWidth()-1, y);
    }
    if(!area.getLinkUp())
    {
        for(int x = 0; x < grid->getWidth(); x++)
            grid->setCellTraversable(false, x, 0);
    }
    if(!area.getLinkDown())
    {
        for(int x = 0; x < grid->getWidth(); x++)
            grid->setCellTraversable(false, x, grid->getHeight()-1);
    }
}

void Space::generate(Mission& mission)
{
    clear();

    QList<Stage*> stages = mission.getStages(); // Stages to generate areas for
    QMap<Stage*,QList<Area>> generatedAreas = QMap<Stage*,QList<Area>>(); // The areas that are generated here, mapped to the stages they were generated for

    for(Stage* stage : stages)
    {
        QList<Area> stageAreas = QList<Area>(); // Will contain all areas generated for this stage

        // Assign a random zone for this stage
        auto iter = zones.begin();
        iter += rand.randomInteger(0, zones.count()-1);
        Zone* zone = &iter.value();

        // Determine first area location (this is based on a previously generated set of areas, if it exists)
        Area firstArea;
        if(generatedAreas.count() > 0)
        {
            Area baseArea;
            Direction dir;

            // Pick a random stage, area and direction to expand
            do {
                QMap<Stage*,QList<Area>>::iterator iter = generatedAreas.begin();
                iter += rand.randomInteger(0, generatedAreas.count()-1);
                Stage* stage = iter.key();
                QList<Area> areaSet = generatedAreas.find(stage).value();

                baseArea = areaSet.at(rand.randomInteger(0, areaSet.count()-1));
                dir = static_cast<Direction>(rand.randomInteger(0, Direction::COUNT));
                firstArea = Area(stage->getID(), zone, randomAreaWidth(), randomAreaHeight());

            } while(!placeInDirection(baseArea, firstArea, dir));
        }
        else // If this is the first stage, just pick a random location
            firstArea = AreaFactory::RandomArea(stage->getID(), zone, 0, 10, 0, 10, options);

        stageAreas.append(firstArea);
        placeArea(firstArea); // We must always place new areas, for space checking to work

        // Determine number of areas for this stage
        int areaCount = rand.randomInteger(options.getMinimumAreasPerStage(), options.getMaximumAreasPerStage());

        // Generate the remaining areas
        for(int i = 1; i < areaCount; i++)
        {
            Area newArea;
            Area baseArea;
            Direction dir;

            // Check that there is at least room for a 1x1 area in each direction, if no direction has room, break the loop
            bool canFit = false;
            newArea = Area(stage->getID(), zone, 1, 1);
            canFit = placeInDirection(baseArea, newArea, Direction::NORTH) || placeInDirection(baseArea, newArea, Direction::SOUTH) ||
                     placeInDirection(baseArea, newArea, Direction::EAST) || placeInDirection(baseArea, newArea, Direction::WEST);

            int loopLimit = 100;
            int loopCount = 0;

            if(canFit)
            {
                do {
                    loopCount++;
                    if(loopCount > loopLimit)
                    {
                        canFit = false;
                        break;
                    }

                    baseArea = stageAreas[rand.randomInteger(0, stageAreas.count()-1)];
                    dir = static_cast<Direction>(rand.randomInteger(0, Direction::COUNT));
                    newArea = Area(stage->getID(), zone, randomAreaWidth(), randomAreaHeight());
                } while(!placeInDirection(baseArea, newArea, dir));

                if(canFit)
                {
                    stageAreas.append(newArea);
                    placeArea(newArea); // We must always place new areas, for space checking to work
                }
            }
            else
                break;
        }

        generatedAreas.insert(stage, stageAreas);
    }

    // Generate walls and connect all areas
    for(auto iter = areas.begin(); iter != areas.end(); iter++)
    {
        Area& area = iter.value();

        QList<QPoint> neighbours = getNeighbours(area);
        for(QPoint point : neighbours)
        {
            Area& neighbour = areas.find(point).value();
            if(area.getStageID() == neighbour.getStageID())
            {
                Direction dir = getDirection(area, neighbour);
                switch(dir)
                {
                case NORTH:
                    area.setLinkUp(Link(area.getLocation(), neighbour.getLocation()));
                    break;
                case SOUTH:
                    area.setLinkDown(Link(area.getLocation(), neighbour.getLocation()));
                    break;
                case EAST:
                    area.setLinkRight(Link(area.getLocation(), neighbour.getLocation()));
                    break;
                case WEST:
                    area.setLinkLeft(Link(area.getLocation(), neighbour.getLocation()));
                    break;
                }
            }
        }

        generateWalls(area);
    }


    emitUpdate();
}

Space Space::Parse(Table* data, QList<Gate*> gates, QList<Key*> keys, QList<Tileset*> tilesets)
{
    Space space;

    // Read in all zones
    QList<Object*> zoneObjects = data->getObjectsOfName(OBJ_ZONE);
    for(Object* obj : zoneObjects)
    {
        Zone zone = Zone::Parse(obj, tilesets);
        space.zones.insert(zone.getName(), zone);
    }

    // Read in all areas (areas subvert the quest data system, they must load their cells from a table that they manage,
    //                      therefore they are given the filepath of the space data to use as a basis for loading this table)
    QList<Object*> areaObjects = data->getObjectsOfName(OBJ_AREA);
    for(Object* obj : areaObjects)
    {
        space.placeArea(Area::Parse(obj, QFileInfo(data->getFilePath()).absoluteDir().absolutePath(), keys, gates));
    }

    // Read in all links
    QList<Link> linkList;
    QList<Object*> linkObjects = data->getObjectsOfName(OBJ_LINK);
    for(Object* obj : linkObjects)
    {
        Link link = Link::Parse(obj);
        linkList.append(link);
    }

    // Point areas to their zones
    QMap<QPoint,Area>* areas = space.getAreas();
    for(auto iter = areas->begin(); iter != areas->end(); iter++)
    {
        Zone* zone = &space.zones.find(iter.value().getZoneName()).value();
        if(zone)
            iter.value().setZone(zone);
    }

    // Place all links into their correct areas
    for(Link& link : linkList)
    {
        Area* first = &areas->find(link.getOrigin()).value();
        Area* second = &areas->find(link.getTarget()).value();

        LinkDirection direction;

        // Decide on the link direction (respective to first area)
        if(link.getOrigin().x() < link.getTarget().x())
            direction = LinkDirection::Right;
        else if(link.getOrigin().x() > link.getTarget().x())
            direction = LinkDirection::Left;
        else if(link.getOrigin().y() < link.getTarget().y())
            direction = LinkDirection::Up;
        else if(link.getOrigin().y() > link.getTarget().y())
            direction = LinkDirection::Down;

        switch(direction)
        {
        case LinkDirection::Right:
            first->setLinkRight(link);
            second->setLinkLeft(link);
            break;
        case LinkDirection::Left:
            first->setLinkLeft(link);
            second->setLinkRight(link);
            break;
        case LinkDirection::Up:
            first->setLinkUp(link);
            second->setLinkDown(link);
            break;
        case LinkDirection::Down:
            first->setLinkDown(link);
            second->setLinkUp(link);
            break;
        }
    }

    // Read in options
    QScopedPointer<Object> obj(new Object());
    space.options = SpaceGenerationOptions::Parse(obj.data());

    return space;
}

void Space::build(Table *data)
{
    data->clear(); // Clear any existing data

    // Build all zones
    for(QMap<QString,Zone>::iterator iter = zones.begin(); iter != zones.end(); iter++)
    {
        Object obj;
        iter.value().build(&obj);
        data->addObject(OBJ_ZONE, obj);
    }

    // Build all areas and compile a list of the links that need building. As with parsing,
    // areas currently subvert the quest data system as they manage their own tables for cell
    // data. Therefore the location of the space data is given to the build function in order
    // to manage this.
    QList<Link> linkList;
    for(auto iter = areas.begin(); iter != areas.end(); iter++)
    {
        Object obj;
        iter.value().build(&obj, QFileInfo(data->getFilePath()).absoluteDir().absolutePath());
        data->addObject(OBJ_AREA, obj);

        Link* right = iter.value().getLinkRight();
        Link* left = iter.value().getLinkLeft();
        Link* down = iter.value().getLinkDown();
        Link* up = iter.value().getLinkUp();

        if(right != nullptr && !linkList.contains(*right))
            linkList.append(*right);
        if(left != nullptr && !linkList.contains(*left))
            linkList.append(*left);
        if(down != nullptr && !linkList.contains(*down))
            linkList.append(*down);
        if(up != nullptr && !linkList.contains(*up))
            linkList.append(*up);
    }

    // Build all links
    for(Link link : linkList)
    {
        Object obj;
        link.build(&obj);
        data->addObject(OBJ_LINK, obj);
    }

    // Build options
    QScopedPointer<Object> obj(new Object());
    options.build(obj.data());
    data->addObject(OBJ_SPACE_OPTIONS, *obj.data());
}

bool Space::addZone(QString name, Zone zone)
{
    if(!zones.contains(name))
    {
        zones.insert(name, zone);
        return true;
    }
    else
        return false;
}

bool Space::removeZone(QString name)
{
    if(zones.contains(name))
    {
        zones.remove(name);
        return true;
    }
    else
        return false;
}

Zone* Space::getZone(QString name)
{
    auto iter = zones.find(name);
    if(iter == zones.end())
        return nullptr;
    else
        return &iter.value();
}

QList<Zone*> Space::getZoneList()
{
    QList<Zone*> zoneList;
    for(QMap<QString,Zone>::iterator iter = zones.begin(); iter != zones.end(); iter++)
        zoneList.append(&iter.value());
    return zoneList;
}

void Space::placeArea(Area area)
{
    // Ensure there is enough room in the grid for the new area.
    // - Expand X direction
    if(cells.length() < area.getLocation().x() + area.getWidth())
    {
        while(cells.length() < area.getLocation().x() + area.getWidth())
        {
            cells.append(QVector<GridCell>());
            while(cells[0].length() > cells.last().length())
                cells.last().append(GridCell());
        }
    }
    // - Expand Y direction
    if(cells[0].length() < area.getLocation().y() + area.getHeight())
    {
        for(int x = 0; x < cells.length(); x++)
        {
            while(cells[x].length() < area.getLocation().y() + area.getHeight())
                cells[x].append(GridCell());
        }
    }

    // Assign grid cells new ID
    for(int i = area.getLocation().x(); i < area.getLocation().x() + area.getWidth(); i++)
    {
        for(int j = area.getLocation().y(); j < area.getLocation().y() + area.getHeight(); j++)
            cells[i][j] = GridCell(area.getLocation());
    }

    // Insert area into area map
    areas.insert(area.getLocation(), area);
}

bool Space::areaFits(Area area)
{
    return areaFits(area.getLocation(), area.getWidth(), area.getHeight());
}

bool Space::areaFits(const QPoint& location, int width, int height)
{
    // If area is out of bounds, return false
    if(location.x() < 0 || location.y() < 0)
        return false;

    // The location may take the area off-grid (out of bounds), but this does not mean it does not fit, therefore only go as far as we can.
    int xLimit = location.x() + width;
    int yLimit = location.y() + height;
    if(xLimit > cells.count()-1) {
        xLimit = cells.count()-1;
    }
    if(yLimit > cells[0].count()-1) {
        yLimit = cells[0].count()-1;
    }

    for(int x = location.x(); x < xLimit; x++)
    {
        for(int y = location.y(); y < yLimit; y++)
        {
            // If cell contains an area, area will not fit into the map
            if(cells[x][y].containsArea())
                return false;
        }
    }

    return true;
}

bool Space::placeInDirection(const Area& baseArea, Area& newArea, Direction direction)
{
    QPoint currentLoc;

    int x = 0;
    int y = 0;

    bool validFound = false;
    switch(direction)
    {

    case SOUTH: // Placing SOUTH
        y = baseArea.getLocation().y() + baseArea.getHeight();
        for(x = baseArea.getLocation().x(); x < baseArea.getLocation().x() + baseArea.getWidth(); x++)
        {
            currentLoc.setX(x);
            currentLoc.setY(y);
            if(areaFits(currentLoc, newArea.getWidth(), newArea.getHeight()))
            {
                validFound = true;
                break;
            }
        }
        break;

    case EAST: // Placing EAST
        x = baseArea.getLocation().x() + baseArea.getWidth();
        for(y = baseArea.getLocation().y(); y < baseArea.getLocation().y() + baseArea.getHeight(); y++)
        {
            currentLoc.setX(x);
            currentLoc.setY(y);
            if(areaFits(currentLoc, newArea.getWidth(), newArea.getHeight()))
            {
                validFound = true;
                break;
            }
        }
        break;

    case NORTH: // Placing NORTH
        y = baseArea.getLocation().y() - newArea.getHeight();
        for(x = baseArea.getLocation().x(); x < baseArea.getLocation().x() + baseArea.getWidth(); x++)
        {
            currentLoc.setX(x);
            currentLoc.setY(y);
            if(areaFits(currentLoc, newArea.getWidth(), newArea.getHeight()))
            {
                validFound = true;
                break;
            }
        }

        break;
    case WEST: // Placing WEST
        x = baseArea.getLocation().x() - newArea.getWidth();
        for(y = baseArea.getLocation().y(); y < baseArea.getLocation().y() + baseArea.getHeight(); y++)
        {
            currentLoc.setX(x);
            currentLoc.setY(y);
            if(areaFits(currentLoc, newArea.getWidth(), newArea.getHeight()))
            {
                validFound = true;
                break;
            }
        }

        break;
    }

    if(validFound)
        newArea.setLocation(currentLoc);

    return validFound;
}

bool Space::areNeighbours(const Area& baseArea, const Area& area, Direction& out)
{
    int yTop = baseArea.getLocation().y() - 1;
    int yBottom = baseArea.getLocation().y() + baseArea.getHeight() + 1;
    int xLeft = baseArea.getLocation().x() - 1;
    int xRight = baseArea.getLocation().x() + baseArea.getWidth() + 1;

    // Check possibilities
    bool northPossible = true, southPossible = true, eastPossible = true, westPossible = true;
    if(yTop < 0)
        northPossible = false;
    if(yBottom > cells[0].count()-1)
        southPossible = false;
    if(xLeft < 0)
        westPossible = false;
    if(xRight > cells.count()-1)
        eastPossible = false;

    // Check north and south
    if(northPossible || southPossible)
    {
        for(int x = baseArea.getLocation().x(); x < baseArea.getLocation().x() + baseArea.getWidth(); x++)
        {
            if(northPossible && cells[x][yTop].getAreaOrigin() == area.getLocation())
            {
                out = Direction::NORTH;
                return true;
            }
            else if(southPossible && cells[x][yBottom].getAreaOrigin() == area.getLocation())
            {
                out = Direction::SOUTH;
                return true;
            }
        }
    }

    // Check east and west
    if(westPossible || eastPossible)
    {
        for(int y = baseArea.getLocation().y(); y < baseArea.getLocation().y() + baseArea.getHeight(); y++)
        {
            if(eastPossible && cells[xRight][y].getAreaOrigin() == area.getLocation())
            {
                out = Direction::EAST;
                return true;
            }
            else if(westPossible && cells[xLeft][y].getAreaOrigin() == area.getLocation())
            {
                out = Direction::WEST;
                return true;
            }
        }
    }

    return false;
}

QList<QPoint> Space::getNeighbours(const Area& area)
{
    QList<QPoint> neighbourLocations = QList<QPoint>();

    int yTop = area.getLocation().y() - 1;
    int yBottom = area.getLocation().y() + area.getHeight() + 1;
    int xLeft = area.getLocation().x() - 1;
    int xRight = area.getLocation().x() + area.getWidth() + 1;

    // Check possibilities
    bool northPossible = true, southPossible = true, eastPossible = true, westPossible = true;
    if(yTop < 0)
        northPossible = false;
    if(yBottom > cells[0].count()-1)
        southPossible = false;
    if(xLeft < 0)
        westPossible = false;
    if(xRight > cells.count()-1)
        eastPossible = false;

    // Check north and south
    if(northPossible || southPossible)
    {
        for(int x = area.getLocation().x(); x < area.getLocation().x() + area.getWidth(); x++)
        {
            if(northPossible && cells[x][yTop].containsArea())
            {
                if(!neighbourLocations.contains(cells[x][yTop].getAreaOrigin()))
                    neighbourLocations.append(cells[x][yTop].getAreaOrigin());
            }
            else if(southPossible && cells[x][yBottom].containsArea())
            {
                if(!neighbourLocations.contains(cells[x][yBottom].getAreaOrigin()))
                    neighbourLocations.append(cells[x][yBottom].getAreaOrigin());
            }
        }
    }

    // Check east and west
    if(westPossible || eastPossible)
    {
        for(int y = area.getLocation().y(); y < area.getLocation().y() + area.getHeight(); y++)
        {
            if(eastPossible && cells[xRight][y].containsArea())
            {
                if(!neighbourLocations.contains(cells[xRight][y].getAreaOrigin()))
                    neighbourLocations.append(cells[xRight][y].getAreaOrigin());
            }
            else if(westPossible && cells[xLeft][y].containsArea())
            {
                if(!neighbourLocations.contains(cells[xLeft][y].getAreaOrigin()))
                    neighbourLocations.append(cells[xLeft][y].getAreaOrigin());
            }
        }
    }

    return neighbourLocations;
}

Direction Space::getDirection(const Area& baseArea, const Area& otherArea)
{
    QPoint base = baseArea.getLocation();
    QPoint other = otherArea.getLocation();

    QPoint dir = other - base;

    if(dir.x() != 0)
    {
        if(dir.x() < 0)
            return Direction::WEST;
        else
            return Direction::EAST;
    }
    else
    {
        if(dir.y() < 0)
            return Direction::NORTH;
        else
            return Direction::SOUTH;
    }
}

bool Space::removeArea(int x, int y)
{
    if(cells[x][y].containsArea())
    {
        Area* area = &areas.find(QPoint(x,y)).value();

        for(int i = x; i < area->getWidth(); i++)
        {
            for(int j = y; j < area->getHeight(); j++)
            {
                cells[i][j].removeArea();
            }
        }

        areas.remove(QPoint(x,y));

        return true;
    }

    return false;
}

bool Space::removeArea(Area area)
{
    for(auto iter = areas.begin(); iter != areas.end(); iter++)
    {
        if(iter.value() == area)
        {
            areas.remove(iter.key());
            return true;
        }
    }

    return false;
}

GridCell* Space::getCell(int x, int y)
{
    if(x < cells.length() && y < cells[x].length())
        return &cells[x][y];

    return nullptr;
}

void Space::clear()
{
    areas.clear();
    for(int x = 0; x < cells.size(); x++)
    {
        for(int y = 0; y < cells[0].size(); y++)
        {
            cells[x][y] = GridCell();
        }
    }
}

bool Space::setStartingArea(QPoint areaOrigin, QPoint position)
{
    // Ensure this is a valid starting area before assigning it
    if(!areas.contains(areaOrigin))
        return false;

    Area* area = &areas.find(areaOrigin).value();

    if(position.x() < 0 && position.y() < 0 &&
            position.x() > area->getWidth() * AREA_TILE_SIZE && position.y() > area->getHeight() * AREA_TILE_SIZE)
        return false;

    startingArea = areaOrigin;
    startingLocation = position;

    return true;
}

QList<Map> Space::buildMaps()
{
    // Build all areas into maps.
    QMap<QPoint, Map> maps = QMap<QPoint, Map>();
    for(auto iter = areas.begin(); iter != areas.end(); iter++)
    {
        maps.insert(iter.key(), iter.value().buildMap());

        Map* map = &maps.last();

        if(iter.key() == startingArea)
            map->addEntity(new Destination(0, startingLocation.x() * map->getTileSize(), startingLocation.y() * map->getTileSize(), "start", Direction::EAST));
    }

    // Link all maps together by looping grid cells
    for(int x = 0; x < getWidth(); x++)
    {
        for(int y = 0; y < getHeight(); y++)
        {
            if(cells[x][y].containsArea())
            {
                // Retrieve the area and map associated with the area (by using area location).
                Area area = areas.find(cells[x][y].getAreaOrigin()).value();
                Map* map = &maps.find(area.getLocation()).value();

                // Retrieve all valid adjacent cells
                GridCell* left = nullptr;
                GridCell* right = nullptr;
                GridCell* up = nullptr;
                GridCell* down = nullptr;
                if(x - 1 >= 0)
                    left = &cells[x-1][y];
                if(y - 1 >= 0)
                    up = &cells[x][y-1];
                if(x + 1 < getWidth())
                    right = &cells[x+1][y];
                if(y + 1 < getHeight())
                    down = &cells[x][y+1];

                QVector<MapEntity*> transporters = QVector<MapEntity*>(); // collection of transporters that need adding to the map

                // If the there is a valid link to a different area, create a valid teletransporter to link the two cells
                if(left && left->containsArea() && area.getLinkLeft() != nullptr && left->getAreaOrigin() != area.getLocation())
                {
                    Map* destination = &maps.find(left->getAreaOrigin()).value();
                    int yPos = (AREA_TILE_SIZE*map->getTileSize()) * (y - area.getLocation().y());
                    int xPos = -SIDE_MAP_TRANSPORTER_SIZE;
                    transporters.append(new Teletransporter(xPos, yPos, SIDE_MAP_TRANSPORTER_SIZE, (AREA_TILE_SIZE*SIDE_MAP_TRANSPORTER_SIZE)*map->getTileSize(), destination->getName(), "_side",
                                                            Teletransporter::Transition::Scroll));
                }
                if(right && right->containsArea() && area.getLinkRight() != nullptr  && right->getAreaOrigin() != area.getLocation())
                {
                    Map* destination = &maps.find(right->getAreaOrigin()).value();
                    int yPos = (AREA_TILE_SIZE*map->getTileSize()) * (y - area.getLocation().y());
                    int xPos = (map->getWidth()*map->getTileSize());
                    transporters.append(new Teletransporter(xPos, yPos, SIDE_MAP_TRANSPORTER_SIZE, (AREA_TILE_SIZE*SIDE_MAP_TRANSPORTER_SIZE)*map->getTileSize(), destination->getName(), "_side",
                                                            Teletransporter::Transition::Scroll, "entities/teletransporter"));
                }
                if(down && down->containsArea() && area.getLinkDown() != nullptr  && down->getAreaOrigin() != area.getLocation())
                {
                    Map* destination = &maps.find(down->getAreaOrigin()).value();
                    int yPos = (map->getHeight()*map->getTileSize());
                    int xPos = (AREA_TILE_SIZE*map->getTileSize()) * (x - area.getLocation().x());
                    transporters.append(new Teletransporter(xPos, yPos, (AREA_TILE_SIZE*SIDE_MAP_TRANSPORTER_SIZE)*map->getTileSize(), SIDE_MAP_TRANSPORTER_SIZE, destination->getName(), "_side",
                                                            Teletransporter::Transition::Scroll));
                }
                if(up && up->containsArea() && area.getLinkUp() != nullptr && up->getAreaOrigin() != area.getLocation())
                {
                    Map* destination = &maps.find(up->getAreaOrigin()).value();
                    int yPos = -SIDE_MAP_TRANSPORTER_SIZE;
                    int xPos = (AREA_TILE_SIZE*map->getTileSize()) * (x - area.getLocation().x());
                    transporters.append(new Teletransporter(xPos, yPos, (AREA_TILE_SIZE*SIDE_MAP_TRANSPORTER_SIZE)*map->getTileSize(), SIDE_MAP_TRANSPORTER_SIZE, destination->getName(), "_side",
                                                            Teletransporter::Transition::Scroll));
                }

                // Add all transporters to the map.
                for(MapEntity* transporter : transporters)
                    map->addEntity(transporter);
            }
        }
    }

    QList<Map> mapList = QList<Map>();
    for(auto iter = maps.begin(); iter != maps.end(); iter++)
    {
        mapList.append(iter.value());
    }
    return mapList;
}

void Space::missionUpdated()
{
    clear();
    emitUpdate();
}

void Space::setMission(Mission* mission)
{
    receiver = new SpaceReceiver(mission, this);
}

int Space::randomAreaWidth()
{
    return rand.randomInteger(options.getMinimumAreaWidth(), options.getMaximumAreaWidth());
}

int Space::randomAreaHeight()
{
    return rand.randomInteger(options.getMinimumAreaHeight(), options.getMaximumAreaHeight());
}
