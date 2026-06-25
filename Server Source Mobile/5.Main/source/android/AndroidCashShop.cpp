#include "stdafx.h"

#ifdef __ANDROID__

#include "android/AndroidCashShop.h"

#include "CBInterface.h"
#include "NewUIBase.h"
#include "NewUIInventoryCtrl.h"
#include "NewUIMessageBox.h"
#include "NewUIMyInventory.h"
#include "NewUISystem.h"
#include "ZzzBMD.h"
#include "ZzzInfomation.h"
#include "ZzzInterface.h"
#include "ZzzInventory.h"
#include "ZzzOpenglUtil.h"
#include "wsclientinline.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace
{
constexpr int kProductsPerPage = 6;
constexpr float kPanelX = 34.0f;
constexpr float kPanelY = 31.0f;
constexpr float kPanelW = 572.0f;
constexpr float kPanelH = 402.0f;
constexpr GLuint kCategoryAtlasBitmap = 150900;
constexpr GLuint kTitlePlaqueBitmap = 150901;
constexpr GLuint kCloseButtonBitmap = 150902;

std::vector<std::string> Split(const std::string& value, char delimiter)
{
    std::vector<std::string> parts;
    std::stringstream stream(value);
    std::string part;
    while (std::getline(stream, part, delimiter))
    {
        parts.push_back(part);
    }
    if (!value.empty() && value.back() == delimiter)
    {
        parts.emplace_back();
    }
    return parts;
}

int ToInt(const std::vector<std::string>& fields, std::size_t index)
{
    return index < fields.size() ? std::atoi(fields[index].c_str()) : 0;
}

bool IsInside(float x, float y, const AndroidCashShop::Rect& rect)
{
    return x >= rect.x && y >= rect.y && x <= (rect.x + rect.w) && y <= (rect.y + rect.h);
}

std::uint32_t NowMs()
{
    return static_cast<std::uint32_t>(GetTickCount());
}

void DrawPanel(float x, float y, float w, float h, float r, float g, float b, float a)
{
    gInterface.DrawBarForm(x, y, w, h, r, g, b, a);
}

void DrawOutline(float x, float y, float w, float h, float r, float g, float b, float a, float thickness = 1.0f)
{
    DrawPanel(x, y, w, thickness, r, g, b, a);
    DrawPanel(x, y + h - thickness, w, thickness, r, g, b, a);
    DrawPanel(x, y, thickness, h, r, g, b, a);
    DrawPanel(x + w - thickness, y, thickness, h, r, g, b, a);
}

void DrawClassicFrame(float x, float y, float w, float h)
{
    using SEASON3B::CNewUIInventoryCtrl;
    const DWORD tint = RGBA(220, 174, 96, 255);
    SEASON3B::RenderImage(CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_TOP_LEFT, x, y, 14.0f, 14.0f, 0, 0, tint);
    SEASON3B::RenderImage(CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_TOP_RIGHT, x + w - 14.0f, y, 14.0f, 14.0f, 0, 0, tint);
    SEASON3B::RenderImage(CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_BOTTOM_LEFT, x, y + h - 14.0f, 14.0f, 14.0f, 0, 0, tint);
    SEASON3B::RenderImage(CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_BOTTOM_RIGHT, x + w - 14.0f, y + h - 14.0f, 14.0f, 14.0f, 0, 0, tint);
    SEASON3B::RenderImage(CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_TOP_PIXEL, x + 10.0f, y, w - 20.0f, 14.0f, 0, 0, tint);
    SEASON3B::RenderImage(CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_BOTTOM_PIXEL, x + 10.0f, y + h - 14.0f, w - 20.0f, 14.0f, 0, 0, tint);
    SEASON3B::RenderImage(CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_LEFT_PIXEL, x, y + 10.0f, 14.0f, h - 20.0f, 0, 0, tint);
    SEASON3B::RenderImage(CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_RIGHT_PIXEL, x + w - 14.0f, y + 10.0f, 14.0f, h - 20.0f, 0, 0, tint);
}

void DrawCategoryIcon(int categoryIndex, float x, float y, float size)
{
    static constexpr int atlasIndex[] = {0, 1, 5, 3, 4, 2};
    const int index = categoryIndex >= 0 && categoryIndex < static_cast<int>(std::size(atlasIndex))
        ? atlasIndex[categoryIndex]
        : 0;
    const int column = index % 3;
    const int row = index / 3;
    SEASON3B::RenderImage(kCategoryAtlasBitmap, x, y, size, size,
        column * 0.25f, row * 0.5f, 0.25f, 0.5f);
}

std::string FormatAmount(double value)
{
    const unsigned long long amount = value > 0.0 ? static_cast<unsigned long long>(value) : 0ULL;
    std::string text = std::to_string(amount);
    for (int position = static_cast<int>(text.size()) - 3; position > 0; position -= 3)
    {
        text.insert(static_cast<std::size_t>(position), ".");
    }
    return text;
}

const char* CoinName(int coinIndex)
{
    switch (coinIndex)
    {
    case 509:
        return "WCoin P";
    case 0:
        return "Goblin";
    default:
        return "WCoin";
    }
}

const char* CategoryName(int index, const std::string& fallback)
{
    static constexpr const char* names[] = {
        "DESTAQUES",
        "EXP",
        "ACESSORIOS",
        "PET",
        "EVENTOS",
        "SUPORTE",
    };
    return index >= 0 && index < static_cast<int>(std::size(names)) ? names[index] : fallback.c_str();
}

void DrawText(
    HFONT font,
    float x,
    float y,
    std::uint32_t color,
    int width,
    int align,
    const char* format,
    ...)
{
    char text[1024] = {};
    va_list args;
    va_start(args, format);
    vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    TextDraw(font, static_cast<int>(x), static_cast<int>(y), color, 0, width, 0, align, "%s", text);
}

#pragma pack(push, 1)
struct CashPointPacket
{
    unsigned char header[4];
    unsigned char result;
    double wCoinC[2];
    double wCoinP[2];
    double goblinPoint;
};

struct ResultPacket
{
    unsigned char header[4];
    unsigned char result;
};
#pragma pack(pop)
}

