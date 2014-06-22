//--------------------------------------------------------------------------------------
// File: BasicHLSL.fx
//
// The effect file for the BasicHLSL sample.  
// 
// Copyright (c) ‚¨‚¿‚á‚Á‚±LAB. All rights reserved.
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
texture g_MeshTexture;              // Color texture for mesh

float4 g_diffuse;
float3 g_ambient;
float3 g_specular;
float g_power;
float3 g_emissive;

int g_nNumLights;
float3 g_LightDir[3];               // Light's direction in world space
float4 g_LightDiffuse[3];           // Light's diffuse color
float3 g_LightAmbient;              // Light's ambient color

float    g_fTime;                   // App's time in seconds
float4x4 g_mWorld;                  // World matrix for object
float4x4 g_mVP;    // View * Projection matrix
float3	 g_EyePos;

float4	g_mBoneQ[102];
float3	g_mBoneTra[102];


//--------------------------------------------------------------------------------------
// Texture samplers
//--------------------------------------------------------------------------------------
sampler MeshTextureSampler = 
sampler_state
{
    Texture = <g_MeshTexture>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
};



float4x4 CalcBlendMat( in float4 boneq[4], in float3 bonetra[4], in float4 bweight )
{
	float4x4 retmat = 0;

	float4x4 tmpmat;
	float dat00, dat01, dat02;
	float dat10, dat11, dat12;
	float dat20, dat21, dat22;
	float x, y, z, w;

	float weight[4] = { bweight.x, bweight.y, bweight.z, bweight.w };

    for(int i=0; i<4; i++ ){
		x = boneq[i].x;
		y = boneq[i].y;
		z = boneq[i].z;
		w = boneq[i].w;

		dat00 = w * w + x * x - y * y - z * z;
		dat01 = 2.0f * ( x * y + w * z );
		dat02 = 2.0f * ( x * z - w * y );

		dat10 = 2.0f * ( x * y - w * z );
		dat11 = w * w - x * x + y * y - z * z;
		dat12 = 2.0f * ( y * z + w * x );

		dat20 = 2.0f * ( x * z + w * y );
		dat21 = 2.0f * ( y * z - w * x );
		dat22 = w * w - x * x - y * y + z * z;

		tmpmat = float4x4(
			dat00, dat01, dat02, 0.0f,
			dat10, dat11, dat12, 0.0f,
			dat20, dat21, dat22, 0.0f,
			bonetra[i].x, bonetra[i].y, bonetra[i].z, 1.0f
		);


		retmat += tmpmat * weight[i];		
	}
	return retmat;
}



//--------------------------------------------------------------------------------------
// Vertex shader output structure
//--------------------------------------------------------------------------------------
struct VS_OUTPUT
{
    float4 Position   : POSITION;   // vertex position 
    float4 Diffuse    : COLOR0;     // vertex diffuse color (note that COLOR0 is clamped from 0..1)
	float3 Specular	  : TEXCOORD0;
    float2 TextureUV  : TEXCOORD1;  // vertex texture coords 
};

struct VS_LINEOUTPUT
{
    float4 Position   : POSITION;   // vertex position 
    float4 Diffuse    : COLOR0;     // vertex diffuse color (note that COLOR0 is clamped from 0..1)
};

struct VS_SPRITEOUTPUT
{
    float4 Position   : POSITION;   // vertex position 
    float4 Diffuse    : COLOR0;     // vertex diffuse color (note that COLOR0 is clamped from 0..1)
    float2 TextureUV  : TEXCOORD1;  // vertex texture coords 
};

