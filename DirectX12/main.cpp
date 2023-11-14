#include <Windows.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include <d3dx12.h>
#ifdef _DEBUG
#include <iostream>
#endif

using namespace std;
using namespace DirectX;//DirectXMath���C�u�����̖��O���

// ���C�u�����̃����N�̐ݒ�
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"DirectXTex.lib")

#pragma pack(push, 1)

struct PMD_VERTEX {
	XMFLOAT3 pos;//���_���: 12�o�C�g
	XMFLOAT3 normal;//�@���x�N�g��: 12�o�C�g
	XMFLOAT2 uv;//uv���W: 8 �o�C�g
	uint16_t bone_no[2];//�{�[���ԍ� : 4�o�C�g
	uint8_t weight;//�{�[���̃E�F�C�g: 1�o�C�g
	uint8_t EdgeFlag;//�֊s���t���O: 1�o�C�g
};//�v38�o�C�g
#pragma pack(pop)

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


void EnableDebugLayer() {
	ID3D12Debug* debugLayer = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer)))) {
		debugLayer->EnableDebugLayer();
		debugLayer->Release();
	}
}

struct TexRGBA 
{
	unsigned char R, G, B, A;
};

std::vector<TexRGBA> texturedata(256 * 256);

#ifdef _DEBUG
int main() {
#else
#include <Windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif

	for (auto& rgba : texturedata)
	{
		rgba.R = rand() % 256;
		rgba.G = rand() % 256;
		rgba.B = rand() % 256;
		rgba.A = rand() % 256;
	}

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
		wrc.right - wrc.left,//�E�B���h�E��
		wrc.bottom - wrc.top,//�E�B���h�E����
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

	HRESULT result = S_OK;
	if (FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory)))) {
		if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&_dxgiFactory)))) {
			return -1;
		}
	}

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
	ID3D12DescriptorHeap* _rtvHeaps = nullptr;
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

	//SRGB�����_�[�^�[�Q�b�g�r���[�ݒ�
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < swpDesc.BufferCount; i++) { //�o�b�N�o�b�t�@�[�̐������ݒ肷��K�v������
		result = _swapChain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&_backBuffers[i]));
		//RTV����
		_dev->CreateRenderTargetView(_backBuffers[i], &rtvDesc, handle);
		//�|�C���^�[�����炷
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	//�[�x�o�b�t�@�쐬
	D3D12_RESOURCE_DESC depthResDesc = {};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;//2�����̃e�N�X�`���ŃI�^
	depthResDesc.Width = window_width;//���ƍ����̓����_�[�^�[�Q�b�g�Ɠ���
	depthResDesc.Height = window_height;//��ɓ���
	depthResDesc.DepthOrArraySize = 1;//�e�N�X�`���z��ł͂Ȃ��A3D�e�N�X�`���ł��Ȃ�
	depthResDesc.Format = DXGI_FORMAT_D32_FLOAT;//�[�x�l�������ݗp�t�H�[�}�b�g
	depthResDesc.SampleDesc.Count = 1;//�T���v����1�s�N�Z��������1��
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;//���̃o�b�t�@�̓f�v�X�X�e���V���Ƃ��Ďg�p���܂�
	depthResDesc.MipLevels = 1;
	depthResDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthResDesc.Alignment = 0;

	//�f�v�X�p�q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES depthHeapProp = {};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;//DEFAULT��������UNKNOWN�ł悵
	depthHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	//���̃N���A�o�����[���d�v�ȈӖ�������
	D3D12_CLEAR_VALUE _depthClearValue = {};
	_depthClearValue.DepthStencil.Depth = 1.0f;//�[���P(�ő�l)�ŃN���A
	_depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;//32bit�[�x�l�Ƃ��ăN���A

	//�o�b�t�@��Create
	ID3D12Resource* depthBuffer = nullptr;
	result = _dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, //�f�v�X�������݂Ɏg�p
		&_depthClearValue,
		IID_PPV_ARGS(&depthBuffer));
	if (result != S_OK) {
		return -1;
	}

	//�[�x�̂��߂̃f�B�X�N���v�^�q�[�v�쐬
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};//�[�x�Ɏg����Ƃ��������킩��΂���
	dsvHeapDesc.NumDescriptors = 1;//�[�x�r���[1�̂�
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;//�f�v�X�X�e���V���r���[�Ƃ��Ďg��
	ID3D12DescriptorHeap* dsvHeap = nullptr;
	result = _dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));
	if (result != S_OK) {
		return -1;
	}

	//�f�v�X�X�e���V���r���[�쐬
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;//�f�v�X�l��32bit�g�p
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;//2D�e�N�X�`��
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;//�t���O�͓��ɂȂ�
	_dev->CreateDepthStencilView(depthBuffer, &dsvDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());

	//�t�F���X�̐���
	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	//�E�B���h�E�\��
	ShowWindow(hwnd, SW_SHOW);

	// PMD�w�b�_�[�\����
	struct PMDHeader {
		float version; //�� : 00 00 80 3F == 1.00
		char model_name[20]; // ���f����
		char comment[256]; //���f���R�����g
	};
	char signature[3];
	PMDHeader pmdheader = {};
	FILE* fp;
	auto err = fopen_s(&fp, "Model/�����~�N.pmd", "rb");
	if (fp == nullptr) {
		return -1;
	}

	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdheader, sizeof(pmdheader), 1, fp);

