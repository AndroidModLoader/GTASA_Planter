#include <mod/amlmod.h>
#include <mod/logger.h>

#ifdef AML32
    #include "GTASA_STRUCTS.h"
    #define BYVER(__for32, __for64) (__for32)
#else
    #include "GTASA_STRUCTS_210.h"
    #define BYVER(__for32, __for64) (__for64)
#endif
#include "isautils.h"

MYMOD(net.rusjj.plantes, GTASA Planter, 1.5, RusJJ)
NEEDGAME(com.rockstargames.gtasa)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.aml, 1.2.1)
END_DEPLIST()

uintptr_t pGTASA;
void* hGTASA;
ISAUtils* sautils;

#define MAXLOCTRIS              4
#define GRASS_MODELS_TAB        4
#define DEFAULT_GRASS_DISTANCE  1
#define DEFAULT_GRASS_LOD       2


// ---------------------------------------------------------------------------------------


const char* grassMdls1[GRASS_MODELS_TAB] =
{
    "models\\grass\\grass0_1.dff",
    "models\\grass\\grass0_2.dff",
    "models\\grass\\grass0_3.dff",
    "models\\grass\\grass0_4.dff",
};
const char* grassMdls2[GRASS_MODELS_TAB] =
{
    "models\\grass\\grass1_1.dff",
    "models\\grass\\grass1_2.dff",
    "models\\grass\\grass1_3.dff",
    "models\\grass\\grass1_4.dff",
};
const char* grassMdls1LOD[GRASS_MODELS_TAB] =
{
    "models\\grass\\LOD\\grass0_1_LOD.dff",
    "models\\grass\\LOD\\grass0_2_LOD.dff",
    "models\\grass\\LOD\\grass0_3_LOD.dff",
    "models\\grass\\LOD\\grass0_4_LOD.dff",
};
const char* grassMdls2LOD[GRASS_MODELS_TAB] =
{
    "models\\grass\\LOD\\grass1_1_LOD.dff",
    "models\\grass\\LOD\\grass1_2_LOD.dff",
    "models\\grass\\LOD\\grass1_3_LOD.dff",
    "models\\grass\\LOD\\grass1_4_LOD.dff",
};
const char* aGrassDistanceSwitch[6] = 
{
    "FED_FXL",
    "FED_FXM",
    "FED_FXH",
    "FED_FXV",
    "ULTRA",
    "FPS KILLER",
};
const char* aGrassLODSwitch[4] = 
{
    "FED_FXL",
    "LOD",
    "LOD+",
    "FED_FXH",
};
RwTexture* tex_gras07Si;
float fGrassMinDistance, fGrassMidDistance, fGrassMid2Distance, fGrassDistance, fGrassFadeDist;
float fGrassMinDistanceSqr, fGrassMidDistanceSqr, fGrassMid2DistanceSqr, fGrassDistanceSqr, fGrassFadeDistSqr;
RpAtomic* PC_PlantModelsTab0LOD[4];
RpAtomic* PC_PlantModelsTab1LOD[4];
bool g_bHasLODs = false, g_bHasRegular = false;
int grassLODify = DEFAULT_GRASS_LOD;


// ---------------------------------------------------------------------------------------


CCamera*            TheCamera;
CPlantSurfProp**    m_SurfPropPtrTab;
uint32_t*           m_countSurfPropsAllocated;
CPlantSurfProp      m_SurfPropTab[MAX_SURFACE_PROPS];
RwTexture**         PC_PlantSlotTextureTab[4];
RwTexture*          PC_PlantTextureTab0[4];
RwTexture*          PC_PlantTextureTab1[4];
RpAtomic**          PC_PlantModelSlotTab[4];
RpAtomic*           PC_PlantModelsTab0[4];
RpAtomic*           PC_PlantModelsTab1[4];
CPlantLocTri**      m_CloseLocTriListHead;
CPlantLocTri**      m_UnusedLocTriListHead;
float*              m_fDNBalanceParam;

#define             LOCTRIS_LIMIT    512 // default is 256

CPlantLocTri        m_LocTrisTab_NEW[LOCTRIS_LIMIT];


// ---------------------------------------------------------------------------------------


void                (*RwRenderStateSet)(RwRenderState, void*);
void                (*FileMgrSetDir)(const char* dir);
FILE*               (*FileMgrOpenFile)(const char* dir, const char* ioflags);
void                (*FileMgrCloseFile)(FILE*);
char*               (*FileLoaderLoadLine)(FILE*);
unsigned short      (*GetSurfaceIdFromName)(void* trash, const char* surfname);
RwTexture*          (*GetTextureFromTextureDB)(const char* texture);
int                 (*SetPlantModelsTab)(unsigned int, RpAtomic**);
int                 (*SetCloseFarAlphaDist)(float, float);
void                (*FlushTriPlantBuffer)();
void                (*RpGeometryLock)(RpGeometry*, int);
void                (*RpGeometryUnlock)(RpGeometry*);
RpGeometry*         (*RpGeometryCreate)(int, int, unsigned int);
RpMaterial*         (*RpGeometryTriangleGetMaterial)(RpGeometry*, RpTriangle*);
void                (*RpGeometryTriangleSetMaterial)(RpGeometry*, RpTriangle*, RpMaterial*);
void                (*RpAtomicSetGeometry)(RpAtomic*, RpGeometry*, unsigned int);
RwStream*           (*RwStreamOpen)(int, int, const char*);
bool                (*RwStreamFindChunk)(RwStream*, int, int, int);
RpClump*            (*RpClumpStreamRead)(RwStream*);
void                (*RwStreamClose)(RwStream*, int);
RpAtomic*           (*GetFirstAtomic)(RpClump*);
void                (*SetFilterModeOnAtomicsTextures)(RpAtomic*, int);
void                (*RpMaterialSetTexture)(RpMaterial*, RwTexture*);
RpAtomic*           (*RpAtomicClone)(RpAtomic*);
void                (*RpClumpDestroy)(RpClump*);
RwFrame*            (*RwFrameCreate)();
void                (*RpGeometryForAllMaterials)(RpGeometry*, RpMaterial* (*)(RpMaterial*, void*), void*);
void                (*RpAtomicSetFrame)(RpAtomic*, RwFrame*);
bool                (*IsSphereVisibleForCamera)(CCamera*, const CVector*, float);
void                (*AddTriPlant)(PPTriPlant*, unsigned int);
CPlayerPed*         (*FindPlayerPed)(int playerId);


