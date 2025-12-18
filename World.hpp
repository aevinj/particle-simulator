#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <string>
#include <iostream>
#include <algorithm>

#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>

#include <fstream>
#include <filesystem>

#include "Config.hpp"
#include "Particle.hpp"
#include "ParticleRenderer.hpp"

struct Slice {
    int start;
    int end;
};

class World {
private:
    const int PARTICLE_COUNT;
    const int SUBSTEPS;

    static constexpr int ndx[4] = { 1,  0,  1, -1 };
    static constexpr int ndy[4] = { 0,  1,  1,  1 };
    static const int CELL_SIZE = 4;
    static constexpr int GRID_COLS = (SCREEN_WIDTH  + CELL_SIZE - 1) / CELL_SIZE;
    static constexpr int GRID_ROWS = (SCREEN_HEIGHT + CELL_SIZE - 1) / CELL_SIZE;

    const float MOUSE_RADIUS   = 100.f;
    const float MOUSE_STRENGTH = 5000.f;
    const float SPAWN_DELAY    = 0.00005f;

    const sf::Vector2f startPos = {static_cast<float>(SCREEN_WIDTH) / 2.f, 10.f};
    sf::Vector2f startingVel = {0.f, 500.f};
    bool goingUp = true;

    const float dt = 1.f / 60.f;

    ParticleRenderer renderer = ParticleRenderer(PARTICLE_COUNT);

    static constexpr int CELL_CAP = 10;
    struct Cell {
        std::uint8_t count;
        int ids[CELL_CAP];
    };
    std::vector<Cell> grid;

    std::mutex mtx;
    std::condition_variable cvDone, cvWork;
    std::vector<std::thread> workers;
    std::atomic<bool> stop{false};
    std::vector<Slice> evenSlices, oddSlices;
    const std::vector<Slice>* currentSlices = nullptr;
    std::atomic<std::size_t> nextJob{0};
    std::atomic<int> remaining{0};
    uint64_t generation = 0;

    std::vector<sf::Color> targetColors;
    bool haveTargetColors = false;

    static int clampi(int v, int lo, int hi) {
        return std::max(lo, std::min(v, hi));
    }

    static sf::Image scaleNearest(const sf::Image& src, unsigned dstW, unsigned dstH) {
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

    void workerLoop() {
        uint64_t localGen = 0;

        while (true) {
            const std::vector<Slice>* slicesPtr = nullptr;

            {
                std::unique_lock<std::mutex> lock(mtx);
                cvWork.wait(lock, [this, &localGen] () {
                    return stop.load(std::memory_order_acquire) || generation != localGen;
                });

                if (stop.load(std::memory_order_acquire)) return;

                localGen = generation;
                slicesPtr = currentSlices;
            }

            for (;;) {
                size_t j = nextJob.fetch_add(1, std::memory_order_relaxed);
                if (j >= slicesPtr->size()) break;
                solveSlice((*slicesPtr)[j]);
            }

            if (remaining.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                std::lock_guard<std::mutex> lock(mtx);
                cvDone.notify_one();
            }
        }
    }

    void runPass(const std::vector<Slice> &slices) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            currentSlices = &slices;
            nextJob.store(0, std::memory_order_relaxed);
            remaining.store(static_cast<int>(workers.size()), std::memory_order_relaxed);
            generation++;
        }

        cvWork.notify_all();

