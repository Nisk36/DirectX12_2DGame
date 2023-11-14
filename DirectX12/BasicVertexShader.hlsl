#include"BasicShaderHeader.hlsli"

Texture2D<float4> tex : register(t0); //0番スロットに設定されたテクスチャ
SamplerState smp : register(s0); //0番スロットに設定されたサンプラ

Output BasicVS( float4 pos : POSITION, float4 normal: NORMAL, float2 uv:TEXCOORD, min16uint2 boneno:  BONE_NO, min16uint weight: WEIGHT)
{
    Output output;
    output.svpos = mul(mul(viewproj, world), pos);//調点在表に対してmatを乗算することで頂点座標を行列でできる、シェーダーの行列演算は列優先であるため左方向にかける
    normal.w = 0;//平行移動成分を無効にする
    output.normal = mul(world, normal);//法線にもワールド変換を行う
    output.uv = uv;
	return output;
}