AndroidCashShop& AndroidCashShop::Instance()
{
    static AndroidCashShop instance;
    return instance;
}

void AndroidCashShop::RequestToggle()
{
    if (m_visible || m_waitingOpen)
    {
        Close(true);
        return;
    }

    if (!LoadCatalog())
    {
        SetStatus("Catalogo da Loja X nao encontrado.", 4000);
        m_visible = true;
        return;
    }

    SendRequestIGS_CashShopOpen(0);
    m_waitingOpen = true;
    SetStatus("Abrindo Loja X...", 3000);
}

void AndroidCashShop::Close(bool notifyServer)
{
    if (notifyServer && (m_visible || m_waitingOpen))
    {
        SendRequestIGS_CashShopOpen(1);
    }
    m_visible = false;
    m_waitingOpen = false;
    m_waitingPurchase = false;
    m_confirmPurchase = false;
}

void AndroidCashShop::RequestBalance()
{
    SendRequestIGS_CashPointInfo();
}

void AndroidCashShop::RequestPurchase()
{
    const Product* product = GetSelectedProduct();
    if (product == nullptr || m_waitingPurchase)
    {
        return;
    }

    CStreamPacketEngine packet;
    packet.Init(0xC1, 0xD2);
    packet << static_cast<BYTE>(0x03);
    packet << static_cast<DWORD>(product->packageIndex);
    packet << static_cast<DWORD>(product->category);
    packet << static_cast<DWORD>(product->productIndex);
    packet << static_cast<WORD>(product->itemIndex);
    packet << static_cast<UINT>(product->coinIndex);
    packet << static_cast<BYTE>(0);
    packet.Send();
    m_waitingPurchase = true;
    m_confirmPurchase = false;
    SetStatus("Processando compra...", 5000);
}

