#pragma once

#include "Common\StepTimer.h"
#include "Common\DeviceResources.h"

#include "DXR\RayTracingRenderer.h"

// Renders Direct3D content on the screen.
namespace SonarPropagation
{
	class SonarPropagationMain
	{
	public:
		SonarPropagationMain();
		void CreateRenderers(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void Update();
		bool Render();

		void OnWindowSizeChanged();
		void OnSuspending();
		void OnResuming();
		void OnDeviceRemoved();

		void OnKeyPressed(Windows::UI::Core::CoreWindow^ sender,Windows::UI::Core::KeyEventArgs^ args);
		void OnKeyReleased(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);
		void OnMouseMoved(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
		void OnMouseWheelMoved(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);

	private:
		// TODO: Replace with your own content renderers.
		std::unique_ptr<RayTracingRenderer> m_sceneRenderer;

		// Rendering loop timer.
		DX::StepTimer m_timer;
	};
}