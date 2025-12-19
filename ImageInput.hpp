#pragma once

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <vector>
#include <fstream>
#include <filesystem>

class ImageInput {
    private:
        const int PARTICLE_COUNT;

        int clampi(int v, int lo, int hi) {
            return std::max(lo, std::min(v, hi));
        }

        sf::Image scaleNearest(const sf::Image& src, unsigned dstW, unsigned dstH) {
            sf::Image dst;
            dst.create(dstW, dstH);

            const unsigned srcW = src.getSize().x;
            const unsigned srcH = src.getSize().y;

            for (unsigned y = 0; y < dstH; ++y) {
                const unsigned sy = (srcH * y) / dstH;
                for (unsigned x = 0; x < dstW; ++x) {
                    const unsigned sx = (srcW * x) / dstW;
                    dst.setPixel(x, y, src.getPixel(sx, sy));
                }
            }

            return dst;
        }

        bool findAssetImagePath(std::string& out) const {
            namespace fs = std::filesystem;

            static const char* candidates[] = {
                "../assets/image.png",
                "../assets/image.jpg",
                "../assets/image.jpeg",
                "../assets/image.bmp",
                "../assets/image.tga",
                "assets/image.png",
                "assets/image.jpg",
                "assets/image.jpeg",
                "assets/image.bmp",
                "assets/image.tga"
            };

            for (const char* p : candidates) {
                fs::path fp(p);
                if (fs::exists(fp) && fs::is_regular_file(fp)) {
                    out = fp.string();
                    return true;
                }
            }
            return false;
        }

    public:
        std::vector<sf::Color> targetColors;
        bool haveTargetColors = false;

        ImageInput(const int particleCount) : PARTICLE_COUNT(particleCount) {}

        void initTargetColorsIfAvailable() {
            namespace fs = std::filesystem;

            std::string imgPath;
            if (!findAssetImagePath(imgPath)) return;
            if (!fs::exists("output.txt") || !fs::is_regular_file("output.txt")) return;

            sf::Image src;
            if (!src.loadFromFile(imgPath)) return;

            sf::Image resized;
            if (src.getSize().x == (unsigned)SCREEN_WIDTH && src.getSize().y == (unsigned)SCREEN_HEIGHT) {
                resized = src;
            } else {
                resized = scaleNearest(src, (unsigned)SCREEN_WIDTH, (unsigned)SCREEN_HEIGHT);
            }

            std::ifstream in("output.txt");
            if (!in) return;

            const int W = (int)resized.getSize().x;
            const int H = (int)resized.getSize().y;

            targetColors.clear();
            targetColors.reserve((std::size_t)PARTICLE_COUNT);

            float x, y;
            while (targetColors.size() < (std::size_t)PARTICLE_COUNT && (in >> x >> y)) {
                int px = clampi((int)std::lround(x), 0, W - 1);
                int py = clampi((int)std::lround(y), 0, H - 1);
                targetColors.push_back(resized.getPixel((unsigned)px, (unsigned)py));
            }

            haveTargetColors = !targetColors.empty();
        }
};