        std::unique_lock<std::mutex> lock(mtx);
        cvDone.wait(lock, [this] () {
            return remaining.load(std::memory_order_relaxed) == 0;
        });
    }

    void buildSlices(int threadCount) {
        evenSlices.clear();
        oddSlices.clear();

        const int minSliceWidth = 2;
        const int maxSliceCount = GRID_COLS / minSliceWidth;

        int sliceCount = std::min(2 * threadCount, maxSliceCount);
        if (sliceCount < 2) sliceCount = 2;
        if (sliceCount % 2 == 1) --sliceCount;

        const int baseW = GRID_COLS / sliceCount;
        const int rem   = GRID_COLS % sliceCount;

        int x = 0;
        for (int s = 0; s < sliceCount; ++s) {
            int w = baseW + (s < rem ? 1 : 0);
            Slice sl{ x, x + w };
            x += w;

            if ((s & 1) == 0) evenSlices.push_back(sl);
            else              oddSlices.push_back(sl);
        }
    }

    void clearGrid() {
        for (auto &c : grid) c.count = 0;
    }

    void buildGrid() {
        clearGrid();

        for (int i = 0; i < static_cast<int>(particles.size()); ++i) {
            const auto& p = particles[i];
            int cx = static_cast<int>(p.position.x / CELL_SIZE);
            int cy = static_cast<int>(p.position.y / CELL_SIZE);
            if (!inBoundsCell(cx, cy)) continue;

            Cell &c = grid[cellIndex(cx, cy)];
            if (c.count < CELL_CAP) {
                c.ids[c.count++] = i;
            } else {
                c.ids[CELL_CAP - 1] = i;
            }
        }
    }

    inline bool inBoundsCell(int cx, int cy) const {
        return (cx >= 0 && cy >= 0 && cx < GRID_COLS && cy < GRID_ROWS);
    }

    inline int cellIndex(int cx, int cy) const {
        return cx * GRID_ROWS + cy;
    }

    void resolveCollision(Particle& a, Particle& b) {
        sf::Vector2f v = a.position - b.position;
        float dist2 = v.x * v.x + v.y * v.y;
        float min_dist = a.radius + b.radius;

        if (dist2 < 1e-12f) { v = {1.f, 0.f}; dist2 = 1.f; }

        float min2 = min_dist * min_dist;
        if (dist2 >= min2) return;

        float dist = std::sqrt(dist2);

        float delta = 0.5f * (min_dist - dist);
        sf::Vector2f n = (v / dist) * delta;

        a.position += n;
        b.position -= n;
    }

    void solveSlice(const Slice &s) {
        for (int x = s.start; x < s.end; ++x) {
            const int base = x * GRID_ROWS;

            for (int y = 0; y < GRID_ROWS; ++y) {
                const int cellIdx = base + y;
                Cell &c = grid[cellIdx];

                if (c.count == 0) continue;

                if (c.count >= 2) {
                    for (std::size_t i = 0; i < c.count; ++i) {
                        int aIdx = c.ids[i];
                        for (std::size_t j = i + 1; j < c.count; ++j) {
                            int bIdx = c.ids[j];
                            resolveCollision(particles[aIdx], particles[bIdx]);
                        }
                    }
                }

                for (int k = 0; k < 4; ++k) {
                    int nx = x + ndx[k];
                    int ny = y + ndy[k];
                    if (!inBoundsCell(nx, ny)) continue;

                    auto &ncell = grid[cellIndex(nx, ny)];
                    if (ncell.count == 0) continue;

                    for (int aIdx = 0; aIdx < c.count; ++aIdx) {
                        for (int bIdx = 0; bIdx < ncell.count; ++bIdx) {
                            resolveCollision(particles[c.ids[aIdx]], particles[ncell.ids[bIdx]]);
                        }
                    }
                }
            }
        }
    }

    void updateStartingVel() {
        if (goingUp) {
            if (startingVel.x + 25.f < 500.f) startingVel.x += 25.f;
            else { startingVel.x = 500.f; goingUp = false; }
        } else {
            if (startingVel.x - 25.f > -500.f) startingVel.x -= 25.f;
            else { startingVel.x = -500.f; goingUp = true; }
        }
    }

    void handleMouseHeld(Particle& particle, const int cx, const int cy, const sf::Vector2f& mousePos) {
        int pcx = static_cast<int>(particle.position.x / CELL_SIZE);
        int pcy = static_cast<int>(particle.position.y / CELL_SIZE);

        if (pcx <= cx + (MOUSE_RADIUS / CELL_SIZE) &&
            pcx >= cx - (MOUSE_RADIUS / CELL_SIZE) &&
            pcy <= cy + (MOUSE_RADIUS / CELL_SIZE) &&
            pcy >= cy - (MOUSE_RADIUS / CELL_SIZE)) {

            sf::Vector2f dir = mousePos - particle.position;
            float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            if (dist < 1e-12f) return;

            sf::Vector2f normalized = dir / dist;
            particle.acceleration += normalized * MOUSE_STRENGTH;
        }
    }

