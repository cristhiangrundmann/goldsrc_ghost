#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <vector>

#define ERROR { printf("Error: %s: %d\n", __FILE__, __LINE__); exit(1); }

using std::vector;

struct DemoHeader
{
    uint8_t tag[8];
    uint32_t demoProtocol;
    uint32_t netProtocol;
    uint8_t  mapName[260];
    uint8_t  modName[260];
    int32_t  mapCrc;
    uint32_t dirOffset;
};

struct DemoDirectory
{
    uint32_t id;
    uint8_t name[64];
    uint32_t flags;
    uint32_t cdTrack;
    float time;
    uint32_t frames;
    uint32_t offset;
    uint32_t length;
};

struct vec3
{
    float x, y, z;
};

struct GhostNode
{
    float realTime;
    vec3 position;
};

struct GhostEntry
{
    uint8_t mapName[260];
    float realTime;
    uint32_t begin;
    uint32_t end;
};

template <class T>
T READ(uint8_t *data, uint32_t &offset)
{
    T *ptr = (T*)&data[offset];
    offset += sizeof(T);
    return *ptr;
}

uint8_t *read_file(const char *filename)
{
    FILE *input = fopen(filename, "rb");
    if(!input) return NULL;

    fseek(input, 0, SEEK_END);
    int size = ftell(input);
    fseek(input, 0, SEEK_SET);

    uint8_t *data = new uint8_t[size];
    if(1 != fread(data, size, 1, input)) ERROR;

    fclose(input);
    return data;
}

class Ghost
{
public:
    vector<GhostEntry> entries;
    vector<GhostNode> nodes;
    uint32_t curEntry;
    uint32_t curNode;

private:
    void process_demo(uint8_t *data, float &realTime)
    {
        uint32_t offset = 0;
        DemoHeader header = READ<DemoHeader>(data, offset);

        GhostEntry entry;
        strcpy((char*)entry.mapName, (const char*)header.mapName);
        entry.realTime = realTime;
        entry.begin = nodes.size();


        offset = header.dirOffset;
        uint32_t numDirs = READ<uint32_t>(data, offset);
        DemoDirectory dir;

        bool playbackFound = false;
        for(int i = 0; i < numDirs; i++)
        {
            dir = READ<DemoDirectory>(data, offset);
            if(0 == strcmp("Playback", (const char*)dir.name))
            {
                playbackFound = true;
                break;
            }
        }

        if(!playbackFound) ERROR;

        offset = dir.offset;

        bool end = false;
        while(!end)
        {
            uint8_t type = READ<uint8_t>(data, offset);
            float time = READ<float>(data, offset);
            uint32_t frame = READ<uint32_t>(data, offset);

            switch(type)
            {
                case 0:
                case 1:
                {
                    offset += 4;
                    vec3 position = READ<vec3>(data, offset);
                    offset += 48;
                    float frameTime = READ<float>(data, offset);
                    nodes.push_back({realTime, position});
                    realTime += frameTime;
                    offset += 396;
                    uint32_t frameDataLength = READ<uint32_t>(data, offset);
                    offset += frameDataLength;
                }
                break;
                case 2: break;
                case 3: offset += 64; break;
                case 4: offset += 32; break;
                case 5: end = true; break;
                case 6: offset += 84; break;
                case 7: offset += 8; break;
                case 8:
                {
                    offset += 4;
                    uint32_t sampleSize = READ<uint32_t>(data, offset);
                    offset += sampleSize + 16;
                }
                break;
                case 9:
                {
                    uint32_t num = READ<uint32_t>(data, offset);
                    offset += num;
                }
                break;
            }
        }

        if(nodes.size() == 0) ERROR;
        entry.end = nodes.size()-1;
        entries.push_back(entry);
    }

public:
    Ghost process_main(const char *base_name, const char *output_name)
    {
        entries.clear();
        nodes.clear();
        Ghost ghost;
        float realTime = 0;

        for(int dem = 1; true; dem++)
        {
            char input_name[260];
            sprintf(input_name, "%s_%d.dem", base_name, dem);
            uint8_t *data = read_file(input_name);
            
            if(data == NULL) break; //end of demos

            if(dem == 2) process_demo(data, realTime);

            if(data) delete[] data;
        }

        return ghost;
    }

    bool getNode(float realTime, const char *mapName)
    {
        if(nodes.size() < 1) return false;
        if(entries.size() < 1) return false;

        //binary search
        uint32_t e0 = 0;
        uint32_t e1 = entries.size()-1;
        while(e1 - e0 > 1)
        {
            //printf("< %s, %s >\n", entries[e0].mapName, entries[e1].mapName);
            uint32_t mid = (e0+e1)/2;
            float midTime = entries[mid].realTime;
            if(realTime < midTime) e1 = mid; else e0 = mid;
        }
        curEntry = e0;
        //printf("Map: %s\n", entries[curEntry].mapName);

        //if(0 != strcmp(mapName, entries[curEntry].mapName)) return false;

        //binary search
        uint32_t n0 = entries[curEntry].begin;
        uint32_t n1 = entries[curEntry].end;
        while(n1 - n0 > 1)
        {
            //printf("< %g, %g >\n", nodes[n0].realTime, nodes[n1].realTime);
            uint32_t mid = (n0+n1)/2;
            float midTime = nodes[mid].realTime;
            if(realTime < midTime) n1 = mid; else n0 = mid;
        }
        curNode = n0;
        return true;
    }
};

int main(int argc, char *argv[])
{
    Ghost g;
    g.process_main("./demos/run", "output.ghost");

    for(int i = 0; i < g.entries.size(); i++)
    {
        //printf("%s\n", g.entries[i].mapName);
    }

    bool b = g.getNode(60*9+17, "a");
}