#include "map.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void initializeMap(SimpleMap *map)
{
    map->size = 0;
}

void insertKeyValuePair(SimpleMap *map, const int key, void *value, int keyLen)
{
    if (map->size < MAX_KEYS)
    {
        KeyValuePair* pair = malloc(sizeof(KeyValuePair));
        // if (keyLen == -1)
        // {
        //     strncpy(pair.key, key, sizeof(pair.key) - 1);
        //     pair.key[sizeof(pair.key) - 1] = '\0';
        // }
        // else
        // {
        //     strncpy(pair.key, key, keyLen);
        //     pair.key[keyLen] = '\0';
        // }
        pair->key = key;
        pair->value = value;

        map->data[map->size++] = pair;
    }
    else
    {
        printf("Map is full. Cannot insert more elements.\n");
    }
}

void *getValueByKey(const SimpleMap *map, const int key)
{
    for (int i = 0; i < map->size; ++i)
    {
        // if (strcmp(map->data[i].key, key) == 0)
        if (map->data[i]->key - key == 0)
        {
            return map->data[i]->value;
        }
    }

    return NULL;
}

void removeByKey(SimpleMap *map, const int key)
{
    for (int i = 0; i < map->size; ++i)
    {
        // if (strcmp(map->data[i].key, key) == 0)
        if (map->data[i]->key - key == 0)
        {
            for (int j = i; j < map->size - 1; ++j)
            {
                map->data[j] = map->data[j + 1];
            }
            map->size--;
            free(map->data[i]->value);
            free(map->data[i]);
            return;
        }
    }

    printf("not found\n");
}