// ---------------------------------------------------------------------------------------


inline void RecalcGrassVars()
{
    fGrassFadeDist =     0.92f / (fGrassDistance - fGrassMinDistance);
    fGrassMidDistance =  0.75f * fGrassMinDistance;
    fGrassMid2Distance = 0.75f * fGrassMinDistance + 0.5f * (fGrassDistance - fGrassMinDistance);

    fGrassDistanceSqr =     fGrassDistance     * fGrassDistance;
    fGrassMinDistanceSqr =  fGrassMinDistance  * fGrassMinDistance;
    fGrassFadeDistSqr =     fGrassFadeDist     * fGrassFadeDist;
    fGrassMidDistanceSqr =  fGrassMidDistance  * fGrassMidDistance;
    fGrassMid2DistanceSqr = fGrassMid2Distance * fGrassMid2Distance;
}
inline void PatchGrassDistance(bool revert)
{
#ifdef AML32
    if(revert)
    {
        aml->Write8(pGTASA + 0x2CE4AE + 0x3, 0x21);
        aml->WriteFloat(pGTASA + 0x2CEA80, 100.0f * 100.0f);
        aml->WriteFloat(pGTASA + 0x2CE720, 100.0f * 100.0f);
    }
    else
    {
        aml->Write8(pGTASA + 0x2CE4AE + 0x3, 0x31); // 100 -> 400
        aml->WriteFloat(pGTASA + 0x2CEA80, 400.0f * 400.0f); // 100^2 -> 400^2
        aml->WriteFloat(pGTASA + 0x2CE720, 400.0f * 400.0f); // 100^2 -> 400^2
    }
#else
    if(revert)
    {
        aml->Write32(pGTASA + 0x390190, 0x90001D4B);
        aml->Write32(pGTASA + 0x390194, 0xBD4F6D68);
        aml->Write32(pGTASA + 0x390528, 0xD0001D6C);
        aml->Write32(pGTASA + 0x390538, 0xBD479989);
        aml->Write32(pGTASA + 0x3902E4, 0xD0001D68);
        aml->Write32(pGTASA + 0x3902EC, 0xBD479908);
    }
    else
    {
        aml->Write32(pGTASA + 0x390190, 0xD0001D4B); // 100 -> 400
        aml->Write32(pGTASA + 0x390194, 0xBD4C1568);
        aml->Write32(pGTASA + 0x390528, 0xF0FFFFEC); // 100^2 -> 400^2
        aml->Write32(pGTASA + 0x390538, 0xBD40D189);
        aml->Write32(pGTASA + 0x3902E4, 0xF0FFFFE8); // 100^2 -> 400^2
        aml->Write32(pGTASA + 0x3902EC, 0xBD40D108);
    }
#endif
}
void OnGrassDistanceChanged(int oldVal, int newVal, void* data)
{
    clampint(0, 5, &newVal);
    switch(newVal)
    {
        default:
            PatchGrassDistance(true);
            fGrassMinDistance = 16.0f;
            fGrassDistance = 25.0f;
            break;
            
        case 1:
            PatchGrassDistance(true);
            fGrassMinDistance = 21.0f;
            fGrassDistance = 52.0f;
            break;

        case 2:
            PatchGrassDistance(true);
            fGrassMinDistance = 45.0f;
            fGrassDistance = 80.0f;
            break;

        case 3:
            PatchGrassDistance(true);
            fGrassMinDistance = 63.0f;
            fGrassDistance = 110.0f;
            break;

        case 4:
            PatchGrassDistance(false);
            fGrassMinDistance = 120.0f;
            fGrassDistance = 170.0f;
            break;

        case 5:
            PatchGrassDistance(false);
            fGrassMinDistance = 350.0f;
            fGrassDistance = 400.0f;
            break;
    }

    SetCloseFarAlphaDist(3.0f, fGrassDistance);
    RecalcGrassVars();
    aml->MLSSetInt("GRSQUAL", newVal);
}
void OnGrassLODChanged(int oldVal, int newVal, void* data)
{
    clampint(0, 3, &newVal);
    grassLODify = newVal;
    aml->MLSSetInt("GRSLOD", newVal);
}


// ---------------------------------------------------------------------------------------