void AndroidCashShop::HandlePacket(const unsigned char* packet, std::size_t size)
{
    if (packet == nullptr || size < 4 || packet[0] != 0xC1 || packet[2] != 0xD2)
    {
        return;
    }

    switch (packet[3])
    {
    case 0x00:
        break;
    case 0x01:
        if (size >= sizeof(CashPointPacket))
        {
            const auto* message = reinterpret_cast<const CashPointPacket*>(packet);
            if (message->result == 0)
            {
                m_wCoinC = message->wCoinC[0];
                m_wCoinP = message->wCoinP[0];
                m_goblinPoint = message->goblinPoint;
            }
        }
        break;
    case 0x02:
        if (size >= sizeof(ResultPacket))
        {
            const auto* message = reinterpret_cast<const ResultPacket*>(packet);
            m_waitingOpen = false;
            if (message->result == 1)
            {
                m_visible = true;
                m_page = 0;
                m_selectedProduct = -1;
                RequestBalance();
                SetStatus("Loja X aberta.", 1800);
            }
            else
            {
                m_visible = false;
                SetStatus("Nao foi possivel abrir a loja nesta area.", 4000);
            }
        }
        break;
    case 0x03:
        if (size >= sizeof(ResultPacket))
        {
            const auto* message = reinterpret_cast<const ResultPacket*>(packet);
            m_waitingPurchase = false;
            if (message->result == 0)
            {
                SetStatus("Compra concluida. Item enviado ao armazenamento.", 4500);
                RequestBalance();
            }
            else
            {
                SetStatus("Compra recusada. Verifique saldo e armazenamento.", 4500);
            }
        }
        break;
    default:
        break;
    }
}

bool AndroidCashShop::LoadCatalog()
{
    if (m_catalogLoaded)
    {
        return true;
    }

    const std::array<std::string, 3> roots = {
        "./Data/InGameShopScript/512.2011.006/",
        "./Data/InGameShopScript/512.2011.6/",
        "./Data/InGameShopScript/",
    };

    std::string categoryPath;
    std::string packagePath;
    for (const std::string& root : roots)
    {
        std::ifstream categoryTest(root + "IBSCategory.txt");
        std::ifstream packageTest(root + "IBSPackage.txt");
        if (categoryTest.good() && packageTest.good())
        {
            categoryPath = root + "IBSCategory.txt";
            packagePath = root + "IBSPackage.txt";
            break;
        }
    }

    if (categoryPath.empty())
    {
        return false;
    }

    std::ifstream categoryFile(categoryPath);
    std::string line;
    while (std::getline(categoryFile, line))
    {
        if (line.empty() || line[0] == '/' || line == "end")
        {
            continue;
        }
        const std::vector<std::string> fields = Split(line, '@');
        if (fields.size() < 7 || ToInt(fields, 3) != 201 || ToInt(fields, 6) != 0)
        {
            continue;
        }
        Category category;
        category.id = ToInt(fields, 0);
        category.name = fields[1];
        category.order = ToInt(fields, 5);
        m_categories.push_back(std::move(category));
    }

    std::sort(m_categories.begin(), m_categories.end(), [](const Category& left, const Category& right)
    {
        return left.order < right.order;
    });

    std::ifstream packageFile(packagePath);
    while (std::getline(packageFile, line))
    {
        if (line.empty() || line[0] == '/' || line == "end")
        {
            continue;
        }
        const std::vector<std::string> fields = Split(line, '@');
        if (fields.size() < 27 || ToInt(fields, 8) != 182 || ToInt(fields, 16) != 181)
        {
            continue;
        }
        Product product;
        product.category = ToInt(fields, 0);
        product.viewOrder = ToInt(fields, 1);
        product.packageIndex = ToInt(fields, 2);
        product.name = fields[3];
        product.price = ToInt(fields, 5);
        product.description = fields[6];
        product.productIndex = ToInt(Split(fields[19], '|'), 0);
        product.itemIndex = ToInt(fields, 20);
        product.coinIndex = ToInt(fields, 25);
        product.cashType = ToInt(fields, 25);
        product.priceIndex = ToInt(Split(fields[23], '|'), 0);
        m_products.push_back(std::move(product));
    }

    std::sort(m_products.begin(), m_products.end(), [](const Product& left, const Product& right)
    {
        if (left.category != right.category)
        {
            return left.category < right.category;
        }
        return left.viewOrder < right.viewOrder;
    });

    m_categories.erase(
        std::remove_if(m_categories.begin(), m_categories.end(), [this](const Category& category)
        {
            return std::none_of(m_products.begin(), m_products.end(), [&category](const Product& product)
            {
                return product.category == category.id;
            });
        }),
        m_categories.end());

    if (m_categories.empty() || m_products.empty())
    {
        m_categories.clear();
        m_products.clear();
        return false;
    }

    m_selectedCategory = 0;
    LoadBitmap("Interface\\AndroidCashShop\\category_atlas.tga", kCategoryAtlasBitmap, GL_LINEAR, GL_CLAMP_TO_EDGE);
    LoadBitmap("Interface\\AndroidCashShop\\title_plaque.tga", kTitlePlaqueBitmap, GL_LINEAR, GL_CLAMP_TO_EDGE);
    LoadBitmap("Interface\\AndroidCashShop\\close_button.tga", kCloseButtonBitmap, GL_LINEAR, GL_CLAMP_TO_EDGE);
    m_catalogLoaded = true;
    return true;
}

