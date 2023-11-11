#include"BasicShaderHeader.hlsli"


Output BasicVS( float4 pos : POSITION, float2 uv:TEXCOORD )
{
    Output output;
    output.svpos = mul(mat, pos);//調点在表に対してmatを乗算することで頂点座標を行列でできる、シェーダーの行列演算は列優先であるため左方向にかける
    output.uv = uv;
	return output;
}
