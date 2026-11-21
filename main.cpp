#include <mod/amlmod.h>
#include <mod/logger.h>

#ifdef AML32
    #include "GTASA_STRUCTS.h"
#else
    #include "GTASA_STRUCTS_210.h"
#endif
#include "isautils.h"

MYMOD(net.rusjj.plantes, GTASA Planter, 1.7, RusJJ)
NEEDGAME(com.rockstargames.gtasa)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.aml, 1.4.0)
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
const char* aNoModels = "No Models Installed";
const char* aYesNo[2] =
{
    "FEM_OFF",
    "FEM_ON",
};
RwTexture *tex_gras07Si;
float* pOpaqueBlack;
float fOpaqueBlackOrig[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

float fGrassMinDistance, fGrassMidDistance, fGrassMid2Distance, fGrassDistance;
float fGrassMidDistanceSqr, fGrassMid2DistanceSqr;
RpAtomic* PC_PlantModelsTab0LOD[4];
RpAtomic* PC_PlantModelsTab1LOD[4];
bool g_bHasLODs = false, g_bHasRegular = false;
int grassLODify = DEFAULT_GRASS_LOD;
int grassBackculling = true;

bool g_bPS2Mode = false;
bool g_bUseSecondModelSet = false;
bool g_bAlphaScale = true;


// ---------------------------------------------------------------------------------------


CCamera*            TheCamera;
CPlantSurfProp**    m_SurfPropPtrTab;
uint32_t*           m_countSurfPropsAllocated;
CPlantSurfProp*     m_SurfPropTab;
CPlantSurfProp      m_SurfPropTab_Fallback[MAX_SURFACE_PROPS]; // backup
RwTexture**         PC_PlantSlotTextureTab[4];
RwTexture*          PC_PlantTextureTab0[4];
RwTexture*          PC_PlantTextureTab1[4];
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
    fGrassMidDistance =  0.75f * fGrassMinDistance;
    fGrassMid2Distance = 0.75f * fGrassMinDistance + 0.5f * (fGrassDistance - fGrassMinDistance);

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
void OnGrassBackCullingChanged(int oldVal, int newVal, void* data)
{
    clampint(0, 1, &newVal);
    grassBackculling = newVal;
    aml->MLSSetInt("GRSBKCL", newVal);
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
    if( *m_countSurfPropsAllocated < MAX_SURFACE_PROPS )
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
    if(!f)
    {
        logger->Error("Cannot open %s!", filename);
        return;
    }

    char* i, *token; int line = 0;
    uint16_t SurfaceIdFromName = 0; uint8_t nPlantCoverDefIndex = 0;
    CPlantSurfProp* SurfacePtr;
    CPlantSurfPropPlantData* ActiveSurface;

    while((i = FileLoaderLoadLine(f)) != NULL)
    {
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
                        logger->Error("SurfacePtr (%s) (surfId %d, .dat line %d) is NULL!", token, (int)SurfaceIdFromName, line);
                    }
                    else
                    {
                        logger->Error("Unknown surface name '%s' in 'plants.dat' (line %d)! See Andrzej to fix this.", token, line);
                        goto NextLine; // Skip only this line (PC stops reading here)
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
      NextLine:;
    }

  CloseFile:
    FileMgrCloseFile(f);
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
        if(!clump) return false;
        RpAtomic* FirstAtomic = GetFirstAtomic(clump);
        SetFilterModeOnAtomicsTextures(FirstAtomic, rwFILTERMIPLINEAR);
        
        // No prelit! Colour comes from _rwOpenGLOpaqueBlack (see GrassMaterialApplying)
        RpGeometry* geometry = FirstAtomic->geometry;
        RpGeometryLock(geometry, 4095);
        geometry->flags = (geometry->flags & 0xFFFFFF87) | rpGEOMETRYMODULATEMATERIALCOLOR;
        RpGeometryUnlock(geometry);

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
extern "C" __attribute__((visibility("default")))
RpAtomic* DrawTriPlants_Inject(RpAtomic* standartAtomics, PPTriPlant* plant)
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
__attribute__((optnone)) __attribute__((naked)) void DrawTriPlants_Patch(void)
{
#ifdef AML32
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
#else
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
#endif
}
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

    g_bHasRegular = LoadGrassModels(grassMdls1,    PC_PlantModelsTab0)    && LoadGrassModels(grassMdls2,    PC_PlantModelsTab1);
    g_bHasLODs    = LoadGrassModels(grassMdls1LOD, PC_PlantModelsTab0LOD) && LoadGrassModels(grassMdls2LOD, PC_PlantModelsTab1LOD);
    
    if(g_bHasRegular)
    {
        SetPlantModelsTab(0, PC_PlantModelsTab0);
        SetPlantModelsTab(1, g_bUseSecondModelSet ? PC_PlantModelsTab1 : PC_PlantModelsTab0);
        SetPlantModelsTab(2, PC_PlantModelsTab0);
        SetPlantModelsTab(3, g_bUseSecondModelSet ? PC_PlantModelsTab1 : PC_PlantModelsTab0);
    }
    else if(g_bHasLODs)
    {
        // fallback if for some reason user didnt install regular models but lods ONLY :/
        SetPlantModelsTab(0, PC_PlantModelsTab0LOD);
        SetPlantModelsTab(1, g_bUseSecondModelSet ? PC_PlantModelsTab1LOD : PC_PlantModelsTab0LOD);
        SetPlantModelsTab(2, PC_PlantModelsTab0LOD);
        SetPlantModelsTab(3, g_bUseSecondModelSet ? PC_PlantModelsTab1LOD : PC_PlantModelsTab0LOD);
    }
    else
    {
        logger->Error("Grass models are missing (models\\grass\\*.dff)! Grass is disabled.");
        for(int i = 0; i < 6; ++i) aGrassDistanceSwitch[i] = aNoModels;
        for(int i = 0; i < 4; ++i) aGrassLODSwitch[i] = aNoModels;
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
inline uint8_t GetCurrentLighting(tColLighting col, float fScale = 0.5f)
{
    // Same as PC's CPlantMgr::Render
    const float fDay   = col.day   * (fScale / 15.0f);
    const float fNight = col.night * (fScale / 15.0f);
    return (uint8_t)(lerp(fDay, fNight, *m_fDNBalanceParam) * 255.0f);
}

static RwRGBA grassBatchColour;
RpMaterial* SetGrassModelProperties(RpMaterial* material, void* data)
{
    PPTriPlant* plant = (PPTriPlant*)data;
    material->texture = plant->texture_ptr;
    material->color   = grassBatchColour;
    return material;
}


// ---------------------------------------------------------------------------------------


uintptr_t GrassMaterialApplying_BackTo;
extern "C" __attribute__((visibility("default")))
void GrassMaterialApplying(RpGeometry* geometry, PPTriPlant* plant, RwRGBA* pBatchColour)
{
    grassBatchColour = *pBatchColour; // color * intensity, faded alpha

    // Unlit + no prelit = glColor4fv(_rwOpenGLOpaqueBlack), so that's our colour
    pOpaqueBlack[0] = grassBatchColour.red   * (1.0f / 255.0f);
    pOpaqueBlack[1] = grassBatchColour.green * (1.0f / 255.0f);
    pOpaqueBlack[2] = grassBatchColour.blue  * (1.0f / 255.0f);
    pOpaqueBlack[3] = grassBatchColour.alpha * ((g_bAlphaScale ? 0.5f : 1.0f) / 255.0f);

    grassBatchColour.alpha = 254; // 255 disables blending

    RpGeometryForAllMaterials(geometry, SetGrassModelProperties, plant);
}
__attribute__((optnone)) __attribute__((naked)) void GrassMaterialApplying_Patch(void)
{
#ifdef AML32
    asm volatile(
        "PUSH {R0-R9}\n"
        "LDR R0, [SP, #0]\n"
        "MOV R1, R10\n"
        "ADD R2, SP, #0x74\n" // pData (0x4C + 0x28 of PUSH)
        "BL GrassMaterialApplying\n");
    asm volatile(
        "MOV R12, %0\n"
        "POP {R0-R9}\n"
        "BX R12\n"
    :: "r" (GrassMaterialApplying_BackTo));
#else
    asm volatile(
        "LDR X0, [X23, #0x30]\n"
        "MOV X1, X19\n"
        "ADD X2, SP, #0x60\n" // pData
        "BL GrassMaterialApplying\n");
    asm volatile(
        "MOV X16, %0\n"
        "MOV W25, #0x68\n"
        "BR X16\n"
    :: "r" (GrassMaterialApplying_BackTo));
#endif
}
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
DECL_HOOKv(PlantMgrRender)
{
    if(!g_bHasRegular && !g_bHasLODs) return; // No models = NULL plantModelsTab in DrawTriPlants

    playerPos = (TheCamera->m_bCamDirectlyBehind) ? FindPlayerPed(-1)->GetPosition() : TheCamera->GetPosition();

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)(g_bPS2Mode ? true : false));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)(grassBackculling ? rwCULLMODECULLFRONT : rwCULLMODECULLNONE)); // Planter only

    if(g_bPS2Mode)
    {
        RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
    }

    for(int type = 0; type < MAXLOCTRIS; ++type)
    {
        CPlantLocTri* plantTris = m_CloseLocTriListHead[type];
        RwTexture** grassTex = PC_PlantSlotTextureTab[type];

        while(plantTris != NULL)
        {
            if(plantTris->m_createsPlants)
            {
                uint8_t colBrightness = GetCurrentLighting(plantTris->m_ColLighting);
                CPlantSurfProp* surface = GetSurfacePtr(plantTris->m_nSurfaceType);
                if(surface && IsSphereVisibleForCamera(TheCamera, &plantTris->m_Center, plantTris->m_SphereRadius))
                {
                    for(int i = 0; i < MAX_SURFACE_DEFINDEXES; ++i)
                    {
                        CPlantSurfPropPlantData& surfProp = surface->m_aSurfaceProperties[i];
                        if(surfProp.m_Color.a && surfProp.m_nModelId != (uint16_t)-1)
                        {
                            PPTriPlant plant;

                            plant.color = *(RwRGBA*)&surfProp.m_Color;
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

                            plant.color.red   = (colBrightness * plant.color.red)   >> 8;
                            plant.color.green = (colBrightness * plant.color.green) >> 8;
                            plant.color.blue  = (colBrightness * plant.color.blue)  >> 8;

                            AddTriPlant(&plant, type);
                        }
                    }
                }
            }
            plantTris = plantTris->m_pNextTri;
        }
        FlushTriPlantBuffer();
    }
    
    memcpy(pOpaqueBlack, fOpaqueBlackOrig, sizeof(fOpaqueBlackOrig));

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)true);
}


