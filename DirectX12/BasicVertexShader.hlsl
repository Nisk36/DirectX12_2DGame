#include"BasicShaderHeader.hlsli"

Texture2D<float4> tex : register(t0); //0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
SamplerState smp : register(s0); //0�ԃX���b�g�ɐݒ肳�ꂽ�T���v��

Output BasicVS( float4 pos : POSITION, float4 normal: NORMAL, float2 uv:TEXCOORD, min16uint2 boneno:  BONE_NO, min16uint weight: WEIGHT)
{
    Output output;
    output.svpos = mul(mul(viewproj, world), pos);//���_�ݕ\�ɑ΂���mat����Z���邱�ƂŒ��_���W���s��łł���A�V�F�[�_�[�̍s�񉉎Z�͗�D��ł��邽�ߍ������ɂ�����
    normal.w = 0;//���s�ړ������𖳌��ɂ���
    output.normal = mul(world, normal);//�@���ɂ����[���h�ϊ����s��
    output.uv = uv;
	return output;
}
