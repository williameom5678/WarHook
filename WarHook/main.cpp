#include "main.h"
#include <D3Dcompiler.h>



uintptr_t modulebase = (uintptr_t)GetModuleHandle(NULL);
std::vector<uintptr_t> offsets = GetOffsets(signatures);

/*
offsets[0] = cGame
offsets[1] = LocalPlayer
offsets[2] = HudInfo
offsets[3] = ScreenWidth
offsets[4] = IsScoping
offsets[5] = ViewMatrix
*/

uintptr_t cGame = *(uintptr_t*)(modulebase + offsets[0]);
int scrW = *(int*)(modulebase + offsets[3]);
int scrH = *(int*)(modulebase + offsets[3] + 0x4);
const uintptr_t mat_addr = (uintptr_t)(modulebase + offsets[5]);

Vector2 scrsize = { (float)scrW,(float)scrH };

typedef void(__stdcall* D3D11DrawIndexedHook) (ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);
typedef HRESULT(__stdcall* D3D11ResizeBuffersHook) (IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
typedef void(__stdcall* D3D11DrawIndexedInstancedHook) (ID3D11DeviceContext* pContext, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation);


D3D11DrawIndexedHook phookD3D11DrawIndexed = NULL;
D3D11ResizeBuffersHook phookD3D11ResizeBuffers = NULL;
D3D11DrawIndexedInstancedHook phookD3D11DrawIndexedInstanced = NULL;


ID3D11DepthStencilState* g_depthEnabled;
ID3D11DepthStencilState* g_depthDisabled;

Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;
ImFont* def_main;
ImFont* med_main;
ImFont* big_main;

HRESULT __stdcall hookD3D11ResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	if (mainRenderTargetView) {
		pContext->OMSetRenderTargets(0, 0, 0);
		mainRenderTargetView->Release();
	}

	HRESULT hr = phookD3D11ResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

	ID3D11Texture2D* pBuffer;
	pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBuffer);

	pDevice->CreateRenderTargetView(pBuffer, NULL, &mainRenderTargetView);

	pBuffer->Release();

	pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);

	D3D11_VIEWPORT vp;
	vp.Width = Width;
	vp.Height = Height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;

	pContext->RSSetViewports(1, &vp);

	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)Width, (float)Height);
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
	ImGui::Render();

	scrsize = { (float)Width, (float)Height };
	
	return hr;
}


void __stdcall hookD3D11DrawIndexed(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
	pContext->IAGetVertexBuffers(0, 1, &veBuffer, &Stride, &veBufferOffset);
	if (veBuffer != NULL)
		veBuffer->GetDesc(&vedesc);
	if (veBuffer != NULL) { veBuffer->Release(); veBuffer = NULL; }

	
	pContext->IAGetIndexBuffer(&inBuffer, &inFormat, &inOffset);
	if (inBuffer != NULL)
		inBuffer->GetDesc(&indesc);
	if (inBuffer != NULL) { inBuffer->Release(); inBuffer = NULL; }

	pContext->PSGetConstantBuffers(pscStartSlot, 1, &pscBuffer);
	if (pscBuffer != NULL)
		pscBuffer->GetDesc(&pscdesc);
	if (pscBuffer != NULL) { pscBuffer->Release(); pscBuffer = NULL; }

	
	pContext->VSGetConstantBuffers(vscStartSlot, 1, &vscBuffer);
	if (vscBuffer != NULL)
		vscBuffer->GetDesc(&vscdesc);
	if (vscBuffer != NULL) { vscBuffer->Release(); vscBuffer = NULL; }
	
	/*if (GetAsyncKeyState(VK_NUMPAD1) & 1)
	{
		nstride += 1;
		printf("Stride: %d \n", nstride);
	}
	if (GetAsyncKeyState(VK_NUMPAD4) & 1)
	{
		nstride -= 1;
		printf("Stride: %d \n", nstride);
	}
	if (GetAsyncKeyState(VK_NUMPAD2) & 1)
	{
		count += 1;
		printf("Count: %d \n", count);
	}
	if (GetAsyncKeyState(VK_NUMPAD5) & 1)
	{
		count -= 1;
		printf("Count: %d \n", count);
	}
	*/
	if (remove_nature == true || remove_smokes == true)
	{
		if (remove_nature)
		{
			if (Stride == 12 || IndexCount == 0 || IndexCount == 7 || IndexCount == 13 || IndexCount == 21 || IndexCount == 19 || IndexCount == 44 || IndexCount == 53)
			{
				pContext->RSSetState(DEPTHBIASState_FALSE); 

				phookD3D11DrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation); 

				pContext->RSSetState(DEPTHBIASState_TRUE);
			}
		}
		if (remove_smokes)
		{
			if (Stride == 0 && IndexCount == 6)
			{
				pContext->RSSetState(DEPTHBIASState_FALSE); 

				phookD3D11DrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation); 

				pContext->RSSetState(DEPTHBIASState_TRUE);
			}
		}

	}
	if (remove_nature)
	{
		if (Stride == 12 || IndexCount == 0 || IndexCount == 7 || IndexCount == 13 || IndexCount == 21 || IndexCount == 19 || IndexCount == 44 || IndexCount == 53)
			return;
		
	}
	if (remove_smokes)
	{
		if (Stride == 0 && IndexCount == 6)
			return;
	}

	return phookD3D11DrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);

}

