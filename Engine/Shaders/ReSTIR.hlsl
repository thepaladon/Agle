// Sources: 
// https://interplayoflight.wordpress.com/2023/12/17/a-gentler-introduction-to-restir/
// and "A Gentle Introduction to ReSTIR"

#include "ShaderHeaders/WavefrontStructsGPU.h" 
#define MAX_UINT 4294967295


// Function for Updating a ReSTIR Light Reservoir
bool UpdateReservoir(inout Reservoir reservoir, uint lightID, float pHat, float pdf, float c, inout uint seed)
{
	
    float weight = pHat / pdf;
    reservoir.m_WSum += weight;
    reservoir.m_EvalLights += c;
	
    if (rand(seed) < weight / (reservoir.m_WSum + 0.00001f))
    {
        reservoir.m_PHat = pHat;
        reservoir.m_PickedLightIdx = lightID;
        return true;
    }

    return false;
}


void CalculateReservoirWeight(inout Reservoir reservoir)
{
	
    if (reservoir.m_PHat >= 0.0001) // NaN Protect
	{
		reservoir.m_Weight = rcp(reservoir.m_PHat) * (rcp(reservoir.m_EvalLights)  * reservoir.m_WSum);
	}
	else
	{
		reservoir.m_Weight = 0.0;
	}
}

bool isReservoirValid(Reservoir reservoir)
{
	const bool defaultReservoir = reservoir.m_PickedLightIdx == MAX_UINT; // true if picked light is not maxuint
	const bool validWeight = reservoir.m_Weight > 0.0; // true if weight is position
	return validWeight && !defaultReservoir;
}

Reservoir InitEmptyReservoir()
{
	Reservoir reservoir;
	reservoir.m_PickedLightIdx = MAX_UINT; // max val, to signify not picked light
	reservoir.m_Weight = 0.0;
	reservoir.m_WSum = 0.0;
	reservoir.m_EvalLights = 0.0;
	reservoir.m_PHat = 0.0;
	reservoir.padding = float3(0.0, 0.0, 0.0);
	return reservoir;
}

// ToDo Make sure to calculate p_hat after calling this
Reservoir CombineReservoirs(Reservoir r1, Reservoir r2, inout uint seed)
{
    Reservoir r = InitEmptyReservoir();
    UpdateReservoir(r, r1.m_PickedLightIdx, r1.m_PHat, rcp(r1.m_Weight * r1.m_EvalLights), r1.m_EvalLights, seed);
    UpdateReservoir(r, r2.m_PickedLightIdx, r2.m_PHat, rcp(r2.m_Weight * r2.m_EvalLights), r2.m_EvalLights, seed);
	
    CalculateReservoirWeight(r);
	return r;
}

