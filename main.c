#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct
{
    uint8_t tag[8];
    uint32_t demoProtocol;
    uint32_t netProtocol;
    uint8_t  mapName[260];
    uint8_t  modName[260];
    int32_t  mapCrc; //unknown
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

typedef struct
{
    int32_t a, b, c, d;
} quad;

typedef struct
{
    uint32_t unused0;
    vec3 position, orientation, forward, right, up;
    float frameTime;
    float time;
    int32_t intermission;
    int32_t paused;
    int32_t spectator;
    int32_t onground;
    int32_t waterlevel;
    vec3 velocity;
    vec3 origin;
    vec3 viewHeight;
    float idealPitch;
    vec3 viewAngles;
    int32_t health;
    vec3 crosshairAngle;
    float viewSize;
    vec3 punchAngle;
    int32_t maxClients;
    int32_t viewEntity;
    int32_t playerCount;
    int32_t maxEntities;
    int32_t demoPlayback;
    int32_t hardware;
    int32_t smoothing;
    int32_t ptr_cmd;
    int32_t ptr_movevars;
    quad viewport;
    int32_t nextView;
    int32_t onlyClientDraw;

    int16_t lerp_msec;
    uint8_t msec;
    uint8_t unused1;
    vec3 viewAngles2;
    float forwardMove;
    float sideMove;
    float upMove;
    int8_t lightLevel;
    uint8_t unused2;
    uint16_t buttons;
    int8_t impulse;
    int8_t weaponSelect;
    uint16_t unused3;
    int32_t impactIndex;
    vec3 impactPosition;

    float gravity;
    float stopSpeed;
    float maxSpeed;
    float spectatorMaxSpeed;
    float acceleration;
    float airAcceleration;
    float waterAcceleration;
    float friction;
    float edgeFriction;
    float waterFriction;
    float entityGravity;
    float bounce;
    float stepSize;
    float maxVelocity;
    float zMax;
    float waveHeight;
    int32_t footsteps;
    uint8_t skyName[32];
    float rollAngle;
    float rollSpeed;
    vec3 skyColor;
    vec3 skyVec;

    vec3 view;
    int32_t modelView;
    int32_t incoming_sequence;
    int32_t incoming_acknowledged;
    int32_t incoming_reliable_acknowledged;
    int32_t incoming_reliable_sequence;
    int32_t outgoing_sequence;
    int32_t reliable_sequence;
    int32_t last_reliable_sequence;

    uint32_t frameDataLength;

} DemoBulk;

typedef struct
{
    float realTime;
    vec3 position;
} GhostNode;

#define READ(x) { k = fread(&x, sizeof(x), 1, input); if(k != 1) { printf("Couldn't read, line %d\n", __LINE__); exit(1); } }
#define MOVETO(n) { fseek(input, n, SEEK_SET); }
#define SKIP(n) { fseek(input, n, SEEK_CUR); }

float process_demo(FILE *input, FILE *output, float realTime)
{
    DemoHeader header;
    int k;

    READ(header);

    MOVETO(header.dirOffset);

    uint32_t numDirs;

    READ(numDirs);

    char playbackFound = 0;

    DemoDirectory dir;

    for(int i = 0; i < numDirs; i++)
    {
        READ(dir);

        if(0 == strcmp("Playback", dir.name))
        {
            playbackFound = 1;
            break;
        }
    }

    if(!playbackFound)
    {
        printf("Playback not found!\n");
        exit(1);
    }

    MOVETO(dir.offset);

    char end = 0;

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
                DemoBulk db;
                READ(db);
                SKIP(db.frameDataLength);

                GhostNode g;
                g.realTime = realTime;
                g.position = db.position;

                int k = fwrite(&g, sizeof(g), 1, output);

                realTime += db.frameTime;
            }
            break;
            case 2: break;
            case 3: SKIP(64); break;
            case 4: SKIP(32); break;
            case 5: end = 1; break;
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

    return realTime;
}

void process_main(const char *base_name, const char *output_name)
{
    int numDemos = 0;

    FILE *output = fopen(output_name, "wb");

    if(!output)
    {
        printf("Could not write file %s\n", output_name);
        exit(1);
    }

    float totalTime = 0;

    while(1)
    {
        char input_name[1024];
        sprintf(input_name, "%s_%d.dem", base_name, numDemos+1);

        FILE *input = fopen(input_name, "rb");
        if(!input) break; //no more demos

        printf("Processing demo file: %s\n", input_name);

        float realTime = 0;
        realTime = process_demo(input, output, realTime);
        fclose(input);

        numDemos++;
    }

    printf("%d demos processed\n", numDemos);
}

int main(int argc, char *argv[])
{
    process_main("./demos/run", "output.ghost");
}