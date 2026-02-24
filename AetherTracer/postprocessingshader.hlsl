cbuffer Params : register(b1)
{
    uint stage;
    float white_point;
    uint num_samples;
}

Texture2D<float4> accumulationTexture : register(t0, space0);
RWTexture2D<float4> renderTarget : register(u2, space0);

RWBuffer<uint> maxLumBuffer : register(u3, space0);

groupshared float g_maxLum[256];

// ACES Constants

const static float3x3 aces_input_matrix =
{
    float3(0.59719f, 0.35458f, 0.04823f),
    float3(0.07600f, 0.90834f, 0.01566f),
    float3(0.02840f, 0.13383f, 0.83777f)
};

const static float3x3 aces_output_matrix =
{
    float3(1.60475f, -0.53108f, -0.07367f),
    float3(-0.10208f, 1.10813f, -0.00605f),
    float3(-0.00327f, -0.07276f, 1.07602f)
};

// Find maximum luminance

void maxLuminance(uint3 dispatchID : SV_DispatchThreadID, uint3 groupThreadID : SV_GroupThreadID, uint groupIndex : SV_GroupIndex)
{
    uint2 dim;
    
    accumulationTexture.GetDimensions(dim.x, dim.y);
    if (dispatchID.x >= dim.x || dispatchID.y >= dim.y)
    {
        g_maxLum[groupIndex] = 0.0f;
    }
    
    
    float3 accum = accumulationTexture.Load(int3(dispatchID.xy, 0)).rgb;
    float luminance = 0.2126f * accum.r + 0.7152f * accum.g + 0.0722f * accum.b;
    
    g_maxLum[groupIndex] = luminance;
    GroupMemoryBarrierWithGroupSync();
    
    // parallel reduction in groupshared memory
    for (uint i = 128; i > 0; i >>= 1)
    {
        if (groupIndex < i)
        {
            g_maxLum[groupIndex] = max(g_maxLum[groupIndex], g_maxLum[groupIndex + i]);
        }
        GroupMemoryBarrierWithGroupSync();
    }
    
    // one thread per group does global atomic max
    if (groupIndex == 0)
    {
        //uint lumBits = asuint(g_maxLum[0]);
        uint lumBits = asuint(20.0f);
        InterlockedMax(maxLumBuffer[0], lumBits);

    }
    
}

// Reinhard
void extendedReinhard(inout float3 accum)
{
    
    float luminance = 0.2126f * accum.r + 0.7152f * accum.g + 0.0722f * accum.b;
    
    if (luminance > 0)
    {
        float mappedLuminance = (luminance * (1.0f + (luminance / (white_point * white_point)))) / (1.0f + luminance);
        
        accum = accum / luminance * mappedLuminance;
        
        float gamma = 2.2f;
        float invGamma = 1.0f / gamma;
        
        accum = pow(accum, invGamma);
       
    }

}


// Hable Filmic
float3 HableParameter(float3 input)
{
    float A = 0.15f;
    float B = 0.50f;
    float C = 0.10f;
    float D = 0.20f;
    float E = 0.02f;
    float F = 0.30f;
    return ((input * (A * input + C * B) + D * E) / (input * (A * input + B) + D * F)) - E / F;
}

void HableFilmic(inout float3 accum)
{
    float exposure_bias = 2.0f;
    
    float3 curr = HableParameter(accum * exposure_bias);
    
    float3 white_scale = float3(1.0f, 1.0f, 1.0f) / HableParameter(white_point);
    
    accum = curr * white_scale;
}

// Aces Filmic
float3 mul(const float3x3 m, const float3 v)
{    
    float x = m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2];
    float y = m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2];
    float z = m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2];
    return float3(x, y, z);
}

float3 rtt_and_odt_fit(float3 v)
{
    float3 a = v * (v + 0.0245786f) - 0.000090537f;
    float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

void aces_filmic(inout float3 accum)
{
    accum = mul(aces_input_matrix, accum);
    accum = rtt_and_odt_fit(accum);
    accum = mul(aces_output_matrix, accum);
}

[numthreads(16, 16, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID, uint3 groupThreadID : SV_GroupThreadID, uint groupIndex : SV_GroupIndex)
{
    if (stage == 0)
    {
        maxLuminance(dispatchID, groupThreadID, groupIndex);
    }
    
    uint2 dim;
    accumulationTexture.GetDimensions(dim.x, dim.y);
    if (dispatchID.x >= dim.x || dispatchID.y >= dim.y)
        return;
    
    float3 accum = accumulationTexture.Load(int3(dispatchID.xy, 0)).rgb;
    accum /= (float) num_samples;
    
    
    if (stage == 1)
    {
        extendedReinhard(accum);
    }
    else if (stage == 2)
    {
        HableFilmic(accum);
    }
    else if (stage == 3)
    {
        aces_filmic(accum);
    }
    
    renderTarget[dispatchID.xy] = float4(accum, 1.0f);

}