//--------------------------------------------------------------------------------------
// This shader computes standard transform and lighting
//--------------------------------------------------------------------------------------
VS_OUTPUT RenderSceneBoneVS( float4 vPos : POSITION, 
                         float3 vNormal : NORMAL,
                         float2 vTexCoord0 : TEXCOORD0,
						 float4 bweight : BLENDWEIGHT,
						 float4 bindices : BLENDINDICES,
                         uniform int nNumLights )
{
    VS_OUTPUT Output;
    float4 wPos;

	float4 boneq[4];
	float3 bonetra[4];

	boneq[0] = g_mBoneQ[bindices.x];
	boneq[1] = g_mBoneQ[bindices.y];
	boneq[2] = g_mBoneQ[bindices.z];
	boneq[3] = g_mBoneQ[bindices.w];
	   
	bonetra[0] = g_mBoneTra[bindices.x];
	bonetra[1] = g_mBoneTra[bindices.y];
	bonetra[2] = g_mBoneTra[bindices.z];
	bonetra[3] = g_mBoneTra[bindices.w];

	float4x4 finalmat;
	finalmat = CalcBlendMat( boneq, bonetra, bweight );

	wPos = mul( vPos, finalmat );
	Output.Position = mul( wPos, g_mVP );
	wPos /= wPos.w;
    
    float3 wNormal;
    wNormal = normalize(mul(vNormal, (float3x3)finalmat)); // normal (world space)
    
    float3 totaldiffuse = float3(0,0,0);
    float3 totalspecular = float3(0,0,0);
	float calcpower = g_power * 0.1f;

    for(int i=0; i<nNumLights; i++ ){
		float nl;
		float3 h;
		float nh;	
		float4 tmplight;
		
		nl = dot( wNormal, g_LightDir[i] );
		h = normalize( ( g_LightDir[i] + g_EyePos - wPos.xyz ) * 0.5f );
		nh = dot( wNormal, h );

        totaldiffuse += g_LightDiffuse[i] * max(0,dot(wNormal, g_LightDir[i]));
		totalspecular +=  ((nl) < 0) || ((nh) < 0) ? 0 : ((nh) * calcpower);
	}

    Output.Diffuse.rgb = g_diffuse.rgb * totaldiffuse.rgb 
		+ g_ambient + g_emissive;   
    Output.Diffuse.a = g_diffuse.a; 
    
	Output.Specular = g_specular * totalspecular;

    Output.TextureUV = vTexCoord0; 
    
    return Output;    
}
VS_OUTPUT RenderSceneBoneNLightVS( float4 vPos : POSITION, 
                         float3 vNormal : NORMAL,
                         float2 vTexCoord0 : TEXCOORD0,
						 float4 bweight : BLENDWEIGHT,
						 float4 bindices : BLENDINDICES )
{
    VS_OUTPUT Output;
    float4 wPos;

	float4 boneq[4];
	float3 bonetra[4];

	boneq[0] = g_mBoneQ[bindices.x];
	boneq[1] = g_mBoneQ[bindices.y];
	boneq[2] = g_mBoneQ[bindices.z];
	boneq[3] = g_mBoneQ[bindices.w];
	   
	bonetra[0] = g_mBoneTra[bindices.x];
	bonetra[1] = g_mBoneTra[bindices.y];
	bonetra[2] = g_mBoneTra[bindices.z];
	bonetra[3] = g_mBoneTra[bindices.w];

	float4x4 finalmat;
	finalmat = CalcBlendMat( boneq, bonetra, bweight );

	wPos = mul( vPos, finalmat );
	Output.Position = mul( wPos, g_mVP );
	wPos /= wPos.w;
    
    float3 wNormal;
    wNormal = normalize(mul(vNormal, (float3x3)finalmat)); // normal (world space)
    
    Output.Diffuse.rgb = g_diffuse.rgb + g_ambient + g_emissive;   
    Output.Diffuse.a = g_diffuse.a; 
    
	Output.Specular = float3( 0.0f, 0.0f, 0.0f );

    Output.TextureUV = vTexCoord0; 
    
    return Output;    
}

