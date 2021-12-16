#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <vector>

using std::vector;

typedef struct
{
    uint8_t tag[8];
    uint32_t demoProtocol;
    uint32_t netProtocol;
    uint8_t  mapName[260];
    uint8_t  modName[260];
    int32_t  mapCrc;
    uint32_t dirOffset;
} DemoHeader;

typedef struct
{
    uint32_t id;
    uint8_t name[64];
    uint32_t flags;
    uint32_t cdTrack;
    float time;
    uint32_t frames;
    uint32_t offset;
    uint32_t length;
} DemoDirectory;

typedef struct
{
    float x, y, z;
} vec3;

#define READ(x) { k = fread(&x, sizeof(x), 1, input); if(k != 1) { printf("Couldn't read, line %d\n", __LINE__); exit(1); } }
#define MOVETO(n) { fseek(input, n, SEEK_SET); }
#define SKIP(n) { fseek(input, n, SEEK_CUR); }

typedef struct
{
    float realTime;
    vec3 position;
} GhostNode;

typedef struct
{
    uint8_t mapName[260];
    float realTime;
    uint32_t begin;
    uint32_t end;
} GhostEntry;

typedef struct
{
    uint32_t id;
    uint32_t numEntries;
} GhostHeader;

/*
    Ghost file spec:

    GhostHeader     header;
    GhostEntry      entries[header.numEntries];
    GhostNode       nodes[?];
*/

void process_demo(FILE *input, vector<GhostEntry> &entries, vector<GhostNode> &nodes, float &realTime)
{
    int k;
    GhostEntry entry;

    DemoHeader header;
    READ(header);

    strcpy((char*)entry.mapName, (const char*)header.mapName);
    entry.realTime = realTime;
    entry.begin = nodes.size();

    MOVETO(header.dirOffset);

    uint32_t numDirs;
    READ(numDirs);

    DemoDirectory dir;
    bool playbackFound = false;

    for(int i = 0; i < numDirs; i++)
    {
        READ(dir);
        if(0 == strcmp("Playback", (const char*)dir.name))
        {
            playbackFound = true;
            break;
        }
    }

    if(!playbackFound)
    {
        printf("Playback not found!\n");
        exit(1);
    }

    MOVETO(dir.offset);

    bool end = false;
    while(!end)
    {
        uint8_t type;
        float time;
        uint32_t frame;

        READ(type);
        READ(time);
        READ(frame);

        switch(type)
        {
            case 0:
            case 1:
            {
                SKIP(4);
                vec3 position;
                READ(position);
                SKIP(48);
                float frameTime;
                READ(frameTime);
                nodes.push_back({realTime, position});
                realTime += frameTime;
                SKIP(396);
                uint32_t frameDataLength;
                READ(frameDataLength);
                SKIP(frameDataLength);             
            }
            break;
            case 2: break;
            case 3: SKIP(64); break;
            case 4: SKIP(32); break;
            case 5: end = true; break;
            case 6: SKIP(84); break;
            case 7: SKIP(8); break;
            case 8:
            {
                SKIP(4);
                uint32_t sampleSize;
                READ(sampleSize);
                SKIP(sampleSize);
                SKIP(16);
            }
            break;
            case 9:
            {
                uint32_t num;
                READ(num);
                SKIP(num);
            }
            break;
        }
    }

    entry.end = nodes.size();
    entries.push_back(entry);
}

void process_main(const char *base_name, const char *output_name)
{
    FILE *output = fopen(output_name, "wb");

    if(!output)
    {
        printf("Could not write file %s\n", output_name);
        exit(1);
    }

    vector<GhostEntry> entries;
    vector<GhostNode> nodes;
    float realTime = 0;

    int numDemos = 0;

    for(int dem = 1; true; dem++)
    {
        char input_name[260];
        sprintf(input_name, "%s_%d.dem", base_name, dem);

        FILE *input = fopen(input_name, "rb");
        if(!input) break;
        process_demo(input, entries, nodes, realTime);
        fclose(input);

        numDemos++;
    }

    GhostHeader header = {1337, (uint32_t)entries.size()};

    int k;
    k = fwrite(&header, sizeof(GhostHeader), 1, output);
    k = fwrite(entries.data(), sizeof(GhostEntry), entries.size(), output);
    k = fwrite(nodes.data(), sizeof(GhostNode), nodes.size(), output);

    fclose(output);
}

int main(int argc, char *argv[])
{
    process_main("./demos/run", "output.ghost");
}