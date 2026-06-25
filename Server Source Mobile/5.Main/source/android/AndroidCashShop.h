#pragma once

#ifdef __ANDROID__

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class AndroidCashShop
{
public:
    struct Rect
    {
        float x;
        float y;
        float w;
        float h;
    };

    static AndroidCashShop& Instance();

    void RequestToggle();
    void HandlePacket(const unsigned char* packet, std::size_t size);
    void Render();
    bool HandleTap(float x, float y);

    bool IsVisible() const { return m_visible; }

private:
    struct Category
    {
        int id = 0;
        int order = 0;
        std::string name;
    };

    struct Product
    {
        int category = 0;
        int viewOrder = 0;
        int packageIndex = 0;
        int productIndex = 0;
        int priceIndex = 0;
        int itemIndex = 0;
        int coinIndex = 0;
        int cashType = 0;
        int price = 0;
        std::string name;
        std::string description;
    };

    AndroidCashShop() = default;

    bool LoadCatalog();
    void Close(bool notifyServer);
    void RequestBalance();
    void RequestPurchase();
    void SetStatus(const char* text, std::uint32_t durationMs);

    const std::vector<const Product*> GetVisibleProducts() const;
    const Product* GetSelectedProduct() const;
    Rect GetPanelRect() const;
    Rect GetCloseRect() const;
    Rect GetCategoryRect(int index) const;
    Rect GetProductRect(int index) const;
    Rect GetPreviousPageRect() const;
    Rect GetNextPageRect() const;
    Rect GetBuyRect() const;
    Rect GetConfirmRect() const;
    Rect GetCancelRect() const;

    bool m_visible = false;
    bool m_waitingOpen = false;
    bool m_waitingPurchase = false;
    bool m_confirmPurchase = false;
    bool m_catalogLoaded = false;
    int m_selectedCategory = 0;
    int m_selectedProduct = -1;
    int m_page = 0;
    double m_wCoinC = 0.0;
    double m_wCoinP = 0.0;
    double m_goblinPoint = 0.0;
    std::uint32_t m_statusUntil = 0;
    std::string m_status;
    std::vector<Category> m_categories;
    std::vector<Product> m_products;
};

extern "C" void AndroidCashShopHandlePacket(const unsigned char* packet, int size);

#endif
