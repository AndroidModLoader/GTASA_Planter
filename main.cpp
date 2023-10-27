#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>

MYMODCFG(net.rusjj.plantes, GTASA Planter, 1.0, RusJJ)
NEEDGAME(com.rockstargames.gtasa)

uintptr_t pGTASA;
void* hGTASA;

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

    
}