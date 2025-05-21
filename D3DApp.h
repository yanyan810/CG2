#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

/// <summary>
/// DirectX 12 の基本的な初期化・描画・終了処理をまとめたクラス
/// - デバイスとスワップチェーンの作成
/// - コマンドリストとレンダーターゲットの制御
/// - バックバッファの切り替え・GPU同期
/// </summary>
class D3DApp {
public:
    /// <summary>
    /// 初期化関数。デバイス・スワップチェーン・RTVなどをセットアップします。
    /// </summary>
    /// <param name="hwnd">ウィンドウハンドル</param>
    /// <param name="width">クライアント領域の幅</param>
    /// <param name="height">クライアント領域の高さ</param>
    void Initialize(HWND hwnd, int width, int height);

    /// <summary>
    /// 描画の開始処理。リソースバリアをRenderTarget状態にします。
    /// </summary>
    void BeginFrame();

    /// <summary>
    /// 描画の終了処理。Present状態に戻し、コマンドを実行して画面に表示します。
    /// </summary>
    void EndFrame();

    /// <summary>
    /// GPUが前のフレームの処理を完了するまで待機します。
    /// </summary>
    void WaitGPU();

    /// <summary>
    /// リソースの解放処理。終了時に呼び出します。
    /// </summary>
    void Finalize();

    /// <summary>
    /// Direct3Dデバイスを取得します。
    /// </summary>
    ID3D12Device* GetDevice() const { return device_.Get(); }

    /// <summary>
    /// コマンドリストを取得します。
    /// </summary>
    ID3D12GraphicsCommandList* GetCommandList() const { return commandList_.Get(); }

    /// <summary>
    /// 現在のバックバッファインデックスを取得します。
    /// </summary>
    UINT GetBackBufferIndex() const { return backBufferIndex_; }

    /// <summary>
    /// 現在のRTVハンドルを取得します。
    /// </summary>
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTV() const;

private:
    void CreateDeviceAndSwapChain(HWND hwnd);
    void CreateRenderTargets();

    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_;

    HANDLE fenceEvent_ = nullptr;
    UINT64 fenceValue_ = 0;
    UINT backBufferIndex_ = 0;
    UINT rtvDescriptorSize_ = 0;

    // バックバッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> renderTargets_[2]{};
};
