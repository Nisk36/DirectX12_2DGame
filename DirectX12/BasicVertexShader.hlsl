#include"BasicShaderHeader.hlsli"


Output BasicVS( float4 pos : POSITION, float2 uv:TEXCOORD )
{
    Output output;
    output.svpos = mul(mat, pos);//���_�ݕ\�ɑ΂���mat����Z���邱�ƂŒ��_���W���s��łł���A�V�F�[�_�[�̍s�񉉎Z�͗�D��ł��邽�ߍ������ɂ�����
    output.uv = uv;
	return output;
}