const std::vector<const AndroidCashShop::Product*> AndroidCashShop::GetVisibleProducts() const
{
    std::vector<const Product*> result;
    if (m_selectedCategory < 0 || m_selectedCategory >= static_cast<int>(m_categories.size()))
    {
        return result;
    }
    const int categoryId = m_categories[m_selectedCategory].id;
    for (const Product& product : m_products)
    {
        if (product.category == categoryId)
        {
            result.push_back(&product);
        }
    }
    return result;
}

const AndroidCashShop::Product* AndroidCashShop::GetSelectedProduct() const
{
    const std::vector<const Product*> products = GetVisibleProducts();
    return m_selectedProduct >= 0 && m_selectedProduct < static_cast<int>(products.size())
        ? products[m_selectedProduct]
        : nullptr;
}

AndroidCashShop::Rect AndroidCashShop::GetPanelRect() const { return {kPanelX, kPanelY, kPanelW, kPanelH}; }
AndroidCashShop::Rect AndroidCashShop::GetCloseRect() const { return {kPanelX + kPanelW - 40.0f, kPanelY + 8.0f, 30.0f, 30.0f}; }
AndroidCashShop::Rect AndroidCashShop::GetCategoryRect(int index) const { return {kPanelX + 10.0f, kPanelY + 91.0f + index * 45.0f, 91.0f, 39.0f}; }
AndroidCashShop::Rect AndroidCashShop::GetProductRect(int index) const
{
    const int column = index % 3;
    const int row = index / 3;
    return {kPanelX + 111.0f + column * 105.0f, kPanelY + 91.0f + row * 128.0f, 99.0f, 120.0f};
}
AndroidCashShop::Rect AndroidCashShop::GetPreviousPageRect() const { return {kPanelX + 205.0f, kPanelY + kPanelH - 31.0f, 30.0f, 22.0f}; }
AndroidCashShop::Rect AndroidCashShop::GetNextPageRect() const { return {kPanelX + 322.0f, kPanelY + kPanelH - 31.0f, 30.0f, 22.0f}; }
AndroidCashShop::Rect AndroidCashShop::GetBuyRect() const { return {kPanelX + 443.0f, kPanelY + 365.0f, 105.0f, 28.0f}; }
AndroidCashShop::Rect AndroidCashShop::GetConfirmRect() const { return {kPanelX + 273.0f, kPanelY + 244.0f, 92.0f, 30.0f}; }
AndroidCashShop::Rect AndroidCashShop::GetCancelRect() const { return {kPanelX + 376.0f, kPanelY + 244.0f, 92.0f, 30.0f}; }

void AndroidCashShop::SetStatus(const char* text, std::uint32_t durationMs)
{
    m_status = text != nullptr ? text : "";
    m_statusUntil = NowMs() + durationMs;
}