void __stdcall hookD3D11DrawIndexedInstanced(ID3D11DeviceContext* pContext, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation)
{


	
	pContext->IAGetVertexBuffers(0, 1, &veBuffer, &Stride, &veBufferOffset);
	if (veBuffer != NULL)
		veBuffer->GetDesc(&vedesc);
	if (veBuffer != NULL) { veBuffer->Release(); veBuffer = NULL; }

	
	pContext->IAGetIndexBuffer(&inBuffer, &inFormat, &inOffset);
	if (inBuffer != NULL)
		inBuffer->GetDesc(&indesc);
	if (inBuffer != NULL) { inBuffer->Release(); inBuffer = NULL; }

	
	pContext->PSGetConstantBuffers(pscStartSlot, 1, &pscBuffer);
	if (pscBuffer != NULL)
		pscBuffer->GetDesc(&pscdesc);
	if (pscBuffer != NULL) { pscBuffer->Release(); pscBuffer = NULL; }

	
	pContext->VSGetConstantBuffers(vscStartSlot, 1, &vscBuffer);
	if (vscBuffer != NULL)
		vscBuffer->GetDesc(&vscdesc);
	if (vscBuffer != NULL) { vscBuffer->Release(); vscBuffer = NULL; }

	if (remove_nature == true || remove_smokes == true)
	{
		if (remove_nature) {
			if (Stride == 12 || IndexCountPerInstance == 0 || IndexCountPerInstance == 7 || IndexCountPerInstance == 13 || IndexCountPerInstance == 21 || IndexCountPerInstance == 19 || IndexCountPerInstance == 44 || IndexCountPerInstance == 53)
			{
				pContext->RSSetState(DEPTHBIASState_FALSE); //depthbias off

				phookD3D11DrawIndexedInstanced(pContext, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation); //redraw

				pContext->RSSetState(DEPTHBIASState_TRUE); //depthbias true
			}
		}
		if (remove_smokes)
		{
			if (Stride == 0 && IndexCountPerInstance == 6)
			{
				pContext->RSSetState(DEPTHBIASState_FALSE); //depthbias off

				phookD3D11DrawIndexedInstanced(pContext, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation); //redraw

				pContext->RSSetState(DEPTHBIASState_TRUE); //depthbias true
			}
		}
		
	
	}
	
	
	

	if (remove_nature == true)
	{
		if (Stride == 12 || IndexCountPerInstance == 0 || IndexCountPerInstance == 7 || IndexCountPerInstance == 13 || IndexCountPerInstance == 21 || IndexCountPerInstance == 19 || IndexCountPerInstance == 44 || IndexCountPerInstance == 53)
			return; 
	}	
	if (remove_smokes == true)
	{
		if (Stride == 0 && IndexCountPerInstance == 6)
			return; 
	}

	return phookD3D11DrawIndexedInstanced(pContext, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

void SetupImGuiStyle()
{
	ImGui::GetStyle().FrameRounding = 6.f;
	ImGui::GetStyle().WindowBorderSize = 0.f;
	ImGui::GetStyle().ChildBorderSize = 0.f;
	ImGui::GetStyle().WindowTitleAlign.x = 0.5f;
	ImGui::GetStyle().WindowTitleAlign.y = 0.4f;
	ImGui::GetStyle().TabRounding = 0.f;
	ImGui::GetStyle().WindowPadding.x = 0.f;


	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.07f, 0.11f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.09f, 0.07f, 0.11f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.09f, 0.07f, 0.11f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.17f, 0.14f, 0.22f, 0.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.17f, 0.14f, 0.22f, 0.72f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.17f, 0.14f, 0.22f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.31f, 0.15f, 0.31f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.31f, 0.15f, 0.31f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.31f, 0.15f, 0.31f, 1.00f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.49f, 0.10f, 0.51f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.49f, 0.10f, 0.51f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.79f, 0.15f, 0.81f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.49f, 0.10f, 0.51f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.79f, 0.15f, 0.81f, 0.79f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.79f, 0.15f, 0.81f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.02f);
	colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.00f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 0.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_Tab] = ImVec4(0.49f, 0.10f, 0.51f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.79f, 0.15f, 0.81f, 0.78f);
	colors[ImGuiCol_TabActive] = ImVec4(0.79f, 0.15f, 0.81f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
	colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 0.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

}



float Distance(Vector3 target, Vector3 localplayer)
{
	float distance = std::sqrtf((target.x - localplayer.x) * (target.x - localplayer.x) +
		(target.y - localplayer.y) * (target.y - localplayer.y) +
		(target.z - localplayer.z) * (target.z - localplayer.z));
	return distance;
}

float Distance(Vector2 target, Vector2 localplayer) {
	float distance = std::sqrtf((target.x - localplayer.x) * (target.x - localplayer.x) +
		(target.y - localplayer.y) * (target.y - localplayer.y));
	return distance;
}

bool WorldToScreen(const Vector3& in, Vector3& out) noexcept
{
	const ViewMatrix& mat = *(ViewMatrix*)mat_addr;

	float width = mat[0][3] * in.x + mat[1][3] * in.y + mat[2][3] * in.z + mat[3][3];

	bool visible = width >= 0.1f;
	if (!visible)
		return false;
	else
		width = 1.0 / width;

	float x = in.x * mat[0][0] + in.y * mat[1][0] + in.z * mat[2][0] + mat[3][0];
	float y = in.x * mat[0][1] + in.y * mat[1][1] + in.z * mat[2][1] + mat[3][1];
	float z = in.x * mat[0][2] + in.y * mat[1][2] + in.z * mat[2][2] + mat[3][2];

	float nx = x * width;
	float ny = y * width;
	float nz = z * width;


	out.x = (scrsize.x / 2 * nx) + (nx + scrsize.x / 2);
	out.y = -(scrsize.y / 2 * ny) + (ny + scrsize.y / 2);
	out.z = nz;

	return true;
}
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);