inline CPlantSurfProp* GetSurfacePtr(unsigned short index)
{
    if( index >= MAX_SURFACES )
    {
        logger->Error("Surface index (%d) is wrong!", index);
        return NULL;
    }
    return m_SurfPropPtrTab[index];
}
inline CPlantSurfProp* AllocSurfacePtr(unsigned short index)
{
    if ( *m_countSurfPropsAllocated < MAX_SURFACE_PROPS )
    {
        m_SurfPropPtrTab[index] = &m_SurfPropTab[(*m_countSurfPropsAllocated)++];
        return m_SurfPropPtrTab[index];
    }
    return NULL;
}
inline void PlantSurfPropMgrLoadPlantsDat(const char* filename)
{
    logger->Info("Parsing %s...", filename);
    FileMgrSetDir("DATA");
    FILE* f = FileMgrOpenFile(filename, "r");
    FileMgrSetDir("");

    char* i, *token; int line = 0;
    unsigned short SurfaceIdFromName = 0, nPlantCoverDefIndex = 0;
    CPlantSurfProp* SurfacePtr;
    CPlantSurfPropPlantData* ActiveSurface;

    while((i = FileLoaderLoadLine(f)) != NULL)
    {
      NextLineBuddy:
        ++line;
        if(!strcmp(i, ";the end")) break;
        if(*i == ';') continue; // if ';'

        int proceeded = 0;
        token = strtok(i, " \t");
        while(token != NULL && proceeded < 18)
        {
            switch(proceeded)
            {
                case 0:
                    SurfaceIdFromName = GetSurfaceIdFromName(NULL, token);
                    if(SurfaceIdFromName != 0) // 0 is "DEFAULT" or unknown
                    {
                        SurfacePtr = GetSurfacePtr(SurfaceIdFromName);
                        if(SurfacePtr != NULL ||
                          (SurfacePtr = AllocSurfacePtr(SurfaceIdFromName)) != NULL) break;
                        logger->Error("SurfacePtr (%s) (%d, line %d) is NULL!", token, SurfaceIdFromName, line);
                    }
                    else
                    {
                        logger->Error("Unknown surface name '%s' in 'plants.dat' (line %d)! See Andrzej to fix this.", token, line);
                        goto NextLineBuddy; // Just let it pass all surfaces (why do we need to stop it actually?)
                    }
                    goto CloseFile;

                case 1:
                    nPlantCoverDefIndex = atoi(token); // 0 or 1 or 2 (cant be any other)
                    if(nPlantCoverDefIndex >= MAX_SURFACE_DEFINDEXES) nPlantCoverDefIndex = 0;
                    ActiveSurface = &SurfacePtr->m_aSurfaceProperties[nPlantCoverDefIndex];
                    break;

                case 2:
                    SurfacePtr->m_nSlotID = atoi(token);
                    break;

                case 3:
                    ActiveSurface->m_nModelId = atoi(token);
                    break;

                case 4:
                    ActiveSurface->m_nUVOffset = atoi(token);
                    break;

                case 5:
                    ActiveSurface->m_Color.r = atoi(token);
                    break;

                case 6:
                    ActiveSurface->m_Color.g = atoi(token);
                    break;

                case 7:
                    ActiveSurface->m_Color.b = atoi(token);
                    break;

                case 8:
                    ActiveSurface->m_Intensity = atoi(token);
                    break;

                case 9:
                    ActiveSurface->m_IntensityVariation = atoi(token);
                    break;

                case 10:
                    ActiveSurface->m_Color.a = atoi(token);
                    break;

                case 11:
                    ActiveSurface->m_fSizeScaleXY = atof(token);
                    break;

                case 12:
                    ActiveSurface->m_fSizeScaleZ = atof(token);
                    break;

                case 13:
                    ActiveSurface->m_fSizeScaleXYVariation = atof(token);
                    break;

                case 14:
                    ActiveSurface->m_fSizeScaleZVariation = atof(token);
                    break;

                case 15:
                    ActiveSurface->m_fWindBendingScale = atof(token);
                    break;

                case 16:
                    ActiveSurface->m_fWindBendingVariation = atof(token);
                    break;

                case 17:
                    ActiveSurface->m_fDensity = atof(token);
                    break;
            }
            ++proceeded;
            token = strtok(NULL, " \t");
        }
    }

  CloseFile:
    FileMgrCloseFile(f);
}
inline void AtomicCreatePrelitIfNeeded(RpAtomic* atomic)
{
    RpGeometry* geometry = atomic->geometry;
    if((geometry->flags & rpGEOMETRYPRELIT) != 0) return;
    RwInt32 numTriangles = geometry->numTriangles;
    RwInt32 numVertices = geometry->numVertices;
    
    RpGeometry* newometry = RpGeometryCreate(numVertices, geometry->numTriangles, rpGEOMETRYPRELIT | rpGEOMETRYTEXTURED | rpGEOMETRYTRISTRIP);
    memcpy(newometry->morphTarget->verts, geometry->morphTarget->verts, sizeof(RwV3d) * numVertices);
    memcpy(newometry->texCoords[0], geometry->texCoords[0], sizeof(RwTexCoords) * numVertices);
    memcpy(newometry->triangles, geometry->triangles, sizeof(RpTriangle) * numTriangles);
    
    for(int i = 0; i < numTriangles; ++i)
    {
        RpMaterial* material = RpGeometryTriangleGetMaterial(geometry, &geometry->triangles[i]);
        RpGeometryTriangleSetMaterial(newometry, &newometry->triangles[i], material);
    }
    RpGeometryUnlock(newometry);
    newometry->flags |= rpGEOMETRYPOSITIONS; // why?
    RpAtomicSetGeometry(atomic, newometry, 1);
}
inline bool GeometrySetPrelitConstantColor(RpGeometry* geometry, CRGBA clr)
{
    if((geometry->flags & rpGEOMETRYPRELIT) == 0) return false;
    RpGeometryLock(geometry, 4095);
    
    RwRGBA* prelitClrPtr = geometry->preLitLum;
    if(prelitClrPtr)
    {
        RwInt32 numPrelit = geometry->numVertices;
        for(int i = 0; i < numPrelit; ++i)
        {
            prelitClrPtr[i] = *(RwRGBA*)&clr;
        }
    }
    
    RpGeometryUnlock(geometry);
    return true;
}
inline void RepaintGrassPrelit(RpGeometry* geometry, RwRGBA clr)
{
    RwRGBA* prelitClrPtr = geometry->preLitLum;

    // Not going to re prelit the whole model as it`s already done!
    if(*(int*)prelitClrPtr == *(int*)&clr) return;

    RpGeometryLock(geometry, 4095);
    RwInt32 numPrelit = geometry->numVertices;
    for(int i = 0; i < numPrelit; ++i) prelitClrPtr[i] = clr;
    RpGeometryUnlock(geometry);
}
inline RpMaterial* SetDefaultGrassMaterial(RpMaterial* material, void* rgba)
{
    material->color = *(RwRGBA*)rgba;
    RpMaterialSetTexture(material, tex_gras07Si);
    return material;
}
inline bool LoadGrassModels(const char** grassModelsNames, RpAtomic** ret)
{
    RpClump* clump = NULL;
    for(int i = 0; i < GRASS_MODELS_TAB; ++i)
    {
        RwStream* stream = RwStreamOpen(2, 1, grassModelsNames[i]);
        if(!stream) return false;
        if(!RwStreamFindChunk(stream, 16, 0, 0))
        {
            RwStreamClose(stream, 0);
            return false;
        }
        clump = RpClumpStreamRead(stream);
        RwStreamClose(stream, 0);
        RpAtomic* FirstAtomic = GetFirstAtomic(clump);
        SetFilterModeOnAtomicsTextures(FirstAtomic, rwFILTERMIPLINEAR);
        
        AtomicCreatePrelitIfNeeded(FirstAtomic);
        RpGeometry* geometry = FirstAtomic->geometry;
        RpGeometryLock(geometry, 4095);
        geometry->flags = (geometry->flags & 0xFFFFFF8F) | rpGEOMETRYMODULATEMATERIALCOLOR;
        RpGeometryUnlock(geometry);
        
        GeometrySetPrelitConstantColor(geometry, rgbaWhite);
        
        static RwRGBA defClr {0, 0, 0, 50};
        RpGeometryForAllMaterials(geometry, SetDefaultGrassMaterial, &defClr);

        RpAtomic* CloneAtomic = RpAtomicClone(FirstAtomic);
        RpClumpDestroy(clump);
        SetFilterModeOnAtomicsTextures(CloneAtomic, rwFILTERLINEAR);
        RpAtomicSetFrame(CloneAtomic, RwFrameCreate());
        ret[i] = CloneAtomic;
    }
    return true; // idk
}
CVector playerPos;
uintptr_t DrawTriPlants_BackTo;
extern "C" RpAtomic* DrawTriPlants_Inject(RpAtomic* standartAtomics, PPTriPlant* plant)
{
    switch(grassLODify)
    {
        default:
            return PC_PlantModelsTab0LOD[plant->model_id];

        case 1:
            // Better to do that from Player Pos!
            if((playerPos - plant->center).MagnitudeSqr() < fGrassMidDistanceSqr)
            {
                return standartAtomics;
            }
            else
            {
                return PC_PlantModelsTab0LOD[plant->model_id];
            }

        case 2:
            // Better to do that from Player Pos!
            if((playerPos - plant->center).MagnitudeSqr() < fGrassMid2DistanceSqr)
            {
                return standartAtomics;
            }
            else
            {
                return PC_PlantModelsTab0LOD[plant->model_id];
            }

        case 3:
            return standartAtomics;
    }
}
#ifdef AML32
__attribute__((optnone)) __attribute__((naked)) void DrawTriPlants_Patch(void)
{
    asm volatile(
        "LDR             R1, [SP]\n" // org
        "LDR.W           R0, [R1, R0, LSL#2]\n" // org
        "MOV             R1, R10\n"
        "BL              DrawTriPlants_Inject\n"
        "VLDR            S0, [R10, #0x48]\n" // org
        "VCVT.U32.F32    S0, S0\n" // org
        "PUSH            {R0}\n"
    );
    asm volatile(
        "MOV             R12, %0\n"
        "POP             {R0}\n"
        "BX              R12\n"
    :: "r" (DrawTriPlants_BackTo));
}
#else
__attribute__((optnone)) __attribute__((naked)) void DrawTriPlants_Patch(void)
{
    asm volatile(
        "MADD            X19, X21, X26, X20\n"
        "LDRH            W8, [X19, #0x30]\n"
        "LDR             X23, [X9, X8, LSL#3]\n"

        "MOV X0,         X23\n"
        "MOV X1,         X19\n"
        "BL              DrawTriPlants_Inject\n"
        "MOV X23,        X0\n"
    );
    asm volatile(
        "MOV             X16, %0\n"
        "LDR             S0, [X19, #0x50]\n"
        "BR              X16\n"
    :: "r" (DrawTriPlants_BackTo));
}
#endif
void InitPlantManager()
{
    PC_PlantTextureTab0[0] = GetTextureFromTextureDB("txgrass0_0"); //PC_PlantTextureTab0[0]->refCount++;
    PC_PlantTextureTab0[1] = GetTextureFromTextureDB("txgrass0_1"); //PC_PlantTextureTab0[1]->refCount++;
    PC_PlantTextureTab0[2] = GetTextureFromTextureDB("txgrass0_2"); //PC_PlantTextureTab0[2]->refCount++;
    PC_PlantTextureTab0[3] = GetTextureFromTextureDB("txgrass0_3"); //PC_PlantTextureTab0[3]->refCount++;

    PC_PlantTextureTab1[0] = GetTextureFromTextureDB("txgrass1_0"); //PC_PlantTextureTab1[0]->refCount++;
    PC_PlantTextureTab1[1] = GetTextureFromTextureDB("txgrass1_1"); //PC_PlantTextureTab1[1]->refCount++;
    PC_PlantTextureTab1[2] = GetTextureFromTextureDB("txgrass1_2"); //PC_PlantTextureTab1[2]->refCount++;
    PC_PlantTextureTab1[3] = GetTextureFromTextureDB("txgrass1_3"); //PC_PlantTextureTab1[3]->refCount++;

    for(int i = 0; i < GRASS_MODELS_TAB; ++i)
    {
        RwTextureSetAddressingMacro(PC_PlantTextureTab0[i], rwTEXTUREADDRESSWRAP);
        RwTextureSetFilterModeMacro(PC_PlantTextureTab0[i], rwFILTERLINEAR);

        RwTextureSetAddressingMacro(PC_PlantTextureTab1[i], rwTEXTUREADDRESSWRAP);
        RwTextureSetFilterModeMacro(PC_PlantTextureTab1[i], rwFILTERLINEAR);
    }
    
    tex_gras07Si = GetTextureFromTextureDB("gras07Si");

    PC_PlantSlotTextureTab[0] = PC_PlantTextureTab0;
    PC_PlantSlotTextureTab[1] = PC_PlantTextureTab1;
    PC_PlantSlotTextureTab[2] = PC_PlantTextureTab0;
    PC_PlantSlotTextureTab[3] = PC_PlantTextureTab1;

    g_bHasLODs = LoadGrassModels(grassMdls1LOD, PC_PlantModelsTab0LOD) && LoadGrassModels(grassMdls2LOD, PC_PlantModelsTab1LOD);
    g_bHasRegular = LoadGrassModels(grassMdls1, PC_PlantModelsTab0) && LoadGrassModels(grassMdls2, PC_PlantModelsTab1);
    if(g_bHasRegular)
    {
        PC_PlantModelSlotTab[0] = PC_PlantModelsTab0;
        PC_PlantModelSlotTab[1] = PC_PlantModelsTab1;
        PC_PlantModelSlotTab[2] = PC_PlantModelsTab0;
        PC_PlantModelSlotTab[3] = PC_PlantModelsTab1;

        SetPlantModelsTab(0, PC_PlantModelsTab0);
        SetPlantModelsTab(1, PC_PlantModelsTab0);
        SetPlantModelsTab(2, PC_PlantModelsTab0);
        SetPlantModelsTab(3, PC_PlantModelsTab0);
    }
    else if(g_bHasLODs)
    {
        PC_PlantModelSlotTab[0] = PC_PlantModelsTab0LOD;
        PC_PlantModelSlotTab[1] = PC_PlantModelsTab1LOD;
        PC_PlantModelSlotTab[2] = PC_PlantModelsTab0LOD;
        PC_PlantModelSlotTab[3] = PC_PlantModelsTab1LOD;

        SetPlantModelsTab(0, PC_PlantModelsTab0LOD);
        SetPlantModelsTab(1, PC_PlantModelsTab0LOD);
        SetPlantModelsTab(2, PC_PlantModelsTab0LOD);
        SetPlantModelsTab(3, PC_PlantModelsTab0LOD);
    }
    else
    {
        return;
    }
    SetCloseFarAlphaDist(3.0f, /* 60.0f */ fGrassDistance);
    RecalcGrassVars();

    if(g_bHasRegular && g_bHasLODs)
    {
        static bool bDidPatch = false;
        if(!bDidPatch)
        {
            bDidPatch = true;

            logger->Info("Found grass LODs as an addon, additional patch is on the way!");

            // Enable LODs!
            #ifdef AML32
                DrawTriPlants_BackTo = pGTASA + 0x2CD37E + 0x1;
                aml->Redirect(pGTASA + 0x2CD370 + 0x1, (uintptr_t)DrawTriPlants_Patch);
            #else
                DrawTriPlants_BackTo = pGTASA + 0x38F024;
                aml->Redirect(pGTASA + 0x38F014, (uintptr_t)DrawTriPlants_Patch);
            #endif
        }
    }
}
inline float lerp(float a, float b, float t)
{
    return a + t * (b - a);
}
inline float GetCurrentLighting(tColLighting col, float fScale = 0.5f)
{
    const float fDay = col.day * fScale / 15.0f;
    const float fNight = col.night * fScale / 15.0f;
    return lerp(fDay, fNight, *m_fDNBalanceParam);
}
RpMaterial* SetGrassModelProperties(RpMaterial* material, void* data)
{
    PPTriPlant* plant = (PPTriPlant*)data;
    material->texture = plant->texture_ptr;
    material->color   = plant->color;
    // RpMaterialSetTexture(material, plant->texture_ptr); // Deletes previous texture, SUS BEHAVIOUR
    // UPD: This is normal. But we need to keep our texture FOREVER. So not deleting it. EVER.
    // UPD2: But why does it happen?! I think it's the same on PC?
    return material;
}
inline float GetGrassAlphaScale(float distance)
{
    if(distance < fGrassMinDistance) return 1.0f;
    
    float scale = 1.0f - fGrassFadeDist * (distance - fGrassMinDistance);
    if(scale < 0) return 0;
    return scale;
}
inline float GetGrassAlphaScaleFromSQR(float distanceSqr)
{
    if(distanceSqr < fGrassMinDistanceSqr) return 1.0f;
    
    float scale = 1.0f - fGrassFadeDistSqr * (distanceSqr - fGrassMinDistanceSqr);
    if(scale < 0) return 0;
    return scale;
}


