#include"BasicShaderHeader.hlsli"


Output BasicVS( float4 pos : POSITION, float4 normal: NORMAL, float2 uv:TEXCOORD, min16uint2 boneno:  BONE_NO, min16uint weight: WEIGHT)
{
    Output output;
    output.svpos = mul(mat, pos);//���_�ݕ\�ɑ΂���mat����Z���邱�ƂŒ��_���W���s��łł���A�V�F�[�_�[�̍s�񉉎Z�͗�D��ł��邽�ߍ������ɂ�����
    output.uv = uv;
    output.normal = normal;
	return output;
}
