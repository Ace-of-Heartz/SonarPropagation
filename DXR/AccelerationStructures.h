#pragma once 

namespace SonarPropagation {
	namespace Graphics {
		namespace DXR {
			/// <summary>
			/// Struct to hold the Acceleration Structure Buffers.
			/// </summary>
			struct AccelerationStructureBuffers {
				ComPtr<ID3D12Resource> pScratch;
				ComPtr<ID3D12Resource> pResult;
				ComPtr<ID3D12Resource> pInstanceDesc; // Used only for top-level AS
			};
		}
	}
}