// ---------------------------------------------------------------------------------------


uintptr_t GrassMaterialApplying_BackTo;
extern "C" void GrassMaterialApplying(RpGeometry* geometry, PPTriPlant* plant)
{
    // ULTRA EXPERIMENTAL AND DUMB WAY!
    RwRGBA newGrassCol = plant->color;
    newGrassCol.alpha = 255;
    RepaintGrassPrelit(geometry, newGrassCol);
    // ULTRA EXPERIMENTAL AND DUMB WAY!

    RpGeometryForAllMaterials(geometry, SetGrassModelProperties, plant);
}
#ifdef AML32
__attribute__((optnone)) __attribute__((naked)) void GrassMaterialApplying_Patch(void)
{
    asm volatile(
        "PUSH {R0-R9}\n"
        "LDR R0, [SP, #0]\n"
        "MOV R1, R10\n"
        "BL GrassMaterialApplying\n");
    asm volatile(
        "MOV R12, %0\n"
        "POP {R0-R9}\n"
        "BX R12\n"
    :: "r" (GrassMaterialApplying_BackTo));
}
#else
__attribute__((optnone)) __attribute__((naked)) void GrassMaterialApplying_Patch(void)
{
    asm volatile(
        "LDR X0, [X23, #0x30]\n"
        "MOV X1, X19\n"
        "BL GrassMaterialApplying\n");
    asm volatile(
        "MOV X16, %0\n"
        "MOV W25, #0x68\n"
        "BR X16\n"
    :: "r" (GrassMaterialApplying_BackTo));
}
#endif
DECL_HOOKv(PlantSurfPropMgrInit)
{
    PlantSurfPropMgrInit();
    PlantSurfPropMgrLoadPlantsDat("PLANTS.DAT");
}
DECL_HOOKv(PlantMgrInit)
{
    PlantMgrInit(); // Acts like a CPlantMgr::ReloadConfig()
    InitPlantManager();

    memset(&m_LocTrisTab_NEW, 0, sizeof(m_LocTrisTab_NEW));
    *m_UnusedLocTriListHead = &m_LocTrisTab_NEW[0];
    m_LocTrisTab_NEW[0].m_pPrevTri = NULL;
    for(int i = 0; i < LOCTRIS_LIMIT-1; ++i)
    {
        m_LocTrisTab_NEW[i].m_pNextTri = &m_LocTrisTab_NEW[i + 1];
    }
    m_LocTrisTab_NEW[LOCTRIS_LIMIT-1].m_pNextTri = NULL;
}
static PPTriPlant plant;
DECL_HOOKv(PlantMgrRender)
{
    playerPos = (TheCamera->m_bCamDirectlyBehind) ? FindPlayerPed(-1)->GetPosition() : TheCamera->GetPosition();
    
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)false);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)false);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)false);
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONALWAYS);
    // new
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLFRONT);

    CVector camPos = TheCamera->GetPosition();
    for(int type = 0; type < MAXLOCTRIS; ++type)
    {
        CPlantLocTri* plantTris = m_CloseLocTriListHead[type];
        RwTexture** grassTex = PC_PlantSlotTextureTab[type];

        while(plantTris != NULL)
        {
            if(plantTris->m_createsPlants)
            {
                float colBrightness = GetCurrentLighting(plantTris->m_ColLighting, 0.7f);
                CPlantSurfProp* surface = GetSurfacePtr(plantTris->m_nSurfaceType);
                if(surface && IsSphereVisibleForCamera(TheCamera, &plantTris->m_Center, plantTris->m_SphereRadius))
                {
                    for(int i = 0; i < MAX_SURFACE_DEFINDEXES; ++i)
                    {
                        CPlantSurfPropPlantData& surfProp = surface->m_aSurfaceProperties[i];
                        if(surfProp.m_nModelId != 0xFFFF)
                        {
                            plant.color = *(RwRGBA*)&surfProp.m_Color;
                            plant.color.alpha = (uint8_t)(GetGrassAlphaScaleFromSQR((camPos - plantTris->m_Center).MagnitudeSqr()) * plant.color.alpha);

                            if(plant.color.alpha != 0)
                            {
                                plant.V1 = plantTris->m_V1;
                                plant.V2 = plantTris->m_V2;
                                plant.V3 = plantTris->m_V3;
                                plant.center = plantTris->m_Center;
                                plant.model_id = surfProp.m_nModelId;
                                plant.num_plants = ((plantTris->m_nMaxNumPlants[i] + 8) & 0xFFF8);
                                plant.scale.x = surfProp.m_fSizeScaleXY;
                                plant.scale.y = surfProp.m_fSizeScaleZ;
                                plant.texture_ptr = grassTex[surfProp.m_nUVOffset];
                                plant.intensity = surfProp.m_Intensity;
                                plant.intensity_var = surfProp.m_IntensityVariation;
                                plant.seed = plantTris->m_Seed[i];
                                plant.scale_var_xy = surfProp.m_fSizeScaleXYVariation;
                                plant.scale_var_z = surfProp.m_fSizeScaleZVariation;
                                plant.wind_bend_scale = surfProp.m_fWindBendingScale;
                                plant.wind_bend_var = surfProp.m_fWindBendingVariation;
                                
                                plant.color.red   *= colBrightness;
                                plant.color.green *= colBrightness;
                                plant.color.blue  *= colBrightness;

                                AddTriPlant(&plant, type);
                            }
                        }
                    }
                }
            }
            plantTris = plantTris->m_pNextTri;
        }
        FlushTriPlantBuffer();
    }

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)false);
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONGREATEREQUAL);
}


