#include "WebRenderer.hpp"
#include "WebRendererHandler.hpp"
#include <cef_app.h>

#include <d3dx9.h>
#include <Utils/Logger.hpp>

using Anvil::Client::Rendering::WebRenderer;

WebRenderer* WebRenderer::m_Instance = nullptr;
WebRenderer::WebRenderer() : 
	m_Browser(nullptr),
	m_Client(nullptr),
	m_RenderHandler(nullptr),
	m_Initialized(false),
	m_RenderingInitialized(false),
	m_Texture(nullptr),
	m_Device(nullptr),
	m_Sprite(nullptr),
	m_Font(nullptr)
{
}

WebRenderer* WebRenderer::GetInstance()
{
	if (!m_Instance)
		m_Instance = new WebRenderer;
	return m_Instance;
}

bool WebRenderer::Init()
{
	if (!m_RenderingInitialized)
		return false;

	CefMainArgs s_Args(GetModuleHandle(nullptr));

	auto s_Result = CefExecuteProcess(s_Args, nullptr, nullptr);
	if (s_Result >= 0)
	{
		WriteLog("CefExecuteProcess failed.");
		TerminateProcess(GetCurrentProcess(), 0);
		return false;
	}

	CefSettings s_Settings;
	s_Settings.multi_threaded_message_loop = true;
	CefString(&s_Settings.product_version) = "AnvilOnline";
	CefString(&s_Settings.browser_subprocess_path) = "cefsimple.exe";
	s_Settings.no_sandbox = true;
	s_Settings.pack_loading_disabled = false;
	s_Settings.windowless_rendering_enabled = true;
	s_Settings.ignore_certificate_errors = true;
	s_Settings.log_severity = LOGSEVERITY_VERBOSE;
	s_Settings.single_process = false;
	s_Settings.remote_debugging_port = 8884;

	if (!CefInitialize(s_Args, s_Settings, nullptr, nullptr))
	{
		WriteLog("CefInitialize failed.");
		ExitProcess(0);
		return false;
	}

	m_RenderHandler = new WebRendererHandler(m_Device);

	CefWindowInfo s_WindowInfo;
	CefBrowserSettings s_BrowserSettings;

	s_BrowserSettings.windowless_frame_rate = 60;

	D3DDEVICE_CREATION_PARAMETERS s_Parameters;
	ZeroMemory(&s_Parameters, sizeof(s_Parameters));

	s_Result = m_Device->GetCreationParameters(&s_Parameters);
	if (FAILED(s_Result))
		return false;

	s_WindowInfo.SetAsWindowless(s_Parameters.hFocusWindow, true);

	m_Client = new WebRendererClient(m_RenderHandler);

	auto s_RequestContext = CefRequestContext::GetGlobalContext();
	if (!CefBrowserHost::CreateBrowser(s_WindowInfo, m_Client.get(), "http://google.com", s_BrowserSettings, s_RequestContext))
	{
		m_Initialized = false;
		WriteLog("Failed to initialize WebRenderer.");
		return false;
	}

	CefRect s_Rect;

	s_Result = m_RenderHandler->GetViewRect(m_Browser, s_Rect);
	if (!s_Result)
		return false;

	if (!Resize(s_Rect.width, s_Rect.height))
		return false;

	m_Initialized = true;

	WriteLog("WebRenderer init.");
	return true;
}

bool WebRenderer::InitRenderer(LPDIRECT3DDEVICE9 p_Device)
{
	WriteLog("WebRenderer InitRenderer.");

	if (!p_Device)
		return false;

	m_Device = p_Device;

	// Create a sprite for rendering
	auto s_Result = D3DXCreateSprite(m_Device, &m_Sprite);
	if (FAILED(s_Result))
	{
		WriteLog("Could not create sprite (%x).", s_Result);
		return false;
	}

	// Create a font for debug purposes
	s_Result = D3DXCreateFont(m_Device, 16, 0, FW_NORMAL, 1, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial", &m_Font);
	if (FAILED(s_Result))
	{
		WriteLog("Could not create font (%x).", s_Result);
		return false;
	}

	D3DVIEWPORT9 s_Viewport;
	s_Result = p_Device->GetViewport(&s_Viewport);
	if (FAILED(s_Result))
		return false;

	// Create a texture
	s_Result = p_Device->CreateTexture(s_Viewport.Width, s_Viewport.Height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_Texture, nullptr);
	if (FAILED(s_Result))
		return false;

	m_RenderingInitialized = true;
	return true;
}

bool WebRenderer::Render(LPDIRECT3DDEVICE9 p_Device)
{
	if (!p_Device)
		return false;

	if (!m_Sprite || !m_Font || !m_Texture || !m_RenderHandler)
		return false;

	auto s_RenderHandler = static_cast<WebRendererHandler*>(m_RenderHandler.get());

	D3DLOCKED_RECT s_Rect;
	auto s_Result = m_Texture->LockRect(0, &s_Rect, nullptr, D3DLOCK_DISCARD);
	if (SUCCEEDED(s_Result))
	{
		unsigned long s_Width = 0, s_Height = 0;
		if (!s_RenderHandler->GetViewportInformation(s_Width, s_Height))
		{
			m_Texture->UnlockRect(0);
			return false;
		}

		auto s_TextureData = s_RenderHandler->GetTexture();
		if (!s_TextureData)
		{
			m_Texture->UnlockRect(0);
			return false;
		}

		memcpy(s_Rect.pBits, s_TextureData, s_Width * s_Height * 4);

		m_Texture->UnlockRect(0);
	}

	p_Device->Clear(1, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(255, 128, 128, 128), 0, 0);

	m_Sprite->Begin(D3DXSPRITE_ALPHABLEND);

	m_Sprite->Draw(m_Texture, nullptr, nullptr, &D3DXVECTOR3(0, 0, 0), 0xFFFFFFFF);

	m_Sprite->Flush();

	m_Sprite->End();

	return true;
}

bool WebRenderer::Update()
{
	if (!m_Initialized)
		return false;

	return true;
}

bool WebRenderer::Initialized()
{
	return m_Initialized;
}

bool WebRenderer::Resize(unsigned long p_Width, unsigned long p_Height)
{
	if (!m_RenderHandler)
		return false;

	auto s_Result = 
		static_cast<WebRendererHandler*>(m_RenderHandler.get())->Resize(p_Width, p_Height);

	if (!s_Result)
	{
		WriteLog("Resize failed.");
		return false;
	}

	return true;
}