VS_OUTPUT RenderSceneNoBoneVS( float4 vPos : POSITION, 
                         float3 vNormal : NORMAL,
                         float2 vTexCoord0 : TEXCOORD0,
                         uniform int nNumLights )
{
    VS_OUTPUT Output;
    float4 wPos;

	float4x4 finalmat = g_mWorld;

	wPos = mul( vPos, finalmat );
	Output.Position = mul( wPos, g_mVP );
	wPos /= wPos.w;
    
    float3 wNormal;
    wNormal = normalize(mul(vNormal, (float3x3)finalmat)); // normal (world space)
    
    float3 totaldiffuse = float3(0,0,0);
    float3 totalspecular = float3(0,0,0);
	float calcpower = g_power * 0.1f;

    for(int i=0; i<nNumLights; i++ ){
		float nl;
		float3 h;
		float nh;	
		float4 tmplight;
		
		nl = dot( wNormal, g_LightDir[i] );
		h = normalize( ( g_LightDir[i] + g_EyePos - wPos.xyz ) * 0.5f );
		nh = dot( wNormal, h );

        totaldiffuse += g_LightDiffuse[i] * max(0,dot(wNormal, g_LightDir[i]));
		totalspecular +=  ((nl) < 0) || ((nh) < 0) ? 0 : ((nh) * calcpower);
	}

    Output.Diffuse.rgb = g_diffuse.rgb * totaldiffuse.rgb 
		+ g_ambient + g_emissive;   
    Output.Diffuse.a = g_diffuse.a; 
    
	Output.Specular = g_specular * totalspecular;

    Output.TextureUV = vTexCoord0; 
    
    return Output;    
}

VS_OUTPUT RenderSceneNoBoneNLightVS( float4 vPos : POSITION, 
                         float3 vNormal : NORMAL,
                         float2 vTexCoord0 : TEXCOORD0 )
{
    VS_OUTPUT Output;
    float4 wPos;

	float4x4 finalmat = g_mWorld;

	wPos = mul( vPos, finalmat );
	Output.Position = mul( wPos, g_mVP );
	wPos /= wPos.w;
    
    float3 wNormal;
    wNormal = normalize(mul(vNormal, (float3x3)finalmat)); // normal (world space)
    

    Output.Diffuse.rgb = g_diffuse.rgb + g_ambient + g_emissive;   
    Output.Diffuse.a = g_diffuse.a; 
    
	Output.Specular = float3( 0.0f, 0.0f, 0.0f );

    Output.TextureUV = vTexCoord0; 
    
    return Output;    
}


VS_LINEOUTPUT RenderSceneLineVS( float4 vPos : POSITION )
{
    VS_LINEOUTPUT Output;
    float4 wPos;

	float4x4 finalmat = g_mWorld;

	wPos = mul( vPos, finalmat );
	Output.Position = mul( wPos, g_mVP );


    Output.Diffuse.rgb = g_diffuse.rgb;
	Output.Diffuse.a = g_diffuse.a; 
        
    return Output;    
}


VS_SPRITEOUTPUT RenderSceneSpriteVS( float4 vPos : POSITION, 
                         float2 vTexCoord0 : TEXCOORD0 )
{
    VS_SPRITEOUTPUT Output;
	Output.Position = vPos;
    Output.Diffuse.rgb = g_diffuse.rgb;   
    Output.Diffuse.a = g_diffuse.a; 
    Output.TextureUV = vTexCoord0; 
    
    return Output;    
}



//--------------------------------------------------------------------------------------
// Pixel shader output structure
//--------------------------------------------------------------------------------------
struct PS_OUTPUT
{
    float4 RGBColor : COLOR0;  // Pixel color    
};


//--------------------------------------------------------------------------------------
// This shader outputs the pixel's color by modulating the texture's
//       color with diffuse material color
//--------------------------------------------------------------------------------------
PS_OUTPUT RenderScenePSTex( VS_OUTPUT In ) 
{ 
    PS_OUTPUT Output;
    Output.RGBColor = tex2D(MeshTextureSampler, In.TextureUV) * In.Diffuse + float4( In.Specular, 0 );
    return Output;
}

PS_OUTPUT RenderScenePSNotex( VS_OUTPUT In ) 
{ 
    PS_OUTPUT Output;
    Output.RGBColor = In.Diffuse + float4( In.Specular, 0 );
    return Output;
}

PS_OUTPUT RenderScenePSLine( VS_LINEOUTPUT In ) 
{ 
    PS_OUTPUT Output;
    Output.RGBColor = In.Diffuse;
    return Output;
}