// ---------------------------------------------------------------------------------------


extern "C" void OnModLoad()
{
    logger->SetTag("SA Planter");
    
    pGTASA = aml->GetLib("libGTASA.so");
    hGTASA = aml->GetLibHandle("libGTASA.so");
    
    if(!pGTASA || !hGTASA)
    {
        logger->Info("Only GTA:SA Android (v2.00 and v2.10 64-bit) supported by Planter!");
        return;
    }

    #ifdef AML32
        GrassMaterialApplying_BackTo =          pGTASA + 0x2CD434 + 0x1;
        aml->Redirect(pGTASA + 0x2CD42A + 0x1,  (uintptr_t)GrassMaterialApplying_Patch);
        HOOKPLT(PlantSurfPropMgrInit,           pGTASA + 0x6721C8);
        HOOKPLT(PlantMgrInit,                   pGTASA + 0x673C90);
        HOOKPLT(PlantMgrRender,                 pGTASA + 0x6726D0);
        //HOOKBLX(PS2_GrassRandomizer,            pGTASA + 0x2CD388 + 0x1); // only on PC, uint64 -> uint32
    #else
        GrassMaterialApplying_BackTo =          pGTASA + 0x38F0D4;
        aml->Redirect(pGTASA + 0x38F0C0,        (uintptr_t)GrassMaterialApplying_Patch);
        HOOKBL(PlantSurfPropMgrInit,            pGTASA + 0x390DE0);
        HOOKPLT(PlantMgrInit,                   pGTASA + 0x846680);
        HOOKPLT(PlantMgrRender,                 pGTASA + 0x844308);
        //HOOKBL(PS2_GrassRandomizer,             pGTASA + 0x38F02C); // only on PC, uint64 -> uint32
        
        aml->WriteFloat(pGTASA + 0x38F0D0,      400.0f * 400.0f); // for patches in fn PatchGrassDistance
    #endif

    SET_TO(TheCamera,                       aml->GetSym(hGTASA, "TheCamera"));
    SET_TO(m_SurfPropPtrTab,                aml->GetSym(hGTASA, "_ZN17CPlantSurfPropMgr16m_SurfPropPtrTabE"));
    SET_TO(m_countSurfPropsAllocated,       aml->GetSym(hGTASA, "_ZN17CPlantSurfPropMgr25m_countSurfPropsAllocatedE"));
    SET_TO(m_SurfPropTab,                   aml->GetSym(hGTASA, "_ZN17CPlantSurfPropMgr13m_SurfPropTabE"));
    SET_TO(PC_PlantSlotTextureTab,          aml->GetSym(hGTASA, "_ZN9CPlantMgr22PC_PlantSlotTextureTabE"));
    SET_TO(PC_PlantTextureTab0,             aml->GetSym(hGTASA, "_ZN9CPlantMgr19PC_PlantTextureTab0E"));
    SET_TO(PC_PlantTextureTab1,             aml->GetSym(hGTASA, "_ZN9CPlantMgr19PC_PlantTextureTab1E"));
    SET_TO(PC_PlantModelSlotTab,            aml->GetSym(hGTASA, "_ZN9CPlantMgr20PC_PlantModelSlotTabE"));
    SET_TO(PC_PlantModelsTab0,              aml->GetSym(hGTASA, "_ZN9CPlantMgr18PC_PlantModelsTab0E"));
    SET_TO(PC_PlantModelsTab1,              aml->GetSym(hGTASA, "_ZN9CPlantMgr18PC_PlantModelsTab1E"));
    SET_TO(m_CloseLocTriListHead,           aml->GetSym(hGTASA, "_ZN9CPlantMgr21m_CloseLocTriListHeadE"));
    SET_TO(m_UnusedLocTriListHead,          aml->GetSym(hGTASA, "_ZN9CPlantMgr22m_UnusedLocTriListHeadE"));
    SET_TO(m_fDNBalanceParam,               aml->GetSym(hGTASA, "_ZN25CCustomBuildingDNPipeline17m_fDNBalanceParamE"));

    SET_TO(RwRenderStateSet,                aml->GetSym(hGTASA, "_Z16RwRenderStateSet13RwRenderStatePv"));
    SET_TO(FileMgrSetDir,                   aml->GetSym(hGTASA, "_ZN8CFileMgr6SetDirEPKc"));
    SET_TO(FileMgrOpenFile,                 aml->GetSym(hGTASA, "_ZN8CFileMgr8OpenFileEPKcS1_"));
    SET_TO(FileMgrCloseFile,                aml->GetSym(hGTASA, BYVER("_ZN8CFileMgr9CloseFileEj", "_ZN8CFileMgr9CloseFileEy")));
    SET_TO(FileLoaderLoadLine,              aml->GetSym(hGTASA, BYVER("_ZN11CFileLoader8LoadLineEj", "_ZN11CFileLoader8LoadLineEy")));
    SET_TO(GetSurfaceIdFromName,            aml->GetSym(hGTASA, "_ZN14SurfaceInfos_c20GetSurfaceIdFromNameEPc"));
    SET_TO(GetTextureFromTextureDB,         aml->GetSym(hGTASA, "_ZN22TextureDatabaseRuntime10GetTextureEPKc"));
    SET_TO(SetPlantModelsTab,               aml->GetSym(hGTASA, "_ZN14CGrassRenderer17SetPlantModelsTabEjPP8RpAtomic"));
    SET_TO(SetCloseFarAlphaDist,            aml->GetSym(hGTASA, "_ZN14CGrassRenderer20SetCloseFarAlphaDistEff"));
    SET_TO(FlushTriPlantBuffer,             aml->GetSym(hGTASA, "_ZN14CGrassRenderer19FlushTriPlantBufferEv"));
    SET_TO(RpGeometryLock,                  aml->GetSym(hGTASA, "_Z14RpGeometryLockP10RpGeometryi"));
    SET_TO(RpGeometryUnlock,                aml->GetSym(hGTASA, "_Z16RpGeometryUnlockP10RpGeometry"));
    SET_TO(RpGeometryCreate,                aml->GetSym(hGTASA, "_Z16RpGeometryCreateiij"));
    SET_TO(RpGeometryTriangleGetMaterial,   aml->GetSym(hGTASA, "_Z29RpGeometryTriangleGetMaterialPK10RpGeometryPK10RpTriangle"));
    SET_TO(RpGeometryTriangleSetMaterial,   aml->GetSym(hGTASA, "_Z29RpGeometryTriangleSetMaterialP10RpGeometryP10RpTriangleP10RpMaterial"));
    SET_TO(RpAtomicSetGeometry,             aml->GetSym(hGTASA, "_Z19RpAtomicSetGeometryP8RpAtomicP10RpGeometryj"));
    SET_TO(RwStreamOpen,                    aml->GetSym(hGTASA, "_Z12RwStreamOpen12RwStreamType18RwStreamAccessTypePKv"));
    SET_TO(RwStreamFindChunk,               aml->GetSym(hGTASA, "_Z17RwStreamFindChunkP8RwStreamjPjS1_"));
    SET_TO(RpClumpStreamRead,               aml->GetSym(hGTASA, "_Z17RpClumpStreamReadP8RwStream"));
    SET_TO(RwStreamClose,                   aml->GetSym(hGTASA, "_Z13RwStreamCloseP8RwStreamPv"));
    SET_TO(GetFirstAtomic,                  aml->GetSym(hGTASA, "_Z14GetFirstAtomicP7RpClump"));
    SET_TO(SetFilterModeOnAtomicsTextures,  aml->GetSym(hGTASA, "_Z30SetFilterModeOnAtomicsTexturesP8RpAtomic19RwTextureFilterMode"));
    SET_TO(RpMaterialSetTexture,            aml->GetSym(hGTASA, "_Z20RpMaterialSetTextureP10RpMaterialP9RwTexture"));
    SET_TO(RpAtomicClone,                   aml->GetSym(hGTASA, "_Z13RpAtomicCloneP8RpAtomic"));
    SET_TO(RpClumpDestroy,                  aml->GetSym(hGTASA, "_Z14RpClumpDestroyP7RpClump"));
    SET_TO(RwFrameCreate,                   aml->GetSym(hGTASA, "_Z13RwFrameCreatev"));
    SET_TO(RpGeometryForAllMaterials,       aml->GetSym(hGTASA, "_Z25RpGeometryForAllMaterialsP10RpGeometryPFP10RpMaterialS2_PvES3_"));
    SET_TO(RpAtomicSetFrame,                aml->GetSym(hGTASA, "_Z16RpAtomicSetFrameP8RpAtomicP7RwFrame"));
    SET_TO(IsSphereVisibleForCamera,        aml->GetSym(hGTASA, "_ZN7CCamera15IsSphereVisibleERK7CVectorf"));
    SET_TO(AddTriPlant,                     aml->GetSym(hGTASA, "_ZN14CGrassRenderer11AddTriPlantEP10PPTriPlantj"));
    SET_TO(FindPlayerPed,                   aml->GetSym(hGTASA, "_Z13FindPlayerPedi"));

    sautils = (ISAUtils*)GetInterface("SAUtils");
    if(sautils)
    {
        #ifdef AML32
            aml->Write(pGTASA + 0x2CD342, "\xB1\xEE\xCF\xFA\x01\x99\xD1\xED\x00\x8A", 10);
            aml->PlaceB(pGTASA + 0x2CD34C + 0x1, pGTASA + 0x2CD35C + 0x1);
        #else
            aml->Write32(pGTASA + 0x38EFE0, 0x1E2E1001);
        #endif

        int grassDist = DEFAULT_GRASS_DISTANCE;
        aml->MLSGetInt("GRSQUAL", &grassDist); clampint(0, 5, &grassDist);
        OnGrassDistanceChanged(1, grassDist, NULL);
        sautils->AddClickableItem(eTypeOfSettings::SetType_Display, "Grass Distance", grassDist, 0, 5, aGrassDistanceSwitch, OnGrassDistanceChanged, NULL);

        grassLODify = DEFAULT_GRASS_LOD;
        aml->MLSGetInt("GRSLOD", &grassLODify); clampint(0, 3, &grassLODify);
        sautils->AddClickableItem(eTypeOfSettings::SetType_Display, "Grass Quality", grassLODify, 0, 3, aGrassLODSwitch, OnGrassLODChanged, NULL);
    }
}