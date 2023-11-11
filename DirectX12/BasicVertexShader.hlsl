#include"BasicShaderHeader.hlsli"


Output BasicVS( float4 pos : POSITION, float4 normal: NORMAL, float2 uv:TEXCOORD, min16uint2 boneno:  BONE_NO, min16uint weight: WEIGHT)
{
    Output output;
    output.svpos = mul(mat, pos);//調点在表に対してmatを乗算することで頂点座標を行列でできる、シェーダーの行列演算は列優先であるため左方向にかける
    output.uv = uv;
    output.normal = normal;
	return output;
}
