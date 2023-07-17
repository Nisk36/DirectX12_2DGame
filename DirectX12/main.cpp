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

// ���C�u�����̃����N�̐ݒ�
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")



//@brief �R���\�[����ʂŃt�H�[�}�b�g�t���������\��
//@param format�t�H�[�}�b�g(%d, %f�Ƃ�)
//@param �ϒ�����
//@remarks ���̊֐��̓f�o�b�O�p�ł��B�f�o�b�O���ɂ������삵�Ȃ�
void DebugOutputFormatString(const char* format, ...) 
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);
#endif
}

//�E�B���h�E�v���V�[�W��
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	//�E�B���h�E���j�����ꂽ��Ă΂��
	if (msg == WM_DESTROY) {
		PostQuitMessage(0);//OS�ɑ΂��āu���̃A�v�����I���v�Ɠ`����
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

const unsigned int window_width = 1280;
const unsigned int window_height = 720;

// ��{�I�u�W�F�N�g�̎󂯎M�ƂȂ�ϐ�
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
	//�E�B���h�E�N���X�̐���&�o�^
	WNDCLASSEX w = {};

	w.cbSize = sizeof(WNDCLASSEX);
	//�R�[���o�b�N�֐��̎w��
	w.lpfnWndProc = (WNDPROC)WindowProcedure;
	//�A�v���P�[�V�����N���X��
	w.lpszClassName = _T("DX12Sample");
	//�n���h���̎擾
	w.hInstance = GetModuleHandle(nullptr);
	
	//�A�v���P�[�V�����N���X
	RegisterClassEx(&w);
	//�E�B���h�E�T�C�Y�ݒ�
	RECT wrc = { 0, 0, window_width, window_height };
	//�֐���p���ăE�B���h�E�̃T�C�Y�␳
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
	//�E�B���h�E�I�u�W�F�N�g�̐���
	HWND hwnd = CreateWindow(w.lpszClassName,//�N���X���w��
		_T("DX12�e�X�g"),//�^�C�g���o�[����
		WS_OVERLAPPEDWINDOW,//�^�C�g���o�[�Ƌ��E��������E�B���h�E
		CW_USEDEFAULT,//�\��x���W��OS�ɔC����
		CW_USEDEFAULT,//�\��y���W��
		wrc.right-wrc.left,//�E�B���h�E��
		wrc.bottom-wrc.top,//�E�B���h�E����
		nullptr,//�e�E�B���h�E�n���h��
		nullptr,//���j���[�n���h��
		w.hInstance,//�Ăяo���A�v���P�[�V�����n���h��
		nullptr//�ǉ��p�����[�^
		);
#ifdef _DEBUG
	//�f�o�b�O���C���[��Enable
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
	//�A�_�v�^�[�̗񋓗p
	vector<IDXGIAdapter*> adapters;
	//�����ɓ���̖��O�����A�_�v�^�[�I�u�W�F�N�g������
	IDXGIAdapter* tmpAdapter = nullptr;
	
	for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
		adapters.push_back(tmpAdapter);
	}

	for (auto adpt : adapters) {
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);
		wstring strDesc = adesc.Description;

		//�T�������A�_�v�^�[�̖��O���m�F
		if (strDesc.find(L"Intel") != std::string::npos) {
			tmpAdapter = adpt;
			break;
		}
	}


	//Direct3D�f�o�C�X�̏�����
	D3D_FEATURE_LEVEL featureLevel;
	for (auto lv : levels) {
		if (D3D12CreateDevice(nullptr, lv, IID_PPV_ARGS(&_dev)) == S_OK) {
			featureLevel = lv;
			break;
		}
	}
	
	//CommandAllocator�̐���
	result = _dev->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&_cmdAllocator)
	);
	if (result != S_OK) return -1;

	//CommandList�̐���
	result = _dev->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		_cmdAllocator,
		nullptr,
		IID_PPV_ARGS(&_cmdList)
	);
	if (result != S_OK) return -1;

	//CommandQueue�̐���
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	//D3D12_COMMAND_QUEUE_DESC�\���̂Őݒ���s��
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;//�^�C���A�E�g�Ȃ�
	cmdQueueDesc.NodeMask = 0;//�A�_�v�^�[��1�����p���Ȃ��Ƃ���0�ł悢
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;//Priority�͓��Ɏw��Ȃ�
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;//CommandList�ƍ��킹��
	result = _dev->CreateCommandQueue(
		&cmdQueueDesc,
		IID_PPV_ARGS(&_cmdQueue)
	);
	if (result != S_OK) return -1;

	//SwapChain�쐬
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = window_width;//��ʉ𑜓x(��)
	swapChainDesc.Height = window_height;//��ʉ𑜓x(��)
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//�s�N�Z���t�H�[�}�b�g
	swapChainDesc.Stereo = false;//�X�e���I�\���t���O(3D�f�B�X�v���C�̃X�e���I���[�h)
	swapChainDesc.SampleDesc.Count = 1;//�}���`�T���v���̎w��
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapChainDesc.BufferCount = 2;//�_�u���o�b�t�@�[�Ȃ�2��OK
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;//�o�b�N�o�b�t�@�͐L�яk�݉\
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;//�t���b�v��͑��₩�ɔj��
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;//���Ɏw��Ȃ�
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;//�E�B���h�E�ƃt���X�N���[���؂�ւ��\
	result = _dxgiFactory->CreateSwapChainForHwnd(
		_cmdQueue,
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)&_swapChain
	);
	if (result != S_OK) return -1;

	//DescriptorHeap�쐬
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;//�ǂ�ȃr���[�����̂��w��A�����ł̓����_�[�^�[�Q�b�g�r���[(RTV)
	heapDesc.NodeMask = 0;//GPU��1�ł��邽��0�ł悢
	heapDesc.NumDescriptors = 2;//�_�u���o�b�t�@�����O�̍ۂ͕\��2���邽��2
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;//�r���[�ɓ���������V�F�[�_�[������Q�Ƃ���K�v�����邩�ǂ����A�����ł͕K�v�Ȃ�����NONE
	result = _dev->CreateDescriptorHeap(
		&heapDesc,
		IID_PPV_ARGS(&_rtvHeaps)
	);
	if (result != S_OK) return -1;

	//Descriptor��SwapChain��̃o�b�t�@�[�̊֘A�t��
	DXGI_SWAP_CHAIN_DESC swpDesc = {};
	result = _swapChain->GetDesc(&swpDesc);
	if (result != S_OK) return -1;
	vector<ID3D12Resource*> _backBuffers(swpDesc.BufferCount);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = _rtvHeaps->GetCPUDescriptorHandleForHeapStart(); //�擪�̃A�h���X�𓾂�
	for (int i = 0; i < swpDesc.BufferCount; i++) { //�o�b�N�o�b�t�@�[�̐������ݒ肷��K�v������
		//RTV����
		_dev->CreateRenderTargetView(_backBuffers[i], nullptr, handle);
		//�|�C���^�[�����炷
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	//�t�F���X�̐���
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	
	//�E�B���h�E�\��
	ShowWindow(hwnd, SW_SHOW);

	MSG msg = {};
	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//�A�v���P�[�V�����I������msg��WM_QUIT��n��
		if (msg.message == WM_QUIT)
		{
			break;
		}

		//DirectX����
		auto bbIdx = _swapChain->GetCurrentBackBufferIndex();//���݂̃o�b�N�o�b�t�@�[���w���C���f�b�N�X���擾

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
		_cmdList->OMSetRenderTargets(//�����_�[�^�[�Q�b�g���w��
			1,//�����_�[�^�[�Q�b�g�� �����ł�1
			&rtvH, //�����_�[�^�[�Q�b�g�n���h���̐擪�A�h���X
			true, //�������ɘA�����Ă��邩
			nullptr //�k�x�X�e���V���o�b�t�@�[�r���[�̃n���h��
		);
		float clearColor[] = {1.0f, 1.0f, 0.0f, 1.0f };
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);//��ʂ̃N���A

		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		_cmdList->ResourceBarrier(1, &BarrierDesc);

		_cmdList->Close();//���ߎ��s�O��Close���K�{, ��΂ɖY��Ȃ��悤��
		ID3D12CommandList* cmdLists[] = { _cmdList };
		_cmdQueue->ExecuteCommandLists(1, cmdLists);//�R�}���h���X�g�̎��s

		_cmdQueue->Signal(_fence, ++_fenceVal);//�҂�
		if (_fence->GetCompletedValue() != _fenceVal) {
			auto event = CreateEvent(nullptr, false, false, nullptr);//event�n���h���̎擾
			_fence->SetEventOnCompletion(_fenceVal, event);//�t�F���X�l��_fenceVal�ɂȂ�����event�𔭐�������
			WaitForSingleObject(event, INFINITE);//�C�x���g����������܂ő҂�
			CloseHandle(event);//�C�x���g�n���h�������
		}

		_cmdAllocator->Reset();//�L���[���N���A
		_cmdList->Reset(_cmdAllocator, nullptr);//�ĂуR�}���h���X�g�����߂鏀��
		_swapChain->Present(1, 0);//�t���b�v
	}

	//�����N���X�͎g��Ȃ��̂œo�^��������
	UnregisterClass(w.lpszClassName, w.hInstance);
	//DebugOutputFormatString("Show window test.");
	//getchar();
	return 0;
}
