#include"peraShaderHeader.hlsli"


//�s�N�Z���V�F�[�_�[
float4 ps(Output input) : SV_Target
{
    return tex.Sample(smp, input.uv);
}
