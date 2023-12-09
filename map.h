#ifndef MAP_H
#define MAP_H

#define MAX_KEYS 100

typedef struct
{
    // unsigned char key[50];
    // unsigned char* key;
    int key;
    // int value;
    void *value;
} KeyValuePair;

typedef struct
{
    KeyValuePair data[MAX_KEYS];
    int size;
} SimpleMap;

void initializeMap(SimpleMap *map);
// void insertKeyValuePair(SimpleMap *map, const unsigned char *key, void *value, int keyLen);
void insertKeyValuePair(SimpleMap *map, const int key, void *value, int keyLen);
void *getValueByKey(const SimpleMap *map, const int key);
void removeByKey(SimpleMap *map, const int key);

#endif // MAP_H
