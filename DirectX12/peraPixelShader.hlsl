#include"peraShaderHeader.hlsli"


//ピクセルシェーダー
float4 ps(Output input) : SV_Target
{
    //GrayScale
    /*
    float4 col = tex.Sample(smp, input.uv);
    float Y = dot(col.rgb, float3(0.299, 0.587, 0.114));
    return float4(Y, Y, Y, 1);
    */
    
    //Reverse
    /*
    float4 col = tex.Sample(smp, input.uv);
    return float4(float3(1.0f, 1.0f, 1.0f) - col.rgb, col.a);
    */
    
    //階調を落とす
    /*
    float4 col = tex.Sample(smp, input.uv);
    return float4(col.rgb - fmod(col.rgb, 0.25f), col.a);
    */
    
    //画素平均化ぼかし
	/*
	float4 col = tex.Sample(smp, input.uv);
	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);

	float dx = 1.0f / w;
	float dy = 1.0f / h;
	float4 ret = float4(0, 0, 0, 0);

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy));
	ret += tex.Sample(smp, input.uv + float2(0, -2 * dy));
	ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy));
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0));
	ret += tex.Sample(smp, input.uv);
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 0));
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy));
	ret += tex.Sample(smp, input.uv + float2(0, 2 * dy));
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy));

	return ret / 9.0f;
	*/
    
    // エンボス処理
	/*
	float4 col = tex.Sample(smp, input.uv);
	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);

	float dx = 1.0f / w;
	float dy = 1.0f / h;
	float4 ret = float4(0, 0, 0, 0);

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 2;
	ret += tex.Sample(smp, input.uv + float2(0, -2 * dy));
	ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)) * 0;
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0));
	ret += tex.Sample(smp, input.uv);
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 0)) * -1;
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy)) * 0;
	ret += tex.Sample(smp, input.uv + float2(0, 2 * dy)) * -1;
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy)) * -2;

	return ret;
	*/
	
	// シャープネス
	/*
	float4 col = tex.Sample(smp, input.uv);
	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);

	float dx = 1.0f / w;
	float dy = 1.0f / h;
	float4 ret = float4(0, 0, 0, 0);

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 0;
	ret += tex.Sample(smp, input.uv + float2(0, -2 * dy)) * -1;
	ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)) * 0;
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)) * -1;
	ret += tex.Sample(smp, input.uv) * 5;
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 0)) * -1;
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy)) * 0;
	ret += tex.Sample(smp, input.uv + float2(0, 2 * dy)) * -1;
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy)) * 0;

	return ret;
	*/
	
	// 輪郭線抽出
	/*
    float4 col = tex.Sample(smp, input.uv);
	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);

	float dx = 1.0f / w;
	float dy = 1.0f / h;
	float4 ret = float4(0, 0, 0, 0);

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 0;
	ret += tex.Sample(smp, input.uv + float2(0, -2 * dy)) * -1;
	ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)) * 0;
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)) * -1;
	ret += tex.Sample(smp, input.uv) * 4;
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 0)) * -1;
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy)) * 0;
	ret += tex.Sample(smp, input.uv + float2(0, 2 * dy)) * -1;
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy)) * 0;

	float grayscale = dot(ret, float3(0.299, 0.587, 0.114));
	grayscale = pow(1.0f - grayscale, 10.0f);
	grayscale = step(0.2f, grayscale);
    return float4(grayscale.xxx, col.a);
	*/
	
	
}