PS_OUTPUT RenderScenePSSprite( VS_SPRITEOUTPUT In ) 
{ 
    PS_OUTPUT Output;
    Output.RGBColor = tex2D(MeshTextureSampler, In.TextureUV) * In.Diffuse;
    return Output;
}


//--------------------------------------------------------------------------------------
// Renders scene to render target
//--------------------------------------------------------------------------------------
technique RenderBoneL0
{
    pass P0
    {          
        VertexShader = compile vs_3_0 RenderSceneBoneNLightVS();
        PixelShader  = compile ps_3_0 RenderScenePSTex();
    }
    pass P1
    {          
        VertexShader = compile vs_3_0 RenderSceneBoneNLightVS();
        PixelShader  = compile ps_3_0 RenderScenePSNotex();
    }

}

technique RenderBoneL1
{
    pass P0
    {          
        VertexShader = compile vs_3_0 RenderSceneBoneVS( 1 );
        PixelShader  = compile ps_3_0 RenderScenePSTex();
    }
    pass P1
    {          
        VertexShader = compile vs_3_0 RenderSceneBoneVS( 1 );
        PixelShader  = compile ps_3_0 RenderScenePSNotex();
    }

}

technique RenderBoneL2
{
    pass P0
    {          
        VertexShader = compile vs_3_0 RenderSceneBoneVS( 2 );
        PixelShader  = compile ps_3_0 RenderScenePSTex();
    }
    pass P1
    {          
        VertexShader = compile vs_3_0 RenderSceneBoneVS( 2 );
        PixelShader  = compile ps_3_0 RenderScenePSNotex();
    }

}

technique RenderBoneL3
{
    pass P0
    {          
        VertexShader = compile vs_3_0 RenderSceneBoneVS( 3 );
        PixelShader  = compile ps_3_0 RenderScenePSTex();
    }
    pass P1
    {          
        VertexShader = compile vs_3_0 RenderSceneBoneVS( 3 );
        PixelShader  = compile ps_3_0 RenderScenePSNotex();
    }

}

technique RenderNoBoneL0
{
    pass P0
    {          
        VertexShader = compile vs_3_0 RenderSceneNoBoneNLightVS();
        PixelShader  = compile ps_3_0 RenderScenePSTex();
    }
    pass P1
    {          
        VertexShader = compile vs_3_0 RenderSceneNoBoneNLightVS();
        PixelShader  = compile ps_3_0 RenderScenePSNotex();
    }

}


technique RenderNoBoneL1
{
    pass P0
    {          
        VertexShader = compile vs_3_0 RenderSceneNoBoneVS( 1 );
        PixelShader  = compile ps_3_0 RenderScenePSTex();
    }
    pass P1
    {          
        VertexShader = compile vs_3_0 RenderSceneNoBoneVS( 1 );
        PixelShader  = compile ps_3_0 RenderScenePSNotex();
    }

}

technique RenderNoBoneL2
{
    pass P0
    {          
        VertexShader = compile vs_3_0 RenderSceneNoBoneVS( 2 );
        PixelShader  = compile ps_3_0 RenderScenePSTex();
    }
    pass P1
    {          
        VertexShader = compile vs_3_0 RenderSceneNoBoneVS( 2 );
        PixelShader  = compile ps_3_0 RenderScenePSNotex();
    }

}

technique RenderNoBoneL3
{
    pass P0
    {          
        VertexShader = compile vs_3_0 RenderSceneNoBoneVS( 3 );
        PixelShader  = compile ps_3_0 RenderScenePSTex();
    }
    pass P1
    {          
        VertexShader = compile vs_3_0 RenderSceneNoBoneVS( 3 );
        PixelShader  = compile ps_3_0 RenderScenePSNotex();
    }

}

technique RenderLine
{
    pass P0
    {          
        VertexShader = compile vs_3_0 RenderSceneLineVS();
        PixelShader  = compile ps_3_0 RenderScenePSLine();
    }
}

technique RenderSprite
{
    pass P0
    {          
        VertexShader = compile vs_3_0 RenderSceneSpriteVS();
        PixelShader  = compile ps_3_0 RenderScenePSSprite();
    }
}
