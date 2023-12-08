#ifndef MAP_H
#define MAP_H

#define MAX_KEYS 100

typedef struct
{
    char key[50];
    // int value;
    unsigned char* value;
} KeyValuePair;

typedef struct
{
    KeyValuePair data[MAX_KEYS];
    int size;
} SimpleMap;

void initializeMap(SimpleMap *map);
void insertKeyValuePair(SimpleMap *map, const char *key, unsigned char* value);
unsigned char* getValueByKey(const SimpleMap *map, const char *key);

#endif // MAP_H
