#include "map.h"
#include <stdio.h>
#include <string.h>

void initializeMap(SimpleMap *map)
{
    map->size = 0;
}

void insertKeyValuePair(SimpleMap *map, const char *key, unsigned char* value)
{
    if (map->size < MAX_KEYS)
    {
        KeyValuePair pair;
        strncpy(pair.key, key, sizeof(pair.key) - 1);
        pair.key[sizeof(pair.key) - 1] = '\0';
        pair.value = value;

        map->data[map->size++] = pair;
    }
    else
    {
        printf("Map is full. Cannot insert more elements.\n");
    }
}

unsigned char* getValueByKey(const SimpleMap *map, const char *key)
{
    for (int i = 0; i < map->size; ++i)
    {
        if (strcmp(map->data[i].key, key) == 0)
        {
            return map->data[i].value;
        }
    }

    return "";
}
