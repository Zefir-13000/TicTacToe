#include <Texture.h>
#include <GameEngine.h>

Texture::Texture(uint8_t* srcData, uint32_t width, uint32_t height) {
	m_width = width;
	m_height = height;

	if (srcData) {
		InitBitmap(srcData);
	}
	else {
		uint8_t* data = new uint8_t[width * height * 4];
		memset(data, 0, width * height * 4);
		InitBitmap(data);
		delete[] data;
	}
}

Texture::~Texture() {
	if (m_bitmap) {
		OutputDebugString("Releasing m_bitmap\n");
		OutputDebugString(m_filename.c_str());

		m_bitmap->Release();
		m_bitmap = nullptr;
		OutputDebugString("m_bitmap released\n");
	}
}

bool Texture::InitBitmap(uint8_t* srcData) {
	if (m_bitmap) {
		m_bitmap->Release();
		m_bitmap = nullptr;
	}

	D2D1_PIXEL_FORMAT pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_IGNORE);
	HRESULT hr = GetRenderTarget()->CreateBitmap(
		D2D1::SizeU(m_width, m_height),
		srcData,
		m_width * 4,
		D2D1::BitmapProperties(pixelFormat),
		&m_bitmap);

	if (FAILED(hr)) {
		m_bitmap = nullptr;
		return false;
	}
	return true;
}

ID2D1Bitmap* Texture::GetBitmap() const {
	return m_bitmap;
}

Vector2i Texture::GetSize() const {
	Vector2i res(m_width, m_height);
	return res;
}

D2D1_RECT_F Texture::GetDrawRect(Vector2f pos, Vector2i size) {
	return D2D1::RectF(pos.x, pos.y, pos.x + size.x, pos.y + size.y);
}

void Texture::UpdateData(uint8_t* srcData) {
	InitBitmap(srcData);
}

void Texture::UpdateSize(uint32_t width, uint32_t height) {
	m_width = width;
	m_height = height;

	uint8_t* data = new uint8_t[width * height * 4];
	memset(data, 0, width * height * 4);
	InitBitmap(data);
	delete[] data;
}

Texture* Texture::GetEmptyTexture(uint32_t width, uint32_t height) {
	Texture* newTexture = new Texture(nullptr, width, height);
	return newTexture;
}

bool Texture::LoadFromFile(std::string filename) {
	if (m_bitmap) {
		m_bitmap->Release();
		m_bitmap = nullptr;
	}

	IWICImagingFactory* pIWICFactory = nullptr;
	IWICBitmapDecoder* pDecoder = NULL;
	IWICBitmapFrameDecode* pSource = NULL;
	IWICStream* pStream = NULL;
	IWICFormatConverter* pConverter = NULL;

	CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, reinterpret_cast<void**>(&pIWICFactory));

	if (filename != TEXTURE_DEFAULT_FILENAME && filename.find("\\") == filename.npos) {

		char buffer[MAX_PATH];
		GetModuleFileName(NULL, buffer, MAX_PATH);

		std::string exe_path(buffer);
		std::string filename_path = exe_path.substr(0, exe_path.rfind("\\"));
		filename = filename_path + "\\data\\images\\" + filename;
	}
	std::wstring wfilename(filename.begin(), filename.end());

	HRESULT hr = pIWICFactory->CreateDecoderFromFilename(
		wfilename.c_str(),
		NULL,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&pDecoder
	);

	if (FAILED(hr)) {
        OutputDebugString("Failed to create decoder from filename\n");
        pIWICFactory->Release();  // Clean up factory
        return false;
    }

	hr = pDecoder->GetFrame(0, &pSource);
	if (FAILED(hr)) {
		OutputDebugString("Failed to get frame from decoder\n");
		pDecoder->Release();
		pIWICFactory->Release();
		return false;
	}

	hr = pIWICFactory->CreateFormatConverter(&pConverter);
	if (FAILED(hr)) {
		OutputDebugString("Failed to create format converter\n");
		pSource->Release();
		pDecoder->Release();
		pIWICFactory->Release();
		return false;
	}

	pSource->GetSize(&m_width, &m_height);

	hr = pConverter->Initialize(
		pSource,
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone,
		NULL,
		0.f,
		WICBitmapPaletteTypeMedianCut
	);
	if (FAILED(hr)) {
		OutputDebugString("Failed to initialize format converter\n");
		pConverter->Release();
		pSource->Release();
		pDecoder->Release();
		pIWICFactory->Release();
		return false;
	}

	hr = GetRenderTarget()->CreateBitmapFromWicBitmap(pConverter, NULL, &m_bitmap);
	if (FAILED(hr)) {
		OutputDebugString("Failed to create bitmap from WIC bitmap\n");
		pConverter->Release();
		pSource->Release();
		pDecoder->Release();
		pIWICFactory->Release();
		return false;
	}

	filename = filename.substr(filename.rfind("\\") + 1);
	m_filename = filename;

	if (pConverter) pConverter->Release();
	if (pSource) pSource->Release();
	if (pDecoder) pDecoder->Release();
	if (pIWICFactory) pIWICFactory->Release();

	return true;
}

std::string Texture::ShowOpenDialog(HWND hWnd) {
	OPENFILENAME ofn;
	char szFileName[MAX_PATH] = "";
	strcpy(szFileName, m_filename.c_str());

	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn); // SEE NOTE BELOW
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = "Image Files (*jpg;*.png)\0*jpg;*.png\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
	//ofn.lpstrDefExt = "png";

	if (GetOpenFileName(&ofn)) {
		return std::string(szFileName);
	}
	return "\0";
}

std::string Texture::GetFilename() const {
	return m_filename;
}