public:
    std::vector<Particle> particles;

    World(const int count, const int substeps)
        : PARTICLE_COUNT(count)
        , SUBSTEPS(substeps)
    {
        particles.reserve(count);
        grid.resize(GRID_ROWS * GRID_COLS);

        initTargetColorsIfAvailable();

        int threadCount = static_cast<int>(std::thread::hardware_concurrency());
        buildSlices(threadCount);
        workers.reserve(threadCount);
        for (int i = 0; i < threadCount; ++i) {
            workers.emplace_back([this] () {
                workerLoop();
            });
        }
    }

    ~World() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            stop.store(true, std::memory_order_release);
            generation++;
        }
        cvWork.notify_all();

        for (auto &th : workers) {
            th.join();
        }
        std::ofstream file("output.txt");
        for (auto &p : particles) {
            file << p.position.x << " " << p.position.y << "\n";
        }
        file.close();
    }

    void spawnIfPossible(const float elapsed_time, sf::Clock& spawner) {
        if (elapsed_time >= SPAWN_DELAY && particles.size() < (size_t)PARTICLE_COUNT) {
            auto colorForIndex = [&](std::size_t idx) -> sf::Color {
                if (haveTargetColors && idx < targetColors.size()) return targetColors[idx];
                return sf::Color(rand() % 255, rand() % 255, rand() % 255);
            };

            const float substep_dt = dt / static_cast<float>(SUBSTEPS);

            sf::Vector2f v(-100.f, 0.f);

            if (particles.size() + 21 > (size_t)PARTICLE_COUNT) {
                int diff = (int)PARTICLE_COUNT - (int)particles.size();
                for (int i = 0; i < diff; ++i) {
                    std::size_t idx = particles.size();
                    particles.emplace_back(startPos, 2.f, colorForIndex(idx));
                }

                for (int i = (int)particles.size() - diff; i < (int)particles.size(); ++i)
                    particles[i].prev_position = particles[i].position - startingVel * substep_dt;

            } else {
                for (int i = 0; i < 21; ++i) {
                    std::size_t idx = particles.size();
                    particles.emplace_back(startPos + v, 2.f, colorForIndex(idx));
                    v.x += 10;
                }

                for (int i = (int)particles.size() - 21; i < (int)particles.size(); ++i)
                    particles[i].prev_position = particles[i].position - startingVel * substep_dt;
            }

            updateStartingVel();
            spawner.restart();
        }
    }

    void update(InputState& inpState) {
        if (particles.empty()) return;

        const float substep_dt = dt / static_cast<float>(SUBSTEPS);
        const float dampening  = 0.8f;
        const float padding    = static_cast<float>(CELL_SIZE);

        buildGrid();

        for (int s = 0; s < SUBSTEPS; ++s) {
            int mx = 0, my = 0, rCells = 0;
            if (inpState.mouseHeld) {
                mx = std::max(0, std::min(static_cast<int>(inpState.mousePos.x / CELL_SIZE), GRID_COLS - 1));
                my = std::max(0, std::min(static_cast<int>(inpState.mousePos.y / CELL_SIZE), GRID_ROWS - 1));
                rCells = static_cast<int>(MOUSE_RADIUS / CELL_SIZE) + 1;
            }

            for (auto &p : particles) {
                p.accelerate(Particle::GRAVITY);
            }

            if (inpState.mouseHeld) {
                int x0 = std::max(0, mx - rCells);
                int x1 = std::min(GRID_COLS - 1, mx + rCells);
                int y0 = std::max(0, my - rCells);
                int y1 = std::min(GRID_ROWS - 1, my + rCells);

                for (int cy = y0; cy <= y1; ++cy) {
                    for (int cx = x0; cx <= x1; ++cx) {
                        auto &cell = grid[cellIndex(cx, cy)];
                        for (int idx = 0; idx < cell.count; ++idx) {
                            handleMouseHeld(particles[cell.ids[idx]], mx, my, inpState.mousePos);
                        }
                    }
                }
            }

            runPass(evenSlices);
            runPass(oddSlices);

            for (auto& p : particles) {
                p.applyBorderBounce((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, padding, dampening);
            }

            for (auto& p : particles) {
                p.integrate(substep_dt);

                sf::Vector2f disp = p.getDisplacement();
                float disp2 = disp.x * disp.x + disp.y * disp.y;
                if (disp2 > 2.f * padding) {
                    p.prev_position = p.position;
                }
            }

            buildGrid();
        }
    }

    void draw(sf::RenderWindow& window) {
        renderer.build(particles);
        renderer.draw(window);
    }
};

class VisualText {
private:
    sf::Font font;
    sf::Text particleCount, timeBetweenFrames;

public:
    VisualText() {
        font.loadFromFile("../assets/arial.ttf");

        particleCount.setFont(font);
        particleCount.setString("--");
        particleCount.setCharacterSize(24);
        particleCount.setFillColor(sf::Color::Black);

        timeBetweenFrames.setFont(font);
        timeBetweenFrames.setString("--");
        timeBetweenFrames.setCharacterSize(24);
        timeBetweenFrames.setFillColor(sf::Color::Black);
        timeBetweenFrames.setPosition(0.f, 30.f);
    }

    void draw(sf::RenderWindow& window) const {
        window.draw(particleCount);
        window.draw(timeBetweenFrames);
    }

    void setParticle(std::string cnt) { particleCount.setString(cnt); }
    void setFrames(std::string cnt)   { timeBetweenFrames.setString(cnt + "ms"); }
};
