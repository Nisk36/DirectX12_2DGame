#include <Windows.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <d3d12.h>
#include <dxgi1_6.h>
#ifdef _DEBUG
#include <iostream>
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
ID3D12CommandAllocator* _cmdAllocator = nullptr;
ID3D12GraphicsCommandList* _cmdList = nullptr;
ID3D12CommandQueue* _cmdQueue = nullptr;
ID3D12DescriptorHeap* _rtvHeaps = nullptr;
ID3D12Fence* _fence = nullptr;

void EnableDebugLayer() {
	ID3D12Debug* debugLayer = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer)))) {
		debugLayer->EnableDebugLayer();
		debugLayer->Release();
	}
}

#ifdef _DEBUG
int main() {
#else
#include <Windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif
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
#ifdef _DEBUG
	//デバッグレイヤーをEnable
	EnableDebugLayer();
#endif

	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

#ifdef _DEBUG
	auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory));
	if (result != S_OK) return -1;
#else
	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
	if (result != S_OK) {
		return -1;
	}
#endif
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
	
	//CommandAllocatorの生成
	result = _dev->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&_cmdAllocator)
	);
	if (result != S_OK) return -1;

	//CommandListの生成
	result = _dev->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		_cmdAllocator,
		nullptr,
		IID_PPV_ARGS(&_cmdList)
	);
	if (result != S_OK) return -1;

	//CommandQueueの生成
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	//D3D12_COMMAND_QUEUE_DESC構造体で設定を行う
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;//タイムアウトなし
	cmdQueueDesc.NodeMask = 0;//アダプターを1つしか用いないときは0でよい
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;//Priorityは特に指定なし
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;//CommandListと合わせる
	result = _dev->CreateCommandQueue(
		&cmdQueueDesc,
		IID_PPV_ARGS(&_cmdQueue)
	);
	if (result != S_OK) return -1;

	//SwapChain作成
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = window_width;//画面解像度(幅)
	swapChainDesc.Height = window_height;//画面解像度(高)
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//ピクセルフォーマット
	swapChainDesc.Stereo = false;//ステレオ表示フラグ(3Dディスプレイのステレオモード)
	swapChainDesc.SampleDesc.Count = 1;//マルチサンプルの指定
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapChainDesc.BufferCount = 2;//ダブルバッファーなら2でOK
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;//バックバッファは伸び縮み可能
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;//フリップ後は速やかに破棄
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;//特に指定なし
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;//ウィンドウとフルスクリーン切り替え可能
	result = _dxgiFactory->CreateSwapChainForHwnd(
		_cmdQueue,
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)&_swapChain
	);
	if (result != S_OK) return -1;

	//DescriptorHeap作成
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;//どんなビューを作るのか指定、ここではレンダーターゲットビュー(RTV)
	heapDesc.NodeMask = 0;//GPUは1つであるため0でよい
	heapDesc.NumDescriptors = 2;//ダブルバッファリングの際は表裏2つあるため2
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;//ビューに当たる情報をシェーダー側から参照する必要があるかどうか、ここでは必要ないためNONE
	result = _dev->CreateDescriptorHeap(
		&heapDesc,
		IID_PPV_ARGS(&_rtvHeaps)
	);
	if (result != S_OK) return -1;

	//DescriptorとSwapChain上のバッファーの関連付け
	DXGI_SWAP_CHAIN_DESC swpDesc = {};
	result = _swapChain->GetDesc(&swpDesc);
	if (result != S_OK) return -1;
	vector<ID3D12Resource*> _backBuffers(swpDesc.BufferCount);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = _rtvHeaps->GetCPUDescriptorHandleForHeapStart(); //先頭のアドレスを得る
	for (int i = 0; i < swpDesc.BufferCount; i++) { //バックバッファーの数だけ設定する必要がある
		//RTV生成
		_dev->CreateRenderTargetView(_backBuffers[i], nullptr, handle);
		//ポインターをずらす
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	//フェンスの生成
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	
	//ウィンドウ表示
	ShowWindow(hwnd, SW_SHOW);

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

		//DirectX処理
		auto bbIdx = _swapChain->GetCurrentBackBufferIndex();//現在のバックバッファーを指すインデックスを取得

		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		BarrierDesc.Transition.pResource = _backBuffers[bbIdx];
		BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		_cmdList->ResourceBarrier(1, &BarrierDesc);

		auto rtvH = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		_cmdList->OMSetRenderTargets(//レンダーターゲットを指定
			1,//レンダーターゲット数 ここでは1
			&rtvH, //レンダーターゲットハンドルの先頭アドレス
			true, //複数時に連続しているか
			nullptr //震度ステンシルバッファービューのハンドル
		);
		float clearColor[] = {1.0f, 1.0f, 0.0f, 1.0f };
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);//画面のクリア

		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		_cmdList->ResourceBarrier(1, &BarrierDesc);

		_cmdList->Close();//命令実行前はCloseが必須, 絶対に忘れないように
		ID3D12CommandList* cmdLists[] = { _cmdList };
		_cmdQueue->ExecuteCommandLists(1, cmdLists);//コマンドリストの実行

		_cmdQueue->Signal(_fence, ++_fenceVal);//待ち
		if (_fence->GetCompletedValue() != _fenceVal) {
			auto event = CreateEvent(nullptr, false, false, nullptr);//eventハンドルの取得
			_fence->SetEventOnCompletion(_fenceVal, event);//フェンス値が_fenceValになったらeventを発生させる
			WaitForSingleObject(event, INFINITE);//イベントが発生するまで待ち
			CloseHandle(event);//イベントハンドルを閉じる
		}

		_cmdAllocator->Reset();//キューをクリア
		_cmdList->Reset(_cmdAllocator, nullptr);//再びコマンドリストをためる準備
		_swapChain->Present(1, 0);//フリップ
	}

	//もうクラスは使わないので登録解除する
	UnregisterClass(w.lpszClassName, w.hInstance);
	//DebugOutputFormatString("Show window test.");
	//getchar();
	return 0;
}