void AndroidCashShop::Render()
{
    if (!m_visible)
    {
        return;
    }

    const Rect panel = GetPanelRect();
    const HFONT normalFont = g_hFont != nullptr ? g_hFont : g_hFontBold;
    const HFONT boldFont = g_hFontBold != nullptr ? g_hFontBold : normalFont;
    const std::vector<const Product*> products = GetVisibleProducts();
    const int pageCount = (std::max)(1, static_cast<int>((products.size() + kProductsPerPage - 1) / kProductsPerPage));
    m_page = std::clamp(m_page, 0, pageCount - 1);
    const Product* selected = GetSelectedProduct();

    BeginBitmap();
    EnableAlphaBlend();
    DrawPanel(0, 0, 640, 480, 0, 0, 0, 0.42f);
    DrawPanel(panel.x, panel.y, panel.w, panel.h, 0.018f, 0.018f, 0.016f, 0.985f);
    DrawOutline(panel.x, panel.y, panel.w, panel.h, 0.34f, 0.27f, 0.16f, 1.0f, 3.0f);
    DrawOutline(panel.x + 5, panel.y + 5, panel.w - 10, panel.h - 10, 0.68f, 0.54f, 0.28f, 0.82f);
    DrawClassicFrame(panel.x + 3, panel.y + 3, panel.w - 6, panel.h - 6);

    DrawPanel(panel.x + 7, panel.y + 7, panel.w - 14, 48, 0.045f, 0.043f, 0.036f, 1.0f);
    DrawPanel(panel.x + 8, panel.y + 53, panel.w - 16, 2, 0.68f, 0.48f, 0.16f, 1.0f);
    SEASON3B::RenderImage(kTitlePlaqueBitmap, panel.x + 88, panel.y + 3, 396.0f, 60.0f,
        0.0f, 0.0f, 1.0f, 1.0f);
    DrawText(boldFont, panel.x + 196, panel.y + 23, 0xFFE09AFF, 180, 3, "LOJA X");

    const Rect close = GetCloseRect();
    SEASON3B::RenderImage(kCloseButtonBitmap, close.x, close.y, close.w, close.h,
        0.0f, 0.0f, 1.0f, 1.0f);

    const float balanceY = panel.y + 59.0f;
    const float balanceX[] = {panel.x + 111.0f, panel.x + 255.0f, panel.x + 399.0f};
    const char* balanceNames[] = {"WCoin", "WCoin P", "Goblin"};
    const std::string balanceValues[] = {FormatAmount(m_wCoinC), FormatAmount(m_wCoinP), FormatAmount(m_goblinPoint)};
    const std::uint32_t balanceColors[] = {0xFFD060FF, 0xD89AFFFF, 0xA8D95CFF};
    for (int index = 0; index < 3; ++index)
    {
        DrawPanel(balanceX[index], balanceY, 134, 24, 0.035f, 0.035f, 0.033f, 1.0f);
        DrawOutline(balanceX[index], balanceY, 134, 24, 0.34f, 0.30f, 0.22f, 1.0f);
        DrawPanel(balanceX[index] + 5, balanceY + 5, 14, 14,
            index == 0 ? 0.56f : (index == 1 ? 0.38f : 0.25f),
            index == 0 ? 0.31f : (index == 1 ? 0.19f : 0.43f),
            index == 0 ? 0.03f : (index == 1 ? 0.48f : 0.08f), 1.0f);
        DrawText(boldFont, balanceX[index] + 5, balanceY + 8, 0xFFF2CEFF, 14, 3,
            index == 2 ? "G" : (index == 1 ? "P" : "W"));
        DrawText(boldFont, balanceX[index] + 24, balanceY + 8, balanceColors[index], 49, 1, "%s", balanceNames[index]);
        DrawText(boldFont, balanceX[index] + 73, balanceY + 8, 0xF0E8DCFF, 55, 3, "%s", balanceValues[index].c_str());
    }

    DrawPanel(panel.x + 8, panel.y + 86, 97, 279, 0.032f, 0.032f, 0.03f, 1.0f);
    DrawOutline(panel.x + 8, panel.y + 86, 97, 279, 0.38f, 0.31f, 0.19f, 1.0f);
    const int categoryCount = (std::min)(6, static_cast<int>(m_categories.size()));
    for (int index = 0; index < categoryCount; ++index)
    {
        const Rect rect = GetCategoryRect(index);
        const bool selected = index == m_selectedCategory;
        DrawPanel(rect.x, rect.y, rect.w, rect.h,
            selected ? 0.18f : 0.055f,
            selected ? 0.075f : 0.052f,
            selected ? 0.025f : 0.047f,
            1.0f);
        DrawOutline(rect.x, rect.y, rect.w, rect.h,
            selected ? 0.93f : 0.30f, selected ? 0.65f : 0.27f, selected ? 0.20f : 0.20f, 1.0f);
        DrawPanel(rect.x + 7, rect.y + 7, 25, 25,
            selected ? 0.48f : 0.13f, selected ? 0.27f : 0.12f, selected ? 0.05f : 0.10f, 1.0f);
        DrawOutline(rect.x + 7, rect.y + 7, 25, 25, 0.54f, 0.42f, 0.22f, 1.0f);
        DrawCategoryIcon(index, rect.x + 7.5f, rect.y + 7.5f, 24.0f);
        if (selected)
        {
            DrawPanel(rect.x, rect.y, 3, rect.h, 0.96f, 0.65f, 0.16f, 1.0f);
        }
        DrawText(boldFont, rect.x + 35, rect.y + 15, selected ? 0xFFE1A1FF : 0xD7C19EFF,
            static_cast<int>(rect.w - 39), 3, "%s", CategoryName(index, m_categories[index].name));
    }

    DrawPanel(panel.x + 108, panel.y + 86, 320, 279, 0.018f, 0.019f, 0.019f, 0.98f);
    DrawOutline(panel.x + 108, panel.y + 86, 320, 279, 0.28f, 0.24f, 0.16f, 1.0f);
    for (int slot = 0; slot < kProductsPerPage; ++slot)
    {
        const int productIndex = m_page * kProductsPerPage + slot;
        if (productIndex >= static_cast<int>(products.size()))
        {
            break;
        }
        const Rect rect = GetProductRect(slot);
        const bool selected = productIndex == m_selectedProduct;
        DrawPanel(rect.x, rect.y, rect.w, rect.h,
            selected ? 0.105f : 0.04f,
            selected ? 0.075f : 0.042f,
            selected ? 0.025f : 0.042f,
            1.0f);
        DrawClassicFrame(rect.x, rect.y, rect.w, rect.h);
        if (selected)
        {
            DrawOutline(rect.x + 2, rect.y + 2, rect.w - 4, rect.h - 4, 0.96f, 0.67f, 0.16f, 1.0f, 2.0f);
        }
        DrawPanel(rect.x + 3, rect.y + 3, rect.w - 6, 17, 0.025f, 0.025f, 0.024f, 0.96f);
        DrawText(boldFont, rect.x + 5, rect.y + 7, 0xF1E9DDFF, static_cast<int>(rect.w - 10), 3,
            "%s", products[productIndex]->name.c_str());
        DrawPanel(rect.x + 3, rect.y + 96, rect.w - 6, 21, 0.027f, 0.026f, 0.022f, 1.0f);
        DrawPanel(rect.x + 9, rect.y + 100, 14, 14, 0.56f, 0.31f, 0.03f, 1.0f);
        DrawText(boldFont, rect.x + 9, rect.y + 103, 0xFFF2CEFF, 14, 3,
            products[productIndex]->coinIndex == 0 ? "G" : (products[productIndex]->coinIndex == 509 ? "P" : "W"));
        DrawText(boldFont, rect.x + 25, rect.y + 103, 0xFFD16CFF, static_cast<int>(rect.w - 30), 3,
            "%s", FormatAmount(products[productIndex]->price).c_str());
    }

    DrawPanel(panel.x + 434, panel.y + 86, 130, 279, 0.025f, 0.025f, 0.023f, 1.0f);
    DrawClassicFrame(panel.x + 434, panel.y + 86, 130, 279);
    if (selected != nullptr)
    {
        DrawText(boldFont, panel.x + 440, panel.y + 96, 0xFFD15DFF, 118, 3, "%s", selected->name.c_str());
        DrawPanel(panel.x + 445, panel.y + 116, 108, 98, 0.035f, 0.035f, 0.033f, 1.0f);
        DrawOutline(panel.x + 445, panel.y + 116, 108, 98, 0.78f, 0.52f, 0.16f, 1.0f, 2.0f);
        DrawText(normalFont, panel.x + 443, panel.y + 223, 0xE3DDD2FF, 112, 3, "%s", selected->description.c_str());
        DrawPanel(panel.x + 445, panel.y + 270, 108, 1, 0.45f, 0.34f, 0.16f, 1.0f);
        DrawText(normalFont, panel.x + 445, panel.y + 280, 0xD8C28EFF, 108, 3, "Quantidade");
        DrawPanel(panel.x + 466, panel.y + 294, 66, 23, 0.045f, 0.043f, 0.038f, 1.0f);
        DrawOutline(panel.x + 466, panel.y + 294, 66, 23, 0.34f, 0.29f, 0.19f, 1.0f);
        DrawText(boldFont, panel.x + 466, panel.y + 302, 0xFFFFFFFF, 66, 3, "1");
        DrawText(normalFont, panel.x + 445, panel.y + 322, 0xD8C28EFF, 108, 3, "Preco");
        DrawText(boldFont, panel.x + 445, panel.y + 337, 0xFFD060FF, 108, 3, "%s %s",
            FormatAmount(selected->price).c_str(), CoinName(selected->coinIndex));
        const Rect buy = GetBuyRect();
        SEASON3B::RenderImage(SEASON3B::CNewUIMessageBoxMng::IMAGE_MSGBOX_BTN_EMPTY_BIG,
            buy.x, buy.y, buy.w, buy.h, 0.0f, 2.0f / 3.0f, 1.0f, 1.0f / 3.0f);
        DrawText(boldFont, buy.x, buy.y + 11, 0xFFF3D4FF, static_cast<int>(buy.w), 3, "COMPRAR");
    }
    else
    {
        DrawText(boldFont, panel.x + 440, panel.y + 99, 0xD8C28EFF, 118, 3, "DETALHES");
        DrawText(normalFont, panel.x + 443, panel.y + 222, 0xAFA99FFF, 112, 3, "Selecione um produto");
    }

    const Rect previous = GetPreviousPageRect();
    const Rect next = GetNextPageRect();
    DrawPanel(previous.x, previous.y, previous.w, previous.h, 0.08f, 0.075f, 0.062f, 1.0f);
    DrawPanel(next.x, next.y, next.w, next.h, 0.08f, 0.075f, 0.062f, 1.0f);
    DrawOutline(previous.x, previous.y, previous.w, previous.h, 0.52f, 0.40f, 0.20f, 1.0f);
    DrawOutline(next.x, next.y, next.w, next.h, 0.52f, 0.40f, 0.20f, 1.0f);
    DrawText(boldFont, previous.x, previous.y + 7, 0xE6D6B8FF, static_cast<int>(previous.w), 3, "<");
    DrawText(boldFont, next.x, next.y + 7, 0xE6D6B8FF, static_cast<int>(next.w), 3, ">");
    DrawText(boldFont, panel.x + 242, panel.y + panel.h - 24, 0xE6D6B8FF, 73, 3, "%d / %d", m_page + 1, pageCount);

    if (!m_status.empty() && NowMs() <= m_statusUntil)
    {
        DrawPanel(panel.x + 112, panel.y + 366, 316, 21, 0.025f, 0.024f, 0.021f, 0.96f);
        DrawOutline(panel.x + 112, panel.y + 366, 316, 21, 0.33f, 0.27f, 0.16f, 1.0f);
        DrawText(normalFont, panel.x + 116, panel.y + 373, 0xFFE38AFF, 308, 3, "%s", m_status.c_str());
    }
    EndBitmap();

    bool hasRenderableItem = false;
    for (int slot = 0; slot < kProductsPerPage; ++slot)
    {
        const int productIndex = m_page * kProductsPerPage + slot;
        if (productIndex < static_cast<int>(products.size()) && products[productIndex]->itemIndex > 0)
        {
            hasRenderableItem = true;
            break;
        }
    }
    if (hasRenderableItem)
    {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glViewport2(0, 0, WindowWidth, WindowHeight);
        gluPerspective2(1.0f, static_cast<float>(WindowWidth) / static_cast<float>(WindowHeight),
            RENDER_ITEMVIEW_NEAR, RENDER_ITEMVIEW_FAR);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        GetOpenGLMatrix(CameraMatrix);
        EnableDepthTest();
        EnableDepthMask();
        glDisable(GL_BLEND);
        glClear(GL_DEPTH_BUFFER_BIT);
        for (int slot = 0; slot < kProductsPerPage; ++slot)
        {
            const int productIndex = m_page * kProductsPerPage + slot;
            if (productIndex >= static_cast<int>(products.size()))
            {
                break;
            }
            const Rect rect = GetProductRect(slot);
            RenderItem3D(rect.x + 20, rect.y + 23, rect.w - 40, 69, products[productIndex]->itemIndex, 0, 0, 0);
        }
        if (selected != nullptr && selected->itemIndex > 0)
        {
            RenderItem3D(panel.x + 459, panel.y + 124, 80, 82, selected->itemIndex, 0, 0, 0);
        }
        UpdateMousePositionn();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
    }

    BeginBitmap();
    EnableAlphaBlend();
    if (m_confirmPurchase && selected != nullptr)
    {
        DrawPanel(panel.x + 184, panel.y + 157, 304, 135, 0.018f, 0.018f, 0.016f, 0.995f);
        DrawOutline(panel.x + 184, panel.y + 157, 304, 135, 0.78f, 0.55f, 0.18f, 1.0f, 2.0f);
        DrawPanel(panel.x + 188, panel.y + 161, 296, 30, 0.18f, 0.10f, 0.025f, 1.0f);
        DrawText(boldFont, panel.x + 188, panel.y + 172, 0xFFE5A4FF, 296, 3, "CONFIRMAR COMPRA");
        DrawText(normalFont, panel.x + 197, panel.y + 207, 0xFFFFFFFF, 278, 3,
            "%s por %s %s?", selected->name.c_str(), FormatAmount(selected->price).c_str(), CoinName(selected->coinIndex));
        const Rect confirm = GetConfirmRect();
        const Rect cancel = GetCancelRect();
        DrawPanel(confirm.x, confirm.y, confirm.w, confirm.h, 0.58f, 0.31f, 0.04f, 1.0f);
        DrawOutline(confirm.x, confirm.y, confirm.w, confirm.h, 1.0f, 0.72f, 0.20f, 1.0f);
        DrawPanel(cancel.x, cancel.y, cancel.w, cancel.h, 0.12f, 0.12f, 0.11f, 1.0f);
        DrawOutline(cancel.x, cancel.y, cancel.w, cancel.h, 0.42f, 0.37f, 0.28f, 1.0f);
        DrawText(boldFont, confirm.x, confirm.y + 10, 0xFFFFFFFF, static_cast<int>(confirm.w), 3, "CONFIRMAR");
        DrawText(boldFont, cancel.x, cancel.y + 10, 0xFFFFFFFF, static_cast<int>(cancel.w), 3, "CANCELAR");
    }
    EndBitmap();
}

