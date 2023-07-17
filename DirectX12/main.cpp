#include <Windows.h>
#include <tchar.h>
#include <string>
#include <vector>
#ifdef _DEBUG
#include <iostream>
#include <tchar.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#endif

using namespace std;

// ライブラリのリンクの設定
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")



//@brief コンソール画面でフォーマット付き文字列を表示
//@param formatフォーマット(%d, %fとか)
//@param 可変長引数
//@remarks この関数はデバッグ用です。デバッグ時にしか動作しない
void DebugOutputFormatString(const char* format, ...) 
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);
#endif
}

//ウィンドウプロシージャ
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	//ウィンドウが破棄されたら呼ばれる
	if (msg == WM_DESTROY) {
		PostQuitMessage(0);//OSに対して「このアプリを終わる」と伝える
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

const unsigned int window_width = 1280;
const unsigned int window_height = 720;

// 基本オブジェクトの受け皿となる変数
ID3D12Device* _dev = nullptr;
IDXGIFactory6* _dxgiFactory = nullptr;
IDXGISwapChain4* _swapChain = nullptr;



#ifdef _DEBUG
int main() {
	//ウィンドウクラスの生成&登録
	WNDCLASSEX w = {};

	w.cbSize = sizeof(WNDCLASSEX);
	//コールバック関数の指定
	w.lpfnWndProc = (WNDPROC)WindowProcedure;
	//アプリケーションクラス名
	w.lpszClassName = _T("DX12Sample");
	//ハンドルの取得
	w.hInstance = GetModuleHandle(nullptr);
	
	//アプリケーションクラス
	RegisterClassEx(&w);
	//ウィンドウサイズ設定
	RECT wrc = { 0, 0, window_width, window_height };
	//関数を用いてウィンドウのサイズ補正
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
	//ウィンドウオブジェクトの生成
	HWND hwnd = CreateWindow(w.lpszClassName,//クラス名指定
		_T("DX12テスト"),//タイトルバー文字
		WS_OVERLAPPEDWINDOW,//タイトルバーと境界線があるウィンドウ
		CW_USEDEFAULT,//表示x座標はOSに任せる
		CW_USEDEFAULT,//表示y座標も
		wrc.right-wrc.left,//ウィンドウ幅
		wrc.bottom-wrc.top,//ウィンドウ高さ
		nullptr,//親ウィンドウハンドル
		nullptr,//メニューハンドル
		w.hInstance,//呼び出しアプリケーションハンドル
		nullptr//追加パラメータ
		);


	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	
	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
	if (result != S_OK) {
		return -1;
	}

	//auto result =  CreateDXGIFactory(IID_PPV_ARGS(&_dxgiFactory));
	
	//アダプターの列挙用
	vector<IDXGIAdapter*> adapters;
	//ここに特定の名前を持つアダプターオブジェクトが入る
	IDXGIAdapter* tmpAdapter = nullptr;
	
	for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
		adapters.push_back(tmpAdapter);
	}

	for (auto adpt : adapters) {
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);
		wstring strDesc = adesc.Description;

		//探したいアダプターの名前を確認
		if (strDesc.find(L"Intel") != std::string::npos) {
			tmpAdapter = adpt;
			break;
		}
	}


	//Direct3Dデバイスの初期化
	D3D_FEATURE_LEVEL featureLevel;
	for (auto lv : levels) {
		if (D3D12CreateDevice(nullptr, lv, IID_PPV_ARGS(&_dev)) == S_OK) {
			featureLevel = lv;
			break;
		}
	}

	//ウィンドウ表示
	ShowWindow(hwnd, SW_SHOW);



#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif
	MSG msg = {};
	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//アプリケーション終了時にmsgにWM_QUITを渡す
		if (msg.message == WM_QUIT)
		{
			break;
		}
	}

	//もうクラスは使わないので登録解除する
	UnregisterClass(w.lpszClassName, w.hInstance);
	//DebugOutputFormatString("Show window test.");
	//getchar();
	return 0;
}