// ---------------------------------------------------------------------------------------


ON_MOD_LOAD()
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

        aml->Write32(pGTASA + 0x456C64,         0x0AC0EEBD); // Procobj seed fix
    #else
        GrassMaterialApplying_BackTo =          pGTASA + 0x38F0D4;
        aml->Redirect(pGTASA + 0x38F0C0,        (uintptr_t)GrassMaterialApplying_Patch);
        HOOK(PlantSurfPropMgrInit,              pGTASA + 0x390F1C);
        HOOKPLT(PlantMgrInit,                   pGTASA + 0x846680);
        HOOKPLT(PlantMgrRender,                 pGTASA + 0x844308);

        aml->WriteFloat(pGTASA + 0x38F0D0,      400.0f * 400.0f); // for patches in fn PatchGrassDistance

        aml->Write32(pGTASA + 0x540234,         0x1E380015); // Procobj seed fix
    #endif

    SET_TO(TheCamera,                       aml->GetSym(hGTASA, "TheCamera"));
    SET_TO(m_SurfPropPtrTab,                aml->GetSym(hGTASA, "_ZN17CPlantSurfPropMgr16m_SurfPropPtrTabE"));
    SET_TO(m_countSurfPropsAllocated,       aml->GetSym(hGTASA, "_ZN17CPlantSurfPropMgr25m_countSurfPropsAllocatedE"));
    SET_TO(m_SurfPropTab,                   aml->GetSym(hGTASA, "_ZN17CPlantSurfPropMgr13m_SurfPropTabE"));
    SET_TO(PC_PlantSlotTextureTab,          aml->GetSym(hGTASA, "_ZN9CPlantMgr22PC_PlantSlotTextureTabE"));
    SET_TO(PC_PlantTextureTab0,             aml->GetSym(hGTASA, "_ZN9CPlantMgr19PC_PlantTextureTab0E"));
    SET_TO(PC_PlantTextureTab1,             aml->GetSym(hGTASA, "_ZN9CPlantMgr19PC_PlantTextureTab1E"));
    SET_TO(PC_PlantModelsTab0,              aml->GetSym(hGTASA, "_ZN9CPlantMgr18PC_PlantModelsTab0E"));
    SET_TO(PC_PlantModelsTab1,              aml->GetSym(hGTASA, "_ZN9CPlantMgr18PC_PlantModelsTab1E"));
    SET_TO(m_CloseLocTriListHead,           aml->GetSym(hGTASA, "_ZN9CPlantMgr21m_CloseLocTriListHeadE"));
    SET_TO(m_UnusedLocTriListHead,          aml->GetSym(hGTASA, "_ZN9CPlantMgr22m_UnusedLocTriListHeadE"));
    SET_TO(m_fDNBalanceParam,               aml->GetSym(hGTASA, "_ZN25CCustomBuildingDNPipeline17m_fDNBalanceParamE"));
    SET_TO(pOpaqueBlack,                    aml->GetSym(hGTASA, "_rwOpenGLOpaqueBlack"));

    SET_TO(RwRenderStateSet,                aml->GetSym(hGTASA, "_Z16RwRenderStateSet13RwRenderStatePv"));
    SET_TO(FileMgrSetDir,                   aml->GetSym(hGTASA, "_ZN8CFileMgr6SetDirEPKc"));
    SET_TO(FileMgrOpenFile,                 aml->GetSym(hGTASA, "_ZN8CFileMgr8OpenFileEPKcS1_"));
    SET_TO(FileMgrCloseFile,                aml->GetSym(hGTASA, BYBIT("_ZN8CFileMgr9CloseFileEj", "_ZN8CFileMgr9CloseFileEy")));
    SET_TO(FileLoaderLoadLine,              aml->GetSym(hGTASA, BYBIT("_ZN11CFileLoader8LoadLineEj", "_ZN11CFileLoader8LoadLineEy")));
    SET_TO(GetSurfaceIdFromName,            aml->GetSym(hGTASA, "_ZN14SurfaceInfos_c20GetSurfaceIdFromNameEPc"));
    SET_TO(GetTextureFromTextureDB,         aml->GetSym(hGTASA, "_ZN22TextureDatabaseRuntime10GetTextureEPKc"));
    SET_TO(SetPlantModelsTab,               aml->GetSym(hGTASA, "_ZN14CGrassRenderer17SetPlantModelsTabEjPP8RpAtomic"));
    SET_TO(SetCloseFarAlphaDist,            aml->GetSym(hGTASA, "_ZN14CGrassRenderer20SetCloseFarAlphaDistEff"));
    SET_TO(FlushTriPlantBuffer,             aml->GetSym(hGTASA, "_ZN14CGrassRenderer19FlushTriPlantBufferEv"));
    SET_TO(RpGeometryLock,                  aml->GetSym(hGTASA, "_Z14RpGeometryLockP10RpGeometryi"));
    SET_TO(RpGeometryUnlock,                aml->GetSym(hGTASA, "_Z16RpGeometryUnlockP10RpGeometry"));
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

    if(!pOpaqueBlack) SET_TO(pOpaqueBlack, pGTASA + BYBIT(0x6BD6E0, 0x89B2D0));
    if(!m_SurfPropTab) m_SurfPropTab = m_SurfPropTab_Fallback;

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

        grassBackculling = true;
        aml->MLSGetInt("GRSBKCL", &grassBackculling); clampint(0, 1, &grassBackculling);
        sautils->AddClickableItem(eTypeOfSettings::SetType_Display, "Grass Backface Culling", grassBackculling, 0, 1, aYesNo, OnGrassBackCullingChanged, NULL);
    }
}