bool AndroidCashShop::HandleTap(float x, float y)
{
    if (!m_visible)
    {
        return false;
    }

    if (m_confirmPurchase)
    {
        if (IsInside(x, y, GetConfirmRect()))
        {
            RequestPurchase();
        }
        else if (IsInside(x, y, GetCancelRect()))
        {
            m_confirmPurchase = false;
        }
        return true;
    }

    if (IsInside(x, y, GetCloseRect()))
    {
        Close(true);
        return true;
    }

    const int categoryCount = (std::min)(7, static_cast<int>(m_categories.size()));
    for (int index = 0; index < categoryCount; ++index)
    {
        if (IsInside(x, y, GetCategoryRect(index)))
        {
            m_selectedCategory = index;
            m_selectedProduct = -1;
            m_page = 0;
            PlayBuffer(SOUND_CLICK01);
            return true;
        }
    }

    const std::vector<const Product*> products = GetVisibleProducts();
    for (int slot = 0; slot < kProductsPerPage; ++slot)
    {
        const int productIndex = m_page * kProductsPerPage + slot;
        if (productIndex >= static_cast<int>(products.size()))
        {
            break;
        }
        if (IsInside(x, y, GetProductRect(slot)))
        {
            m_selectedProduct = productIndex;
            PlayBuffer(SOUND_CLICK01);
            return true;
        }
    }

    if (GetSelectedProduct() != nullptr && IsInside(x, y, GetBuyRect()))
    {
        m_confirmPurchase = true;
        return true;
    }

    const int pageCount = (std::max)(1, static_cast<int>((products.size() + kProductsPerPage - 1) / kProductsPerPage));
    if (IsInside(x, y, GetPreviousPageRect()))
    {
        m_page = (std::max)(0, m_page - 1);
        m_selectedProduct = -1;
        return true;
    }
    if (IsInside(x, y, GetNextPageRect()))
    {
        m_page = (std::min)(pageCount - 1, m_page + 1);
        m_selectedProduct = -1;
        return true;
    }
    return IsInside(x, y, GetPanelRect()) || m_visible;
}

extern "C" void AndroidCashShopHandlePacket(const unsigned char* packet, int size)
{
    AndroidCashShop::Instance().HandlePacket(packet, size > 0 ? static_cast<std::size_t>(size) : 0);
}

#endif
