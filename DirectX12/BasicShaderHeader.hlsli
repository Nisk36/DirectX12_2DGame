//頂点シェーダーからピクセルシェーダーのやり取りに使用する構造体

struct Output
{
    float4 svpos : SV_POSITION;//システム用
    float4 normal : NORMAL; //法線ベクトル
    float2 uv : TEXCOORD;
};

//定数バッファ
cbuffer cbuff0 : register(b0)
{
    matrix world; //ワールド変換行列
    matrix viewproj; //ビュープロジェクション行列
};

cbuffer Material : register(b1)
{
    float4 diffuse;
    float4 specular;
    float ambient;
}