Texture2D<float> tex : register(t0); //�ʏ�e�N�X�`��

SamplerState smp : register(s0);//�T���v���[

struct Output
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};