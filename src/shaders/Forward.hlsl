// Standard forward rendering shader implementing basic Lambertian diffuse lighting and uniform ambient light

// ----------------------------------------------------------------------------
// Constant Buffers
// ----------------------------------------------------------------------------

// Uniform global variables across the entire frame
// Combined view and projection matrix
// World space position of camera
// Elapsed time in seconds
cbuffer FrameConstants : register(b0)
{
    float4x4 ViewProjection;
    float3 CameraPosition;
    float Time;
};

// Variables that will be changing per 3D mesh instance during the draw call
/// World Transformation matrix
// (RGBA) Diffuse color
cbuffer ObjectConstants : register(b1)
{
    float4x4 World;
    float4 BaseColor;
};

// ----------------------------------------------------------------------------
// Pipeline Structures
// ----------------------------------------------------------------------------

// Data structure bound to the Input Assembler stage from the Vertex Buffer
// Position: Coordinate of the vertex
// Normal: Surface normal direction
struct VSInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
};

// Data structure output by Vertex Shader and passed to the Rasterizer.
// The Rasterizer interpolates these values across triangles before the Pixel Shader.
// H stands for Homogenous meaning the coordinates have been prepared for flat screen.
// PositionH: System Value Position this specific variable contains the final screen-space coordinates
// NormalW: Stores the surface normal to calculate how light bounces off the surface
// PositionW: raw 3D coord of the vertex to calculate things like attenuation
struct VSOutput
{
    float4 PositionH : SV_Position;
    float3 NormalW : NORMAL;
    float3 PositionW : POSITION;
};

// ----------------------------------------------------------------------------
// Vertex Shader
// ----------------------------------------------------------------------------

// Transforms vertex attributes from local object space to world and clip space.
VSOutput VSMain(VSInput input)
{
    VSOutput output;

    // Transform position from Local Space to World Space (W=1.0 allows translation)
    float4 worldPos = mul(float4(input.Position, 1.0), World);

    // Save world space position for lighting calculations in the pixel shader
    output.PositionW = worldPos.xyz;

    // Transform World position to Clip Space for hardware rasterization
    output.PositionH = mul(worldPos, ViewProjection);

    // Transform normal vector to World Space (W=0.0 ignores translation, keeping it a direction)
    // Normalize handles any scaling uniformities introduced by the world matrix
    output.NormalW = normalize(mul(float4(input.Normal, 0.0), World).xyz);

    return output;
}

// ----------------------------------------------------------------------------
// Pixel  Buffers
// ----------------------------------------------------------------------------

// Calculates final per-pixel shading using Lambertian diffuse reflection.

float4 PSMain(VSOutput input) : SV_Target0
{
    // Direction of the global directional light source (normalized)
    float3 lightDir = normalize(float3(-0.3, -1.0, -0.2));

    // Compute diffuse intensity using N·L dot product
    // Normal is re-normalized to correct for linear interpolation errors across the triangle
    // Light vector is inverted to point from surface *to* light source
    // Saturate clamps values to [0.0, 1.0] to prevent negative lighting on backward-facing surfaces
    float ndotl = saturate(dot(normalize(input.NormalW), -lightDir));

    // Constant ambient term (0.08 RGB) to simulate bounced light and prevent pitch-black shadows
    float3 ambient = 0.08.xxx;

    // Combine diffuse and ambient factors, then multiply by material base color
    float3 lit = BaseColor.rgb * (ambient + ndotl);

    // Combine diffuse and ambient factors, then multiply by material base color
    return float4(lit, BaseColor.a);
}