#pragma pack(1)//�����o���A���C�����g����ăp�f�B���O�����܂邽��pack, �ǂݍ��݂�������
	struct PMD_Material
	{
		XMFLOAT3 diffuse;//�f�B�t���[�Y�F
		float alpha; //�f�B�t���[�Y�̃�
		float specularity; //�X�؃L�����̋���
		XMFLOAT3 specular; //�X�؃L�����F
		XMFLOAT3 ambient; //�A���r�G���g�F
		unsigned char toonIdx; //�g�D�[���F
		unsigned char edgeFlg; //�}�e���A�����Ƃ̗֊s���t���O
		unsigned int indicesNum; //���̃}�e���A�������蓖�Ă���C���f�b�N�X��
		char texFilePath[20]; //�e�N�X�`���t�@�C���p�X
	};
#pragma pack()

	struct MaterialForHlsl
	{
		XMFLOAT3 diffuse;//�f�B�t���[�Y�F
		float alpha; //�f�B�t���[�Y�̃�
		float specularity; //�X�؃L�����̋���
		XMFLOAT3 specular; //�X�؃L�����F
		XMFLOAT3 ambient; //�A���r�G���g�F
	};

	struct AdditionalMaterialData
	{
		std::string texFliePath;//�p�X
		int toonIdx;
		bool edgeFlag;
	};

	struct Material {
		unsigned int indicesNum;
		MaterialForHlsl material;
		AdditionalMaterialData additional;
	};

	unsigned int vertNum;//���_��
	fread(&vertNum, sizeof(vertNum), 1, fp);

	constexpr unsigned int pmdvertex_size = 38;//���_1������̃T�C�Y
	std::vector<unsigned char> vertices(vertNum * pmdvertex_size);//�o�b�t�@�m��
	fread(vertices.data(), vertices.size(), 1, fp);

	unsigned int indicesNum;//�C���f�b�N�X��
	fread(&indicesNum, sizeof(indicesNum), 1, fp);

	//CreateCommitResource()�p�̃q�[�v�\���̂̒�`
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	//���\�[�X�ݒ�̍\����
	auto resdesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size()*sizeof(PMD_VERTEX));

	//UPLOAD
	ID3D12Resource* vertBuff = nullptr;
	result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));
	if (result != S_OK) return -1;

	unsigned char* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);

	std::copy(vertices.begin(), vertices.end(), vertMap);

	vertBuff->Unmap(0, nullptr);

	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();//�o�b�t�@�̉��z�A�h���X
	vbView.SizeInBytes = vertices.size()*sizeof(PMD_VERTEX);//�S�o�C�g��
	vbView.StrideInBytes = pmdvertex_size;//1���_������̃o�C�g��

	std::vector<unsigned short> indices(indicesNum);

	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	//�}�e���A���ǂݍ���
	unsigned int materialNum;
	fread(&materialNum, sizeof(materialNum), 1, fp);
	std::vector<Material> materials(materialNum);

	std::vector<PMD_Material> pmdMaterials(materialNum);
	fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(PMD_Material), 1, fp);
	for (int i = 0; i < pmdMaterials.size(); ++i) {
		materials[i].indicesNum = pmdMaterials[i].indicesNum;
		materials[i].material.diffuse = pmdMaterials[i].diffuse;
		materials[i].material.alpha = pmdMaterials[i].alpha;
		materials[i].material.specular = pmdMaterials[i].specular;
		materials[i].material.specularity = pmdMaterials[i].specularity;
		materials[i].material.ambient = pmdMaterials[i].ambient;
	}
	//fp��Close
	fclose(fp);

	ID3D12Resource* idxBuff = nullptr;
	//resdesc.Width = sizeof(indices);
	heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	resdesc = CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0]));
	result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&idxBuff));

	//������o�b�t�@�ɃC���f�b�N�X�f�[�^���R�s�[
	unsigned short* mappedIdx = nullptr;
	idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(indices.begin(), indices.end(), mappedIdx);
	idxBuff->Unmap(0, nullptr);

	//�C���f�b�N�X�o�b�t�@�r���[���쐬
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = indices.size()*sizeof(indices[0]);

	//�}�e���A���o�b�t�@���쐬
	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize + 0xff) & ~0xff;
	heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	resdesc = CD3DX12_RESOURCE_DESC::Buffer(materialBuffSize * materialNum);
	ID3D12Resource* materialBuff = nullptr;
	result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&materialBuff)
	);

	//�}�b�v�}�e���A���ɃR�s�[
	char* mapMaterial = nullptr;
	result = materialBuff->Map(0, nullptr, (void**)&mapMaterial);
	for (auto& m : materials) {
		*((MaterialForHlsl*)mapMaterial) = m.material;//�f�[�^�R�s�[
		mapMaterial += materialBuffSize;//���̃A���C�����g�ʒu�܂Ői�߂�
	}
	materialBuff->Unmap(0, nullptr);

	ID3D12DescriptorHeap* materialDescHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC materialDescHeapDesc = {};
	materialDescHeapDesc.NumDescriptors = materialNum;
	materialDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	materialDescHeapDesc.NodeMask = 0;
	materialDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//�f�X�N���v�^�q�[�v���
	result = _dev->CreateDescriptorHeap(&materialDescHeapDesc, IID_PPV_ARGS(&materialDescHeap));//����

	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};
	matCBVDesc.BufferLocation = materialBuff->GetGPUVirtualAddress();
	matCBVDesc.SizeInBytes = materialBuffSize;

	auto matDescHeapH = materialDescHeap->GetCPUDescriptorHandleForHeapStart();
	for (size_t i = 0; i < materialNum; ++i) {
		//�}�e���A���Œ�o�b�t�@�r���[
		_dev->CreateConstantBufferView(&matCBVDesc, matDescHeapH);
		matDescHeapH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		matCBVDesc.BufferLocation += materialBuffSize;
	}


	//�V�F�[�_�̓ǂݍ��݂Ɛ���
	ID3DBlob* _vsBlob = nullptr;
	ID3DBlob* _psBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	//���_�V�F�[�_�[�̓ǂݍ���
	result = D3DCompileFromFile(L"BasicVertexShader.hlsl",
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicVS", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &_vsBlob, &errorBlob);
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("�t�@�C������������܂���");
		}
		else {
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		return -1;
	}
	//�s�N�Z���V�F�[�_�[�̓ǂݍ���
	result = D3DCompileFromFile(L"BasicPixelShader.hlsl",
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &_psBlob, &errorBlob);
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("�t�@�C������������܂���");
		}
		else {
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		return -1;
	}
	//���_���C�A�E�g(���_���ǂ����������w�肷��)�̃f�[�^
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"BONE_NO", 0, DXGI_FORMAT_R16G16_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"WEIGHT", 0, DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"EDGE_FLG", 0, DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};
	//�O���t�B�b�N�X�p�C�v���C���̐ݒ�
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	gpipeline.pRootSignature = nullptr;
	gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
	gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();
	//�T���v���}�X�N�̐ݒ�
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//���g��0xffffffff
	//���X�^���C�U�[�X�e�[�g�̐ݒ�
	gpipeline.RasterizerState.MultisampleEnable = false;//�܂��A���`�F���͎g��Ȃ�
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//�J�����O���Ȃ�
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;//���g��h��Ԃ�
	gpipeline.RasterizerState.DepthClipEnable = true;//�[�x�����̃N���b�s���O�͗L����
	// �u�����h�X�e�[�g�̐ݒ�
	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;
	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	renderTargetBlendDesc.BlendEnable = false;//�ЂƂ܂����Z���Z�⃿�u�����f�B���O�͎g�p���Ȃ�
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	renderTargetBlendDesc.LogicOpEnable = false;//�ЂƂ܂��_�����Z�͎g�p���Ȃ�
	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;
	//InputLayout�̐ݒ�
	gpipeline.InputLayout.pInputElementDescs = inputLayout;//���C�A�E�g�擪�A�h���X
	gpipeline.InputLayout.NumElements = _countof(inputLayout);//���C�A�E�g�z��
	//IBStripCutValue�̐ݒ�
	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//�X�g���b�v���̃J�b�g�Ȃ�
	// PrimitiveTopologyType�̐ݒ�
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//�O�p�`�ō\��
	// �����_�[�^�[�Q�b�g�̐ݒ�
	gpipeline.NumRenderTargets = 1;//���͂P�̂�
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0�`1�ɐ��K�����ꂽRGBA
	//�[�x�X�e���V��
	gpipeline.DepthStencilState.DepthEnable = true;//�[�x
	gpipeline.DepthStencilState.StencilEnable = false;//���Ƃ�
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;//�[�x�l�̌^�w��
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;//Z�e�X�g�ɂ����Đ[�x�l���������ق����̗p����
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//�s�N�Z���`�掞�ɐ[�x�o�b�t�@�[�ɐ[�x�l����������
	// �A���`�G�C���A�X�̐ݒ�
	gpipeline.SampleDesc.Count = 1;//�T���v�����O��1�s�N�Z���ɂ��P
	gpipeline.SampleDesc.Quality = 0;//�N�I���e�B�͍Œ�


	// ���[�g�V�O�l�`���[�̐ݒ�
	ID3D12RootSignature* rootsignature = nullptr;
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	//���[�g�p�����[�^�p��descirptorTable�����o�[�̐ݒ�
	D3D12_DESCRIPTOR_RANGE descTblRange[2] = {};
	//�萔�p���W�X�^�[0��
	descTblRange[0].NumDescriptors = 1;
	descTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descTblRange[0].BaseShaderRegister = 0;
	descTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	//�萔�p���W�X�^�[1��(�}�e���A���p)
	descTblRange[1].NumDescriptors = 1;
	descTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descTblRange[1].BaseShaderRegister = 1;
	descTblRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;


	//���[�g�p�����[�^(�f�B�X�N���v�^�e�[�u���̎���)
	//�f�B�X�N���v�^�e�[�u���́A�萔�o�b�t�@��e�N�X�`���Ȃǂ�CPU��GPU�ł���肷��ۂɃ��\�[�X�Ɋ��蓖�Ă郌�W�X�^�[��ʂƃ��W�X�^�[�ԍ��̎w��̃Z�b�g���܂Ƃ߂Ă������
	D3D12_ROOT_PARAMETER rootparam[2] = {};
	rootparam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//���ׂẴV�F�[�_���猩����
	rootparam[0].DescriptorTable.pDescriptorRanges = &descTblRange[0];//�f�B�X�N���v�^�����W�̃A�h���X
	rootparam[0].DescriptorTable.NumDescriptorRanges = 1;//�f�B�X�N���v�^�����W��

	rootparam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//���ׂẴV�F�[�_���猩����
	rootparam[1].DescriptorTable.pDescriptorRanges = &descTblRange[1];//�f�B�X�N���v�^�����W�̃A�h���X
	rootparam[1].DescriptorTable.NumDescriptorRanges = 1;//�f�B�X�N���v�^�����W��
	rootSignatureDesc.NumParameters = 2;
	rootSignatureDesc.pParameters = rootparam;
	
	//�T���v���[(uv�l�ɂ���ăe�N�X�`���f�[�^����ǂ��F�����o����)�̐ݒ�
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//���J��Ԃ�
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//�c�J��Ԃ�
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//���s�J��Ԃ�
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;//�{�[�_�[�̎��͍�
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;//��Ԃ��Ȃ�(�j�A���X�g�l�C�o�[)
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;//�~�b�v�}�b�v�ő�l
	samplerDesc.MinLOD = 0.0f;//�~�b�v�}�b�v�ŏ��l
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;//�I�[�o�[�T���v�����O�̍ۃ��T���v�����O���Ȃ��H
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//�s�N�Z���V�F�[�_����̂݉�

	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 1;


	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);
	if (result != S_OK) {
		return -1;
	}
	result = _dev->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootsignature));
	if (result != S_OK) {
		return -1;
	}
	rootSigBlob->Release();

	// �O���t�B�b�N�X�p�C�v���C���X�e�[�g�ɐݒ�
	gpipeline.pRootSignature = rootsignature;

	//�p�C�v���C���X�e�[�g�̍쐬
	ID3D12PipelineState* _pipelinestate = nullptr;
	result = _dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&_pipelinestate));
	if (result != S_OK) {
		return -1;
	}

	//�r���[�|�[�g(��ʂɑ΂��ă����_�����O���ʂ��ǂ̂悤�ɕ\�����邩)�̐ݒ�
	D3D12_VIEWPORT viewport = {};
	viewport.Width = window_width;
	viewport.Height = window_height;
	viewport.TopLeftX = 0;//�o�͐�̍�����WX
	viewport.TopLeftY = 0;//�o�͐�̉E����WY
	viewport.MaxDepth = 1.0f;
	viewport.MinDepth = 0.0f;

	//�V�U�[�Z�`(�r���[�|�[�g�ɏo�͂��ꂽ�摜�̂ǂ�����ǂ��܂ł����ۂɉ�ʂɉf���o����)�̐ݒ�
	D3D12_RECT scissorrect = {};
	scissorrect.top = 0;
	scissorrect.left = 0;
	scissorrect.right = scissorrect.left + window_width;
	scissorrect.bottom = scissorrect.bottom + window_height;

	//�V�F�[�_���ɓn�����߂̊�{�I�ȍs��f�[�^
	struct MatricesData {
		XMMATRIX world;//���f���{�̂���]���������C�ړ��������肷��s��
		XMMATRIX viewproj;//�r���[�ƃv���W�F�N�V���������s��
	};

	//�萔�o�b�t�@�쐬
	XMMATRIX worldMat = XMMatrixIdentity();
	XMFLOAT3 eye(0, 10, -15);//���_�x�N�g��
	XMFLOAT3 target(0, 10, 0);//�����_�x�N�g��
	XMFLOAT3 up(0, 1, 0);//��x�N�g��
	auto viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	auto projMat = XMMatrixPerspectiveFovLH(
		XM_PIDIV2,//��p��90��
		static_cast<float>(window_height) / static_cast<float>(window_height),//�A�X�y�N�g��
		1.0f,//�߂��N���b�s���O�ʂ܂ł̋���
		100.0f//�����N���b�s���O�ʂ܂ł̋���
	);

	ID3D12Resource* constBuff = nullptr;
	heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	resdesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(MatricesData) + 0xff) & ~0xff);
	result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuff)
	);
	if (result != S_OK) {
		return -1;
	}

	//�}�b�v�ɂ��萔�̃R�s�[
	MatricesData* mapMatrix;
	result = constBuff->Map(0, nullptr, (void**)&mapMatrix);
	mapMatrix->world = worldMat;
	mapMatrix->viewproj = viewMat * projMat;

	//��{�I�ȏ��̎󂯓n���p�f�B�X�N���v�^�q�[�v�̍쐬
	ID3D12DescriptorHeap* basicDescHeap = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//�V�F�[�_�[���猩����悤��
	descHeapDesc.NodeMask = 0;//�}�X�N��0
	descHeapDesc.NumDescriptors = 2;//SRV, CBV�̌v2��
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//�f�B�X�N���v�^�q�[�v���
	result = _dev->CreateDescriptorHeap(
		&descHeapDesc,
		IID_PPV_ARGS(&basicDescHeap)
	);
	if (result != S_OK) {
		return -1;
	}

	//�f�B�X�N���v�^�̐擪�n���h�����l�����Ă���
	auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = static_cast<UINT>(constBuff->GetDesc().Width);

	_dev->CreateConstantBufferView(
		&cbvDesc,//�ݒ肵���e�N�X�`���ݒ���
		basicHeapHandle
	);

	MSG msg = {};
	unsigned int frame = 0;
	float angle = 0.0f;
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
		
		
		worldMat = XMMatrixRotationY(angle);
		//std::cout << "angle" << angle << std::endl;
		//std::cout << "worldMat" << worldMat.r << std::endl;
		mapMatrix->world = worldMat;
		mapMatrix->viewproj = viewMat * projMat;
		angle += 0.005f;
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
		auto dsvH = dsvHeap->GetCPUDescriptorHandleForHeapStart();
		_cmdList->OMSetRenderTargets(//�����_�[�^�[�Q�b�g���w��
			1,//�����_�[�^�[�Q�b�g�� �����ł�1
			&rtvH, //�����_�[�^�[�Q�b�g�n���h���̐擪�A�h���X
			false , //�������ɘA�����Ă��邩
			&dsvH //�[�x�X�e���V���o�b�t�@�[�r���[�̃n���h��
		);

		//��ʂ̃N���A
		float r, g, b;
		//bit�V�t�g��r,g,b�����𒊏o���Đ��K��
		//r = (float)(0xff & frame >> 16) / 255.0f;
		//g = (float)(0xff & frame >> 8) / 255.0f;
		//b = (float)(0xff & frame >> 0) / 255.0f;
		float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };//��
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);//��ʂ̃N���A
		_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);//Depth�̃r���[�̃N���A
		frame++;
		//�p�C�v���C���X�e�[�g�̃Z�b�g
		_cmdList->SetPipelineState(_pipelinestate);
		//�r���[�|�[�g�̃Z�b�g
		_cmdList->RSSetViewports(1, &viewport);
		//�V�U�[�Z�`�̃Z�b�g
		_cmdList->RSSetScissorRects(1, &scissorrect);
		//�v���~�e�B�u�g�|���W(���_���ǂ̂悤�ɑg�ݍ��킹�ē_�E���E�|���S�����\������̂�)�̃Z�b�g
		_cmdList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//���_���̃Z�b�g
		_cmdList->IASetVertexBuffers(0, 1, &vbView);
		_cmdList->IASetIndexBuffer(&ibView);
		//���[�g�V�O�l�`���̃Z�b�g
		_cmdList->SetGraphicsRootSignature(rootsignature);
		//�f�B�X�N���v�^�q�[�v�̎w��
		_cmdList->SetDescriptorHeaps(1, &basicDescHeap);
		//���[�g�p�����[�^�ƃf�B�X�N���v�^�q�[�v�̊֘A�Â�
		_cmdList->SetGraphicsRootDescriptorTable(0, basicDescHeap->GetGPUDescriptorHandleForHeapStart());
		//_cmdList->DrawIndexedInstanced(indicesNum, 1, 0, 0, 0);

		//�}�e���A��
		auto materialH = materialDescHeap->GetGPUDescriptorHandleForHeapStart();
		unsigned int idxOffset = 0;

		//auto cbvsrvIncSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
		for (auto& m : materials) {
			_cmdList->SetGraphicsRootDescriptorTable(1, materialH);
			_cmdList->DrawIndexedInstanced(m.indicesNum, 1, idxOffset, 0, 0);
			materialH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			idxOffset += m.indicesNum;
		}

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
