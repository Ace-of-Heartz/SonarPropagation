#include "ImGuiManager.h"
#include "pch.h"


SonarPropagation::Graphics::Utils::ImGuiManager::ImGuiManager()
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	io.Fonts->AddFontDefault();

	ImGui::StyleColorsDark();
}

SonarPropagation::Graphics::Utils::ImGuiManager::~ImGuiManager()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplUwp_Shutdown();
	ImGui::DestroyContext();
}

void SonarPropagation::Graphics::Utils::ImGuiManager::InitImGui(
	int frameCount,
	DXGI_FORMAT backBufferFormat,
	ID3D12Device5* dxrDevice,
	ID3D12DescriptorHeap* srvHeap
) {

	// Setup Platform/Renderer backends
	ImGui_ImplUwp_InitForCurrentView();
	ImGui_ImplDX12_Init(
		dxrDevice,
		frameCount,
		backBufferFormat,
		srvHeap,
		srvHeap->GetCPUDescriptorHandleForHeapStart(),
		srvHeap->GetGPUDescriptorHandleForHeapStart());

}

void SonarPropagation::Graphics::Utils::ImGuiManager::BeginImGui(ID3D12GraphicsCommandList4* commandList) {

	PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, "ImGui");
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplUwp_NewFrame();
		ImGui::NewFrame();
		
	}

}

void SonarPropagation::Graphics::Utils::ImGuiManager::EndImGui(ID3D12GraphicsCommandList4* commandList) {

	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

	PIXEndEvent(commandList);
}




