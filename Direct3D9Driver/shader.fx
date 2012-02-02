float4x4 matWorld;
float4x4 matProj;
float4x4 matView;
float4x4 matWorldViewProj;
float4x4 matShadow;
float4x4 matWorldInverseTranspose;
		
float3 cameraPos;
			
float4	rgbaColor;
bool	bLighting;
bool	bTexDefault;
bool	bTexReflect;

bool	 bEnableTexDefault;
bool	 bEnableTexReflect;
bool	 bEnableTexBump;
bool	 bEnableTexOpac;
texture  texDefault;
texture  texReflect;
texture  texBump;
texture  texOpac;

float3 LightDir = {0,0,1};
float4 LightDiffuse = {1,1,1,1};

sampler colorMap = sampler_state
{
	Texture   = (texDefault);
	MipFilter = LINEAR;
	MinFilter = ANISOTROPIC;
	MagFilter = LINEAR;
};

sampler alphaMap = sampler_state
{
	Texture   = (texOpac);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
};

sampler normalMap = sampler_state
{
	Texture   = (texBump);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
};

sampler envMap = sampler_state
{
	Texture   = (texReflect);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
};

struct VS_INPUT
{
	float4 Pos 		: POSITION; 
	float3 Normal 		: NORMAL; 
	float4 Diffuse		: COLOR0;
	float2 TexCoord 	: TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 Pos   		: POSITION;
	float4 Diffuse		: COLOR0;
	float2 TextureUV  	: TEXCOORD0;
	float3 TextureENV 	: TEXCOORD1;
	float Reflect		: TEXCOORD2;
	float3 halfVector 	: TEXCOORD3;
	float3 lightDir 	: TEXCOORD4;
};

VS_OUTPUT VS(VS_INPUT IN)
{
	VS_OUTPUT OUT;
	
	OUT.Pos = mul(IN.Pos, matWorldViewProj);
	
	if(rgbaColor.a < 0)
		OUT.Diffuse  = IN.Diffuse;
	else 
		OUT.Diffuse  = rgbaColor;
	
	OUT.TextureUV = IN.TexCoord; 
	OUT.halfVector = float3(0,0,0);
	OUT.lightDir = float3(0,0,0);
	OUT.TextureENV = float3(0,0,0);
	OUT.Reflect = 0;

	if( !bLighting )
		return OUT;
			
	if( bEnableTexReflect )
	{
		float3 P = mul((float3)OUT.Pos,matView);					// position (view space)
		float3 N = normalize(mul(IN.Normal, (float3x3)matWorld));	 		// normal (view space)
		float3 V = normalize(P);							// view direction (view space)
		float3 G = normalize(2 * dot(N, -P) * N + P);					// Glance vector (view space)
		float  f = 0.5 - dot(V, N); f = 1 - 4 * f * f;		   			// fresnel term
		float  k_r = 0.2f;
		
		OUT.TextureENV = mul( G, transpose( matProj ) );			// Glance Vector to View Space
		OUT.Reflect = max( 0, k_r * f);							// Reflection
	}
		
	if( bEnableTexBump )
	{		

		float4 Tangent = mul( matView, float4(0,0,1,1)); // IN.Tangend
		float3 halfVector = normalize( LightDir + normalize(Tangent.xyz));	
		float3 n = mul(IN.Normal, (float3x3)matWorldInverseTranspose);
		float3 t = mul(Tangent.xyz, (float3x3)matWorldInverseTranspose);
		float3 b = cross(n, t) * Tangent.w;
		float3x3 tbnMatrix = float3x3(t.x, b.x, n.x,
					      t.y, b.y, n.y,
					      t.z, b.z, n.z);
					      
		OUT.halfVector = mul(halfVector, tbnMatrix);
		OUT.lightDir = mul(LightDir, tbnMatrix);
	}
		
	float3 vNormalSceneSpace = normalize(mul(IN.Normal, (float3x3)matView));
	float3 vTotalLightDiffuse = LightDiffuse * ( max(dot(vNormalSceneSpace, LightDir), 0.0) + max(dot(vNormalSceneSpace, -LightDir), 0.0) );

	OUT.Diffuse.rgb *= vTotalLightDiffuse * 1.2;  
	
//	OUT.Diffuse.rgb = (IN.Normal*IN.Normal)/2; 

	return OUT;
}

float4 PS( VS_OUTPUT IN ) : COLOR0
{
	float4 OUT = IN.Diffuse;
	
	if( bEnableTexBump )
	{
		float3 n = normalize( tex2D( normalMap, IN.TextureUV ) * 2.0 - 1.0 );
		float3 h = normalize(IN.halfVector);
		float3 l = normalize(IN.lightDir);
		
		float nDotL = saturate(dot(n, l));
		float nDotH = saturate(dot(n, h));
		float power = (nDotL == 0.0f) ? 0.0f : pow(nDotH, 2.5 /*material.shininess*/);
		
		OUT *= 0.2 + nDotL;
	}
	
	if( bEnableTexDefault )
		OUT *= tex2D( colorMap, IN.TextureUV );

	if( bEnableTexOpac )
	{
		float a = tex2D( alphaMap, IN.TextureUV );
		OUT.a = a*2;   	
	}
	

	if( bEnableTexReflect )
		OUT += IN.Reflect * texCUBE( envMap, IN.TextureENV );
	 
	OUT += IN.Reflect * 0.3;
	
	return OUT;
}

#define SHADOW

#ifdef SHADOW
struct VS_OUTPUT_SHADOW
{
	float4 Position	: POSITION;
	float4 Diffuse	: COLOR0;
};


VS_OUTPUT_SHADOW VS_SHADOW( float4 vPos : POSITION, float3 vNormal : NORMAL)
{
	VS_OUTPUT_SHADOW Output;
	
	Output.Position = mul(vPos, mul(mul(matWorld,matShadow), mul(matView,matProj)));
	Output.Diffuse.r = 0.2;		
	Output.Diffuse.g = 0.2;  
	Output.Diffuse.b = 0.2;  
	Output.Diffuse.a = 1.0;  	
	
	return Output;	
}

struct PS_OUTPUT_SHADOW
{
	float4 Color	: COLOR0;
	float Depth	: DEPTH;
};

PS_OUTPUT_SHADOW PS_SHADOW( float4 Diff : COLOR0, float2 Tex  : TEXCOORD0)
{
	PS_OUTPUT_SHADOW Output;
	Output.Color = Diff;
	Output.Depth = 1;
	return Output;
}
#endif

technique Render
{
	pass P0
	{   
		//AlphaBlendEnable = rgbaColor.a > 0;

		VertexShader = compile vs_2_0 VS();
		PixelShader  = compile ps_2_0 PS();
	}
	
#ifdef SHADOW
	
	pass P1
	{
		//AlphaBlendEnable = TRUE;

		VertexShader = compile vs_2_0 VS_SHADOW();
		PixelShader  = compile ps_2_0 PS_SHADOW();
	}
#endif
}

technique Render_without_shadow
{
	pass P0
	{   
		VertexShader = compile vs_2_0 VS();
		PixelShader  = compile ps_2_0 PS();
	}
}