void InitImGui()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	def_main = io.Fonts->AddFontFromMemoryTTF(myfont, sizeof(myfont), 15.f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
	med_main = io.Fonts->AddFontFromMemoryTTF(myfont, sizeof(myfont), 20.f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
	big_main = io.Fonts->AddFontFromMemoryTTF(myfont, sizeof(myfont), 30.f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
	ImGui::GetIO().Fonts->Build();
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
	io.IniFilename = NULL;
	SetupImGuiStyle();
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDevice, pContext);
}


void draw3dbox(Matrix3x3 rotation, Vector3 bbmin, Vector3 bbmax, Vector3 position, float Invulnerable, Vector3& bbcenter)
{
	Vector3 ax[6];
	ax[0] = Vector3{ rotation[0][0], rotation[0][1], rotation[0][2] }.Scale(bbmin.x);
	ax[1] = Vector3{ rotation[1][0], rotation[1][1], rotation[1][2] }.Scale(bbmin.y);
	ax[2] = Vector3{ rotation[2][0], rotation[2][1], rotation[2][2] }.Scale(bbmin.z);
	ax[3] = Vector3{ rotation[0][0], rotation[0][1], rotation[0][2] }.Scale(bbmax.x);
	ax[4] = Vector3{ rotation[1][0], rotation[1][1], rotation[1][2] }.Scale(bbmax.y);
	ax[5] = Vector3{ rotation[2][0], rotation[2][1], rotation[2][2] }.Scale(bbmax.z);

	Vector3 temp[6];
	temp[0] = position + ax[2];
	temp[1] = position + ax[5];
	temp[2] = temp[0] + ax[3];
	temp[3] = temp[1] + ax[3];
	temp[4] = temp[0] + ax[0];
	temp[5] = temp[1] + ax[0];

	Vector3 v[8];
	v[0] = temp[2] + ax[1];
	v[1] = temp[2] + ax[4];
	v[2] = temp[3] + ax[4];
	v[3] = temp[3] + ax[1];
	v[4] = temp[4] + ax[1];
	v[5] = temp[4] + ax[4];
	v[6] = temp[5] + ax[4];
	v[7] = temp[5] + ax[1];

	const auto draw = ImGui::GetBackgroundDrawList();

	ImColor color1 = ImColor(255, 0, 0); //Front color
	ImColor color2 = ImColor(255, 255, 255); //Other
	

	if (Invulnerable > 0.f)
		color1 = ImColor(0, 255, 0, 200);

	Vector3 p1, p2;
	for (int i = 0; i < 4; i++)
	{

		if (WorldToScreen(v[i], p1) && WorldToScreen(v[(i + 1) & 3], p2))
			draw->AddLine({ p1.x, p1.y }, { p2.x, p2.y }, color1, 2.f);

		if (WorldToScreen(v[4 + i], p1) && WorldToScreen(v[4 + ((i + 1) & 3)], p2))
			draw->AddLine({ p1.x, p1.y }, { p2.x, p2.y }, color2, 2.f);

		if (WorldToScreen(v[i], p1) && WorldToScreen(v[4 + i], p2))
			draw->AddLine({ p1.x, p1.y }, { p2.x, p2.y }, color2, 2.f);
	}
	//center of the box 
	bbcenter = (v[0] + v[6]) / 2;
}

void draw3dbox(Matrix3x3 rotation, Vector3 bbmin, Vector3 bbmax, Vector3 position)
{
	Vector3 ax[6];
	ax[0] = Vector3{ rotation[0][0], rotation[0][1], rotation[0][2] }.Scale(bbmin.x);
	ax[1] = Vector3{ rotation[1][0], rotation[1][1], rotation[1][2] }.Scale(bbmin.y);
	ax[2] = Vector3{ rotation[2][0], rotation[2][1], rotation[2][2] }.Scale(bbmin.z);
	ax[3] = Vector3{ rotation[0][0], rotation[0][1], rotation[0][2] }.Scale(bbmax.x);
	ax[4] = Vector3{ rotation[1][0], rotation[1][1], rotation[1][2] }.Scale(bbmax.y);
	ax[5] = Vector3{ rotation[2][0], rotation[2][1], rotation[2][2] }.Scale(bbmax.z);

	Vector3 temp[6];
	temp[0] = position + ax[2];
	temp[1] = position + ax[5];
	temp[2] = temp[0] + ax[3];
	temp[3] = temp[1] + ax[3];
	temp[4] = temp[0] + ax[0];
	temp[5] = temp[1] + ax[0];

	Vector3 v[8];
	v[0] = temp[2] + ax[1];
	v[1] = temp[2] + ax[4];
	v[2] = temp[3] + ax[4];
	v[3] = temp[3] + ax[1];
	v[4] = temp[4] + ax[1];
	v[5] = temp[4] + ax[4];
	v[6] = temp[5] + ax[4];
	v[7] = temp[5] + ax[1];

	const auto draw = ImGui::GetBackgroundDrawList();

	ImColor color1 = ImColor(255, 0, 0);
	ImColor color2 = ImColor(255, 255, 255);

	Vector3 p1, p2;
	for (int i = 0; i < 4; i++)
	{

		if (WorldToScreen(v[i], p1) && WorldToScreen(v[(i + 1) & 3], p2))
			draw->AddLine({ p1.x, p1.y }, { p2.x, p2.y }, color1, 2.f);

		if (WorldToScreen(v[4 + i], p1) && WorldToScreen(v[4 + ((i + 1) & 3)], p2))
			draw->AddLine({ p1.x, p1.y }, { p2.x, p2.y }, color2, 2.f);

		if (WorldToScreen(v[i], p1) && WorldToScreen(v[4 + i], p2))
			draw->AddLine({ p1.x, p1.y }, { p2.x, p2.y }, color2, 2.f);
	}

}

void drawOffscreenCentered(Vector3 origin, float distance)
{
	ImRect screen_rect = { 0.0f, 0.0f, scrsize.x, scrsize.y };
	auto angle = atan2((scrsize.y / 2) - origin.y, (scrsize.x / 2) - origin.x);
	angle += origin.z > 0 ? M_PI : 0.0f;

	ImVec2 arrow_center{
		(scrsize.x / 2) + off_radius * cosf(angle),
		(scrsize.y / 2) + off_radius * sinf(angle)
	};
	std::array<ImVec2, 4> points{
		ImVec2(-22.0f, -8.6f),
		ImVec2(0.0f, 0.0f),
		ImVec2(-22.0f, 8.6f),
		ImVec2(-18.0f, 0.0f)
	};
	for (auto& point : points)
	{
		auto x = point.x * off_arrow_size;
		auto y = point.y * off_arrow_size;

		point.x = arrow_center.x + x * cosf(angle) - y * sinf(angle);
		point.y = arrow_center.y + x * sinf(angle) + y * cosf(angle);
	}
	auto draw = ImGui::GetBackgroundDrawList();
	float alpha = 1.0f;
	if (origin.z > 0)
	{
		constexpr float nearThreshold = 200.0f * 200.0f;
		ImVec2 screen_outer_diff =
		{
			origin.x < 0 ? abs(origin.x) : (origin.x > screen_rect.Max.x ? origin.x - screen_rect.Max.x : 0.0f),
			origin.y < 0 ? abs(origin.y) : (origin.y > screen_rect.Max.y ? origin.y - screen_rect.Max.y : 0.0f),
		};
		float distance = static_cast<float>(std::pow(screen_outer_diff.x, 2) + std::pow(screen_outer_diff.y, 2));
		alpha = origin.z < 0 ? 1.0f : (distance / nearThreshold);
	}
	auto arrowColor = ImColor((int)(off_color[0] * 255.f), (int)(off_color[1] * 255.f), (int)(off_color[2] * 255.f));
	arrowColor.Value.w = (std::min)(alpha, 1.0f);
	draw->AddTriangleFilled(points[0], points[1], points[3], arrowColor);
	draw->AddTriangleFilled(points[2], points[1], points[3], arrowColor);
	draw->AddQuad(points[0], points[1], points[2], points[3], ImColor(0.0f, 0.0f, 0.0f, alpha), 0.6f);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (open)
	{
		ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
		return true;
	}

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

void ESP()
{
	UnitList list = *(UnitList*)(cGame + 0x368);
	if (!list.unitList)
		return;
	Player* localplayer = *(Player**)(modulebase + offsets[1]);
	bool isScoping = *(bool*)(modulebase + offsets[4]);
	char* curmap = *(char**)(cGame + 0x1C0);
	if (localplayer->IsinHangar())
	{
		if (localplayer->ControlledUnit == NULL or localplayer->ControlledUnit->UnitInfo == NULL)
			return;
		auto position = localplayer->ControlledUnit->Position;
		const auto& rotation = localplayer->ControlledUnit->RotationMatrix;
		
		auto draw = ImGui::GetBackgroundDrawList();
		
		const Vector3& bbmin = localplayer->ControlledUnit->BBMin;
		const Vector3& bbmax = localplayer->ControlledUnit->BBMax;
		
		draw3dbox(rotation, bbmin, bbmax, position);

		return;
	}

	const auto screen_center_x = scrsize.x / 2;
	const auto screen_center_y = scrsize.y / 2;

	std::uint16_t unitCount = *(std::uint16_t*)(cGame + 0x378);
	for (auto i = 0; i < unitCount; ++i)
	{
		const auto unit = list.unitList->units[i];
		if (!unit)
			continue;
		Player* player = (unit->PlayerInfo == NULL) ? NULL : unit->PlayerInfo;

		const auto local = localplayer->ControlledUnit;
		if (!local)
			continue;
		if (unit->TeamNum == local->TeamNum)
			continue;
		if (unit->Position.x == 0)
			continue;
		const auto draw = ImGui::GetBackgroundDrawList();
		
		if (unit->UnitInfo->isPlane())
		{
			if (!show_planes)
			{
				continue;
			}
		}

		auto position = unit->Position;
		auto distance = Distance(position, local->Position);
		auto name = u8"";
		name = (char8_t*)(unit->UnitInfo->ShortName);
		auto text = std::format("{} - {}m", (char*)name, (int)distance, 2);
		auto size = ImGui::CalcTextSize(text.c_str());
		
		//reload line implementation
		int count = (16 - (unit->VisualReload));
		constexpr float stat = (10.f / 16);
		float progress = ((stat * count) * 0.1f);
		Vector3 bbcenter{};
		
		if (player)
		{
			if (!unit->UnitState == 0 or !player->IsAlive())
				continue;
			const auto& rotation = unit->RotationMatrix;
			const Vector3& bbmin = unit->BBMin;
			const Vector3& bbmax = unit->BBMax;

			
			draw3dbox(rotation, bbmin, bbmax, position, unit->Invulnerable, bbcenter);


			
			Vector3 origin = { };
			if (WorldToScreen(position, origin))
			{
				if (origin.x < 0 || origin.x > scrsize.x || origin.y < 0 || origin.y > scrsize.y)
				{
					if (show_offscreen)
					{
						if (!isScoping)
						{
							drawOffscreenCentered(origin, distance);
						}
						continue;
					}

					continue;
				}

				draw->AddRectFilled({ origin.x - (size.x * 0.5f) - 5, origin.y + 5 },
					{ origin.x + (size.x * 0.5f) + 5, origin.y + 10 + (size.y * 0.5f) + 5 },
					ImColor(0, 0, 0, 100));

				draw->AddText({ origin.x - (size.x * 0.5f), origin.y + (size.y * 0.5f) },
					ImColor(255, 255, 255),
					text.c_str());

				if (show_reload)
				{
					draw->AddRectFilled({ origin.x - (size.x * 0.5f) - 5, origin.y + 10 + (size.y * 0.5f) + 5 },
						{ origin.x + (size.x * 0.5f) + 5, origin.y + 10 + (size.y * 0.3f) + 10 },
						ImColor(0, 0, 0, 150));
					if (progress == 1)
					{
						draw->AddRectFilled({ origin.x - (size.x * 0.5f) - 5, origin.y + 10 + (size.y * 0.5f) + 5 },
							{ origin.x - (size.x * 0.5f) + (progress * size.x) + 5, origin.y + 10 + (size.y * 0.5f) + 10 },
							ImColor(0, 255, 0, 200));
					}
					else
					{
						draw->AddRectFilled({ origin.x - (size.x * 0.5f) - 5, origin.y + 10 + (size.y * 0.5f) + 5 },
							{ origin.x - (size.x * 0.5f) + (progress * size.x) + 5, origin.y + 10 + (size.y * 0.5f) + 10 },
							ImColor(255, 0, 0, 200));
					}
				}

			}
			continue;
		}
		else {
			if (show_bots)
			{
				if (strcmp(curmap, "levels/firing_range.bin") != 0) //If we wanna show bots only in firing range
					continue;

				if (!unit->UnitState == 0)
					continue;
				if (unit->UnitInfo->isDummy())
					continue;
				if (!show_planes)
				{
					if (unit->UnitInfo->isPlane())
						continue;
				}


				const auto& rotation = unit->RotationMatrix;
				const Vector3& bbmin = unit->BBMin;
				const Vector3& bbmax = unit->BBMax;
				text = std::format("BOT - {}m", (int)distance);
				size = ImGui::CalcTextSize(text.c_str());
				
				draw3dbox(rotation, bbmin, bbmax, position, unit->Invulnerable, bbcenter);

				///////////////////////////////////////////////////////////////////////////////////////////
				Vector3 origin = { };
				if (WorldToScreen(position, origin))
				{
					if (origin.x < 0 || origin.x > scrsize.x || origin.y < 0 || origin.y > scrsize.y)
					{
						if (show_offscreen)
						{

							if (!isScoping)
							{
								drawOffscreenCentered(origin, distance);
							}
							continue;
						}
						continue;
					}


					draw->AddRectFilled({ origin.x - (size.x * 0.5f) - 5, origin.y + 5 },
						{ origin.x + (size.x * 0.5f) + 5, origin.y + 10 + (size.y * 0.5f) + 5 },
						ImColor(0, 0, 0, 150));

					draw->AddText({ origin.x - (size.x * 0.5f), origin.y + (size.y * 0.5f) },
						ImColor(255, 255, 255),
						text.c_str());

					if (show_reload)
					{
						draw->AddRectFilled({ origin.x - (size.x * 0.5f) - 5, origin.y + 10 + (size.y * 0.5f) + 5 },
							{ origin.x + (size.x * 0.5f) + 5, origin.y + 10 + (size.y * 0.3f) + 10 },
							ImColor(0, 0, 0, 150));
						if (progress == 1)
						{
							draw->AddRectFilled({ origin.x - (size.x * 0.5f) - 5, origin.y + 10 + (size.y * 0.5f) + 5 },
								{ origin.x - (size.x * 0.5f) + (progress * size.x) + 5, origin.y + 10 + (size.y * 0.5f) + 10 },
								ImColor(0, 255, 0, 200));
						}
						else
						{
							draw->AddRectFilled({ origin.x - (size.x * 0.5f) - 5, origin.y + 10 + (size.y * 0.5f) + 5 },
								{ origin.x - (size.x * 0.5f) + (progress * size.x) + 5, origin.y + 10 + (size.y * 0.5f) + 10 },
								ImColor(255, 0, 0, 200));
						}
					}

				}

			}
		}

	}

}

void HudChanger()
{
	UnitList list = *(UnitList*)(cGame + 0x368);
	WTF* addr = *(WTF**)(cGame + 0x4c8);
	if (!list.unitList)
		return;
	HUD* HudInfo = *(HUD**)(modulebase + offsets[2]);
	if (change_hud)
	{

		HudInfo->penetration_crosshair = true;
		addr->crosshair_distance = 2000.f;
		addr->penetration_distance = 2000.f;

		HudInfo->unit_glow = true;

		HudInfo->gunner_sight_distance = true;

		HudInfo->air_to_air_indicator = true;
		
		HudInfo->draw_plane_aim_indicator = true;
		
		HudInfo->show_bombsight = true;
	}
	else {
		HudInfo->penetration_crosshair = true;
		addr->crosshair_distance = 1000.f;
		addr->penetration_distance = 650.f;

		HudInfo->unit_glow = false;

		HudInfo->gunner_sight_distance = false;

		HudInfo->air_to_air_indicator = true;

		HudInfo->draw_plane_aim_indicator = true;

		HudInfo->show_bombsight = false;
	}
	return;
}

void ZoomMod()
{
	UnitList list = *(UnitList*)(cGame + 0x368);
	if (!list.unitList)
		return;
	Player* localplayer = *(Player**)(modulebase + offsets[1]);
	if (localplayer->IsinHangar())
		return;
	if (localplayer->ControlledUnit == NULL or localplayer->ControlledUnit->UnitInfo == NULL)
		return;

	if (zoom_mod)
	{
		localplayer->ControlledUnit->UnitInfo->ZoomMulti = zoom_mult;
		localplayer->ControlledUnit->UnitInfo->AlternateMulti = alt_mult;
		localplayer->ControlledUnit->UnitInfo->ShadowMulti = shadow_mult;
	}
	else
	{
		localplayer->ControlledUnit->UnitInfo->ZoomMulti = DEFAULT_ZOOM_MULT;
		localplayer->ControlledUnit->UnitInfo->AlternateMulti = DEFAULT_ALT_MULT;
		localplayer->ControlledUnit->UnitInfo->ShadowMulti = DEFAULT_SHADOW_MULT;
	}
	return;
}

void showWarningwindow()
{
	auto text1 = "THIS SOFTWARE DISTRIBUTED FOR FREE.";
	auto text2 = "IF YOU PAID FOR THIS SOFTWARE - YOU GOT SCAMMED.";
	ImGui::SetNextWindowSize({ scrsize.x, scrsize.y });
	ImGui::SetNextWindowPos({ 0, 0 });
	auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
	ImGui::Begin("WARNING", nullptr, flags);
	ImGui::PushFont(big_main);
	auto textWidth1 = ImGui::CalcTextSize(text1).x;
	ImGui::SetCursorPos(ImVec2((scrsize.x - textWidth1) * 0.5f, 100));
	ImGui::Text(text1);
	auto textWidth2 = ImGui::CalcTextSize(text2).x;
	ImGui::SetCursorPosX((scrsize.x - textWidth2) * 0.5f);
	ImGui::Text(text2);
	ImGui::SetCursorPosX((scrsize.x - 225) * 0.5f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
	ImGui::Checkbox("I understood that", &agree);
	ImGui::PopStyleVar();
	ImGui::PopFont();
	ImGui::End();
}

bool init = false;

ID3D11RasterizerState* rwState;
ID3D11RasterizerState* rsState;

enum eDepthState
{
	ENABLED,
	DISABLED,
	READ_NO_WRITE,
	NO_READ_NO_WRITE,
	_DEPTH_COUNT
};

ID3D11PixelShader* psCrimson = NULL;
ID3D11PixelShader* psYellow = NULL;
ID3D11ShaderResourceView* ShaderResourceView;

ID3D11DepthStencilState* myDepthStencilStates[static_cast<int>(eDepthState::_DEPTH_COUNT)];

void SetDepthStencilState(eDepthState aState)
{	
	pContext->OMSetDepthStencilState(myDepthStencilStates[aState], 1);
}

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (!init)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)))
		{
			pDevice->GetImmediateContext(&pContext);
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			window = sd.OutputWindow;
			ID3D11Texture2D* pBackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
			pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
			pBackBuffer->Release();
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
			InitImGui();
			init = true;

			////////////////////////////////////////////////////////////////////////////////
			D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
			depthStencilDesc.DepthEnable = TRUE;
			depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
			depthStencilDesc.StencilEnable = FALSE;
			depthStencilDesc.StencilReadMask = 0xFF;
			depthStencilDesc.StencilWriteMask = 0xFF;
			// Stencil operations if pixel is front-facing
			depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
			depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			// Stencil operations if pixel is back-facing
			depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
			depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			pDevice->CreateDepthStencilState(&depthStencilDesc, &DepthStencilState_FALSE);

			//create depthbias rasterizer state
			D3D11_RASTERIZER_DESC rasterizer_desc;
			ZeroMemory(&rasterizer_desc, sizeof(rasterizer_desc));
			rasterizer_desc.FillMode = D3D11_FILL_SOLID;
			rasterizer_desc.CullMode = D3D11_CULL_NONE; //D3D11_CULL_FRONT;
			rasterizer_desc.FrontCounterClockwise = false;
			float bias = 1000.0f;
			float bias_float = static_cast<float>(-bias);
			bias_float /= 10000.0f;
			rasterizer_desc.DepthBias = DEPTH_BIAS_D32_FLOAT(*(DWORD*)&bias_float);
			rasterizer_desc.SlopeScaledDepthBias = 0.0f;
			rasterizer_desc.DepthBiasClamp = 0.0f;
			rasterizer_desc.DepthClipEnable = true;
			rasterizer_desc.ScissorEnable = false;
			rasterizer_desc.MultisampleEnable = false;
			rasterizer_desc.AntialiasedLineEnable = false;
			pDevice->CreateRasterizerState(&rasterizer_desc, &DEPTHBIASState_FALSE);

			//create normal rasterizer state
			D3D11_RASTERIZER_DESC nrasterizer_desc;
			ZeroMemory(&nrasterizer_desc, sizeof(nrasterizer_desc));
			nrasterizer_desc.FillMode = D3D11_FILL_SOLID;
			//nrasterizer_desc.CullMode = D3D11_CULL_BACK; //flickering
			nrasterizer_desc.CullMode = D3D11_CULL_NONE;
			nrasterizer_desc.FrontCounterClockwise = false;
			nrasterizer_desc.DepthBias = 0.0f;
			nrasterizer_desc.SlopeScaledDepthBias = 0.0f;
			nrasterizer_desc.DepthBiasClamp = 0.0f;
			nrasterizer_desc.DepthClipEnable = true;
			nrasterizer_desc.ScissorEnable = false;
			nrasterizer_desc.MultisampleEnable = false;
			nrasterizer_desc.AntialiasedLineEnable = false;
			pDevice->CreateRasterizerState(&nrasterizer_desc, &DEPTHBIASState_TRUE);
			/////////////////////////////////////////////////////////////////////////////////
		}

		else
			return oPresent(pSwapChain, SyncInterval, Flags);
	}
	
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();
	auto draw = ImGui::GetBackgroundDrawList();
	auto sizeScr = ImGui::GetIO().DisplaySize;

	if (GetAsyncKeyState(VK_INSERT) & 1)
	{
		open = !open;
	}


	if(!agree)
		showWarningwindow();

	ImGui::GetIO().MouseDrawCursor = open;

	if (agree)
	{
		if (open)
		{
			ImGui::PushFont(med_main);
			ImGui::SetNextWindowSize({ 550,350 });
			ImGui::SetNextWindowPos({ (sizeScr.x - 550) * 0.5f, (sizeScr.y - 350) * 0.5f }, ImGuiCond_FirstUseEver);
			ImGui::Begin("WarHook", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

			ImGui::BeginTabBar("main");
			ImGui::SetNextItemWidth(180.f);
			if (ImGui::BeginTabItem("\t\t\t\tESP", &tab_esp, ImGuiTabItemFlags_NoCloseButton))
			{

				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4,5 });
				ImGui::SetCursorPosX(5.f);
				ImGui::Checkbox("Enabled", &esp_status);
				ImGui::SetCursorPosX(5.f);
				ImGui::Checkbox("Show reload bar", &show_reload);
				ImGui::SetCursorPosX(5.f);
				ImGui::Checkbox("Show bots", &show_bots);
				ImGui::SetCursorPosX(5.f);
				ImGui::Checkbox("Show planes", &show_planes);
				ImGui::SetCursorPosX(5.f);
				ImGui::Checkbox("Show offscreen arrows", &show_offscreen);
				if (show_offscreen)
				{
					ImGui::SameLine();
					ImGui::ColorEdit3("Arrow color", off_color, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
					ImGui::Indent();
					ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
					ImGui::SliderFloat("Arrow size", &off_arrow_size, 0.0f, 3.f);
					ImGui::SliderFloat("Radius", &off_radius, 0.0f, 1000.0f);
					ImGui::PopStyleVar();
				}
				//ImGui::SetCursorPosX(5.f);
				//ImGui::Checkbox("Remove nature", &remove_nature);
				ImGui::SetCursorPosX(5.f);
				ImGui::Checkbox("Remove smokes", &remove_smokes);
				ImGui::PopStyleVar();
				ImGui::PopStyleVar();
				ImGui::EndTabItem();
			}

			ImGui::SetNextItemWidth(180.f);
			if (ImGui::BeginTabItem("				Misc", &tab_misc, ImGuiTabItemFlags_NoCloseButton))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4,5 });
				ImGui::SetCursorPosX(5.f);
				ImGui::Checkbox("Zoom modification", &zoom_mod);
				ImGui::SetCursorPosX(5.f);
				ImGui::Text("Zoom multiplayer");
				ImGui::SetCursorPosX(5.f);
				ImGui::SliderFloat("##zoom", &zoom_mult, 1.0f, 100.0f);
				ImGui::SetCursorPosX(5.f);
				ImGui::Text("Alternate multiplayer");
				ImGui::SetCursorPosX(5.f);
				ImGui::SliderFloat("##alt", &alt_mult, 1.0f, 100.0f);
				ImGui::SetCursorPosX(5.f);
				ImGui::Text("Shadow zoom multiplayer");
				ImGui::SetCursorPosX(5.f);
				ImGui::SliderFloat("##shadow", &shadow_mult, 20.0f, 250.0f);
				ImGui::SetCursorPosX(5.f);
				ImGui::Checkbox("Change HUD", &change_hud);
				/*if (change_hud)
				{
					ImGui::SetCursorPosX(5.f);
					ImGui::Checkbox("Enable bomb crosshair", &force_bombsight);
					ImGui::SetCursorPosX(5.f);
					ImGui::Checkbox("Enable Air-To_air indicator", &force_air_to_air);
					ImGui::SetCursorPosX(5.f);
					ImGui::Checkbox("Enable enemy outline (when hovered)", &force_outline);
					ImGui::SetCursorPosX(5.f);
					ImGui::Checkbox("Show distance in scope", &force_distance);
					ImGui::SetCursorPosX(5.f);
					ImGui::Checkbox("Show penetrarion indicator", &force_crosshair);
				}
				*/
				ImGui::PopStyleVar();
				ImGui::PopStyleVar();
				ImGui::EndTabItem();

			}
			ImGui::SetNextItemWidth(180.f);
			if (ImGui::BeginTabItem("		 Support Dev", &tab_support, ImGuiTabItemFlags_NoCloseButton))
			{
				ImGui::SetCursorPosX(5.f);
				ImGui::Text("Current version: 1.5");
				ImGui::SetCursorPosX(5.f);
				ImGui::Text("Support Dev with Ko-Fi (PayPal)");
				ImGui::SameLine();
				if (ImGui::Button("Copy link##1"))
				{
					ImGui::LogToClipboard();
					ImGui::LogText("https:\/\/ko-fi.com\/monkrel");
					ImGui::LogFinish();
				}
				ImGui::SetCursorPosX(5.f);
				ImGui::Text("Support Dev with BuyMeACoffee (CC)");
				ImGui::SameLine();
				if (ImGui::Button("Copy link##2"))
				{
					ImGui::LogToClipboard();
					ImGui::LogText("https:\/\/www.buymeacoffee.com\/monkrel");
					ImGui::LogFinish();
				}
				ImGui::Indent();
				ImGui::SetCursorPosX(5.f);
				ImGui::Text("Crypto:");
				ImGui::SetCursorPosX(5.f);
				ImGui::Text("Bitcoin");
				ImGui::SameLine();
				if (ImGui::Button("Copy address##btc"))
				{
					ImGui::LogToClipboard();
					ImGui::LogText("bc1qstz3rpazwm70f95mmj53360rqxqz5qzn2vlm8r");
					ImGui::LogFinish();
				}
				ImGui::SetCursorPosX(5.f);
				ImGui::Text("Ethereum");
				ImGui::SameLine();
				if (ImGui::Button("Copy address##eth"))
				{
					ImGui::LogToClipboard();
					ImGui::LogText("0xf15357E8ABDB25f303D5D0610aBF803A162b8a03");
					ImGui::LogFinish();
				}
				ImGui::SetCursorPosX(5.f);
				ImGui::Text("Tron (TRX)");
				ImGui::SameLine();
				if (ImGui::Button("Copy address##trx"))
				{
					ImGui::LogToClipboard();
					ImGui::LogText("TNHZFDcT8JVmPRqtWg3s11TK6rCQ5eYhwW");
					ImGui::LogFinish();
				}
				ImGui::Indent();
				ImGui::SetCursorPosX(5.f);
				ImGui::Text("If you want to support me with other method - contact me on Discord:");
				ImGui::SetCursorPosX(5.f);
				if (ImGui::Button("Copy discord"))
				{
					ImGui::LogToClipboard();
					ImGui::LogText("monkrel#0001");
					ImGui::LogFinish();
				}

				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
			if (!tab_esp && !tab_misc && !tab_support)
			{
				if (def_tab)
				{
					auto text1 = "This mod is made by monkrel ";
					auto text2 = "with love <3";
					auto windowWidth = ImGui::GetWindowSize().x;
					auto textWidth = ImGui::CalcTextSize(text1).x;

					ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
					ImGui::Text(text1);

					textWidth = ImGui::CalcTextSize(text2).x;
					ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
					ImGui::Text(text2);


					ImGui::SetCursorPos({ (windowWidth - 100) * 0.5f ,150 });
					ImGui::Button("Start", { 100,30 });


					if (ImGui::IsItemClicked())
					{
						tab_esp = true;
						tab_support = true;
						tab_misc = true;
						def_tab = false;
					}
				}
			}
			ImGui::PopFont();
			
			ImGui::End();
		}

	}
	if (esp_status)
		ESP();
	HudChanger();
	ZoomMod();
	ImGui::Render();
	pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	return oPresent(pSwapChain, SyncInterval, Flags);
}
LRESULT CALLBACK DXGIMsgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) { return DefWindowProc(hwnd, uMsg, wParam, lParam); }

DWORD WINAPI MainThread(LPVOID lpReserved)
{

	
	bool init_hook = false;
	do
	{
		if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
		{
			kiero::bind(8, (void**)&oPresent, hkPresent);
			kiero::bind(73, (void**)&phookD3D11DrawIndexed, hookD3D11DrawIndexed);
			kiero::bind(13, (void**)&phookD3D11ResizeBuffers, hookD3D11ResizeBuffers);
			kiero::bind(81, (void**)&phookD3D11DrawIndexedInstanced, hookD3D11DrawIndexedInstanced);
			init_hook = true;
		}
	} while (!init_hook);

	return TRUE;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hMod);
		CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
		break;
	case DLL_PROCESS_DETACH:
		kiero::shutdown();
		break;
	}
	return TRUE;
}