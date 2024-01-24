/*
* Created by: John Ford
* Date: 1/25/2021
*/
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <tchar.h>

#include "utils/xor.h"
#include "utils/sha512.h"
#include "utils/md5.h"
#include "main.h"

#include <d3d9.h>
#define DIRECTINPUT_VERSION 0x0800
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_internal.h"

#define CURL_STATICLIB 
#pragma comment (lib, "Normaliz.lib")
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Wldap32.lib")
#pragma comment (lib, "Crypt32.lib")
#pragma comment (lib, "advapi32.lib")
#include <curl/curl.h>

static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool ImGui::Spinner(const char* label, float radius, int thickness) {
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);

	ImVec2 pos = window->DC.CursorPos;
	ImVec2 size((radius) * 2, (radius + style.FramePadding.y) * 2);

	const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
	ImGui::ItemSize(bb, style.FramePadding.y);
	if (!ImGui::ItemAdd(bb, id))
		return false;

	window->DrawList->PathClear();

	int num_segments = 30;
	int start = abs(ImSin(g.Time * 1.8f) * (num_segments - 5));

	const float a_min = IM_PI * 2.0f * ((float)start) / (float)num_segments;
	const float a_max = IM_PI * 2.0f * ((float)num_segments - 3) / (float)num_segments;

	const ImVec2 centre = ImVec2(pos.x + radius, pos.y + radius + style.FramePadding.y);

	for (int i = 0; i < num_segments; i++) {
		const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
		window->DrawList->PathLineTo(ImVec2(centre.x + ImCos(a + g.Time * 8) * radius,
			centre.y + ImSin(a + g.Time * 8) * radius));
	}

	//Changed this from the orginal because they made it shitty
	window->DrawList->PathStroke(ImColor(235, 73, 73, 100.0), false, thickness);
}
size_t writeback(void* contents, size_t size, size_t nmemb, void* userp) {
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

void gui() {
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T(" "), NULL };
	RegisterClassEx(&wc);
	hwnd = ::CreateWindow(wc.lpszClassName, _T(" "), WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX, 100, 100, 550, 380, NULL, NULL, wc.hInstance, NULL);

	if (!CreateDeviceD3D(hwnd))
	{
		CleanupDeviceD3D();
		::UnregisterClass(wc.lpszClassName, wc.hInstance);
		return;
	}

	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX9_Init(g_pd3dDevice);

	auto default_font = io.Fonts->AddFontDefault();

	io.IniFilename = NULL;
	ImFontConfig config;
	config.MergeMode = true;
	config.PixelSnapH = true;

	static const ImWchar ranges[] =
	{
		0xf000,
		0xf976,
		NULL
	};

	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) != 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		ImGuiStyle* style = &ImGui::GetStyle();
		style->WindowRounding = 0.0f;
		style->WindowBorderSize = 1.0f;
		style->Alpha = 1.f;
		style->WindowPadding = ImVec2(0, 0);
		style->WindowRounding = 0.0f;
		style->FramePadding = ImVec2(5, 5);
		style->FrameRounding = 4.5f;
		style->ItemSpacing = ImVec2(5, 5);
		style->ItemInnerSpacing = ImVec2(5, 5);
		style->IndentSpacing = 10.0f;
		style->TouchExtraPadding = ImVec2(5, 5);
		style->ScrollbarSize = 13.0f;
		style->ScrollbarRounding = 15.0f;
		style->GrabMinSize = 10.0f;
		style->GrabRounding = 2.0f;
		style->ColumnsMinSpacing = 10.0f;
		style->ButtonTextAlign = ImVec2(0.5, 0.5);
		style->WindowTitleAlign = ImVec2(0.5, 0.5);

		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImColor(255, 255, 255, 155);
		colors[ImGuiCol_WindowBg] = ImColor(20, 20, 20);
		colors[ImGuiCol_Border] = ImColor(20, 20, 20);
		colors[ImGuiCol_FrameBg] = ImColor(40, 40, 40, 100.0);
		colors[ImGuiCol_FrameBgHovered] = ImColor(40, 40, 40, 100.0);
		colors[ImGuiCol_FrameBgActive] = ImColor(40, 40, 40, 100.0);
		colors[ImGuiCol_Button] = ImColor(40, 40, 40, 100.0);
		colors[ImGuiCol_ButtonHovered] = ImColor(40, 40, 40, 160.0);
		colors[ImGuiCol_ButtonActive] = ImColor(40, 40, 40, 160.0);
		colors[ImGuiCol_PlotHistogram] = ImColor(52, 82, 225, 120.0);
		colors[ImGuiCol_PlotHistogramHovered] = ImColor(52, 82, 225, 120.0);

		Sleep(6);
		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		{
			ImGui::PushFont(default_font);
			ImGui::Begin(xorstr(" "), NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
			ImGui::SetWindowPos(ImVec2(0, 0));
			ImGui::SetWindowSize(ImVec2(550, 380));
			ImGui::SetCursorPos(ImVec2(10, 320));
			ImGui::Text(xorstr("v1.0"));

			if (fshjfghjsdsfs == 0) {
				ImGui::SetCursorPos(ImVec2(158, 80));
				ImGui::Text(xorstr("Username"));
				ImGui::SetCursorPos(ImVec2(158, 100));
				ImGui::BeginChild(xorstr("##usernamebox"), ImVec2(325, 40));
				ImGui::InputText("", buck50::username, IM_ARRAYSIZE(buck50::username));
				ImGui::EndChild();
				ImGui::SetCursorPos(ImVec2(158, 140));
				ImGui::Text(xorstr("Password"));
				ImGui::SetCursorPos(ImVec2(158, 160));
				ImGui::BeginChild(xorstr("##passwordbox"), ImVec2(325, 40));
				ImGui::InputText("", buck50::password, IM_ARRAYSIZE(buck50::password), ImGuiInputTextFlags_Password);
				ImGui::EndChild();

				ImGui::SetCursorPos(ImVec2(214, 220));
				if (ImGui::Button(xorstr("Login"), ImVec2(92, 28))) {
					CURL* curl;
					CURLcode res;
					//Data
					std::string cdfcdshkjsd{};
					curl = curl_easy_init();
					if (curl) {
						
						curl_easy_setopt(curl, CURLOPT_URL, /*Auth link ->*/xorstr(""));
						curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
						curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 2L);
						curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1L);
						curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
						curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeback);
						curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cdfcdshkjsd);
						res = curl_easy_perform(curl);
					}
					curl_easy_cleanup(curl);

					if (cdfcdshkjsd.find(((std::string)buck50::username + xorstr("::") + sha512((std::string)buck50::password + xorstr(":pass")))) != std::string::npos) {
						std::cout << xorstr("[dbg] Auth attempt made (Accepted)") << std::endl;
						fshjfghjsdsfs = 1;
					} else {
						::ShowWindow(::GetConsoleWindow(), SW_SHOW);
						std::cout << xorstr("[dbg] Auth attempt made (Invalid username or pass)") << std::endl;
						Sleep(2000);
						::ShowWindow(::GetConsoleWindow(), SW_HIDE);
					}
				}
			} 

			if (fshjfghjsdsfs == 1) {
				ImGui::SetCursorPos(ImVec2(214, 220));
				ImGui::Text(xorstr("Hello"));
			}

			ImGui::End();
			ImGui::PopFont();
		}

		ImGui::EndFrame();
		g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
		g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * 255.0f), (int)(clear_color.y * 255.0f), (int)(clear_color.z * 255.0f), (int)(clear_color.w * 255.0f));
		g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
		if (g_pd3dDevice->BeginScene() >= 0)
		{
			ImGui::Render();
			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
			g_pd3dDevice->EndScene();
		}
		HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
		if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
			ResetDevice();
	}
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClass(wc.lpszClassName, wc.hInstance);
	return;
}
int main() {
	//Hiding the console (You can also just use winmain)
	::ShowWindow(::GetConsoleWindow(), SW_HIDE);
	//DearImGui
	gui();
}
bool CreateDeviceD3D(HWND hWnd) {
	if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
		return false;

	// Create the D3DDevice
	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
	//g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
	if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
		return false;

	return true;
}
void CleanupDeviceD3D() {
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
	if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}
void ResetDevice() {
	ImGui_ImplDX9_InvalidateDeviceObjects();
	HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
	if (hr == D3DERR_INVALIDCALL)
		IM_ASSERT(0);
	ImGui_ImplDX9_CreateDeviceObjects();
}
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
		{
			g_d3dpp.BackBufferWidth = LOWORD(lParam);
			g_d3dpp.BackBufferHeight = HIWORD(lParam);
			ResetDevice();
		}
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
