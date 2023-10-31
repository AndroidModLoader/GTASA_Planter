#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>

#ifdef AML32
    #include "GTASA_STRUCTS.h"
    #define BYVER(__for32, __for64) (__for32)
#else
    #include "GTASA_STRUCTS_210.h"
    #define BYVER(__for32, __for64) (__for64)
#endif
#include "isautils.h"

MYMODCFG(net.rusjj.plantes, GTASA Planter, 1.0, RusJJ)
NEEDGAME(com.rockstargames.gtasa)

uintptr_t pGTASA;
void* hGTASA;
ISAUtils* sautils;

#define MAXLOCTRIS 4
#define GRASS_MODELS_TAB 4


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
const char* aGrassQualitySwitch[4] = 
{
    "FED_FXL",
    "FED_FXM",
    "FED_FXH",
    "FED_FXV",
};
RwTexture* tex_gras07Si;
ConfigEntry* cfgGrassQuality;


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
        m_SurfPropPtrTab[index] = &m_SurfPropTab[*m_countSurfPropsAllocated];
        ++(*m_countSurfPropsAllocated);
        return m_SurfPropPtrTab[index];
    }
    return NULL;
}
inline void PlantSurfPropMgrLoadPlantsDat(const char* filename)
{
    logger->Info("Parsing plants.dat...");
    FileMgrSetDir("DATA");
    FILE* f = FileMgrOpenFile(filename, "r");
    FileMgrSetDir("");

    char* i, *token; int line = 0;
    unsigned short SurfaceIdFromName = 0, nPlantCoverDefIndex = 0;
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
                        logger->Error("SurfacePtr (%s) (%d, line %d) is NULL!", token, SurfaceIdFromName, line);
                    }
                    else
                    {
                        logger->Error("Unknown surface name '%s' in 'plants.dat' (line %d)! See Andrzej to fix this.", token, line);
                        continue; // Just let it pass all surfaces (why do we need to stop it actually?)
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
inline void GeometrySetPrelitConstantColorForce(RpGeometry* geometry, CRGBA clr)
{
    RwRGBA* prelitClrPtr = geometry->preLitLum;
    if(prelitClrPtr)
    {
        RwInt32 numPrelit = geometry->numVertices;
        for(int i = 0; i < numPrelit; ++i)
        {
            prelitClrPtr[i] = *(RwRGBA*)&clr;
        }
    }
}
inline float GetPlantDensity(CPlantLocTri* plant)
{
    // The magnitude of the cross product of 2 vectors give the area of a paralellogram they span.
    return (plant->m_V2 - plant->m_V1).Cross(plant->m_V3 - plant->m_V1).Magnitude() / 2.f;
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
        if(stream && RwStreamFindChunk(stream, 16, 0, 0)) clump = RpClumpStreamRead(stream);
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

    for(int i = 0; i < 4; ++i)
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

    if(LoadGrassModels(grassMdls1, PC_PlantModelsTab0) && LoadGrassModels(grassMdls2, PC_PlantModelsTab1))
    {
        PC_PlantModelSlotTab[0] = PC_PlantModelsTab0;
        PC_PlantModelSlotTab[1] = PC_PlantModelsTab1;
        PC_PlantModelSlotTab[2] = PC_PlantModelsTab0;
        PC_PlantModelSlotTab[3] = PC_PlantModelsTab1;

        SetPlantModelsTab(0, PC_PlantModelsTab0);
        SetPlantModelsTab(1, PC_PlantModelSlotTab[0]);
        SetPlantModelsTab(2, PC_PlantModelSlotTab[0]);
        SetPlantModelsTab(3, PC_PlantModelSlotTab[0]);
        //SetCloseFarAlphaDist(3.0f, 60.0f);
        SetCloseFarAlphaDist(3.0f, 22.0f * powf(2.0, cfgGrassQuality->GetInt()));
    }
}
inline float lerp(float a, float b, float t)
{
    return a + t * (b - a);
}
RpMaterial* SetGrassModelProperties(RpMaterial* material, void* data)
{
    PPTriPlant* plant = (PPTriPlant*)data;
    material->texture = plant->texture_ptr;
    material->color   = plant->color;
    //RpMaterialSetTexture(material, plant->texture_ptr); // Deletes previous texture, SUS BEHAVIOUR
    // UPD: This is normal. But we need to keep our texture FOREVER. So not deleting it. EVER.
    return material;
}


// ---------------------------------------------------------------------------------------


uintptr_t GrassMaterialApplying_BackTo;
extern "C" void GrassMaterialApplying(RpGeometry* geometry, PPTriPlant* plant)
{
    // ULTRA EXPERIMENTAL AND DUMB WAY!
    CRGBA newGrassCol = *(CRGBA*)&plant->color;
    newGrassCol.a = 255;
    GeometrySetPrelitConstantColorForce(geometry, newGrassCol);
    // ULTRA EXPERIMENTAL AND DUMB WAY!

    RpGeometryForAllMaterials(geometry, SetGrassModelProperties, plant);
}
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
DECL_HOOKv(PlantSurfPropMgrInit)
{
    PlantSurfPropMgrInit();
    PlantSurfPropMgrLoadPlantsDat("PLANTS.DAT");
}
DECL_HOOKv(PlantMgrInit)
{
    PlantMgrInit(); // Acts like a CPlantMgr::ReloadConfig()
    InitPlantManager();
}
DECL_HOOKv(PlantMgrRender)
{
    PPTriPlant plant;
    
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)false);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)false);
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONALWAYS);

    for(int type = 0; type < MAXLOCTRIS; ++type)
    {
        CPlantLocTri* plantTris = m_CloseLocTriListHead[type];
        RwTexture** grassTex = PC_PlantSlotTextureTab[type];

        while(plantTris != NULL)
        {
            CPlantSurfProp* surface = GetSurfacePtr(plantTris->m_nSurfaceType);
            if(surface && IsSphereVisibleForCamera(TheCamera, &plantTris->m_Center, plantTris->m_SphereRadius))
            {
                CPlantSurfPropPlantData& surfProp = surface->m_aSurfaceProperties[0];
                //if(surfProp.m_nModelId != -1)
                {
                    plant.V1 = plantTris->m_V1;
                    plant.V2 = plantTris->m_V2;
                    plant.V3 = plantTris->m_V3;
                    plant.center = plantTris->m_Center;
                    plant.model_id = surfProp.m_nModelId;
                    plant.num_plants = ((plantTris->m_nMaxNumPlants[0] + 8) & 0xFFF8);
                    plant.scale.x = surfProp.m_fSizeScaleXY;
                    plant.scale.y = surfProp.m_fSizeScaleZ;
                    plant.texture_ptr = grassTex[surfProp.m_nUVOffset];
                    plant.color = *(RwRGBA*)&surfProp.m_Color;
                    plant.intensity = surfProp.m_Intensity;
                    plant.intensity_var = surfProp.m_IntensityVariation;
                    plant.seed = plantTris->m_Seed[0];
                    plant.scale_var_xy = surfProp.m_fSizeScaleXYVariation;
                    plant.scale_var_z = surfProp.m_fSizeScaleZVariation;
                    plant.wind_bend_scale = surfProp.m_fWindBendingScale;
                    plant.wind_bend_var = surfProp.m_fWindBendingVariation;
    
                    float intens = lerp(0.0333333f * plantTris->m_ColLighting.day,
                                        0.0333333f * plantTris->m_ColLighting.night,
                                        *m_fDNBalanceParam);
                    
                    plant.color.red *= intens;
                    plant.color.green *= intens;
                    plant.color.blue *= intens;

                    //int newal = plant.color.alpha + 80;
                    //plant.color.alpha = newal > 255 ? 255 : newal;

                    AddTriPlant(&plant, type);
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


void OnGrassQualityChanged(int oldVal, int newVal, void* data)
{
    SetCloseFarAlphaDist(3.0f, 22.0f * powf(2.0, newVal));
    cfgGrassQuality->SetInt(newVal);
    cfg->Save();
}


// ---------------------------------------------------------------------------------------


extern "C" void OnModLoad()
{
    logger->SetTag("SA Planter");
    
    pGTASA = aml->GetLib("libGTASA.so");
    hGTASA = aml->GetLibHandle("libGTASA.so");
    
    if(!pGTASA || !hGTASA)
    {
        logger->Info("Get a real GTA:SA first!");
        return;
    }

    GrassMaterialApplying_BackTo = pGTASA + 0x2CD434 + 0x1;
    aml->Redirect(pGTASA + 0x2CD42A + 0x1, (uintptr_t)GrassMaterialApplying_Patch);
    HOOKPLT(PlantSurfPropMgrInit,           pGTASA + 0x6721C8);
    HOOKPLT(PlantMgrInit,                   pGTASA + 0x673C90);
    HOOKPLT(PlantMgrRender,                 pGTASA + 0x6726D0);

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
    SET_TO(FileMgrCloseFile,                aml->GetSym(hGTASA, "_ZN8CFileMgr9CloseFileEj"));
    SET_TO(FileLoaderLoadLine,              aml->GetSym(hGTASA, "_ZN11CFileLoader8LoadLineEj"));
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

    sautils = (ISAUtils*)GetInterface("SAUtils");
    if(sautils)
    {
        aml->Write(pGTASA + 0x2CD342, "\xB1\xEE\xCF\xFA\x01\x99\xD1\xED\x00\x8A", 10);
        aml->PlaceB(pGTASA + 0x2CD34C + 0x1, pGTASA + 0x2CD35C + 0x1);

        cfgGrassQuality = cfg->Bind("GrassQuality", 1);
        cfgGrassQuality->Clamp(0, 2);

        sautils->AddClickableItem(eTypeOfSettings::SetType_Display, "Grass Quality", cfgGrassQuality->GetInt(), 0, 2, aGrassQualitySwitch, OnGrassQualityChanged, NULL);
        if(cfgGrassQuality->GetInt() != 1) OnGrassQualityChanged(1, cfgGrassQuality->GetInt(), NULL);
    }
}