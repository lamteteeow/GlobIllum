#include "render.h"

#include "gi/color.h"
#include "gi/algorithm.h"

#include <iostream>

// ---------------------------------------------------------------------------------
// helper functions

static constexpr size_t TILESIZE = 32;

inline float block_convergence(const Context& ctx, size_t bx, size_t by) {
    float mean = 0, m2 = 0, count = 0;
    for (size_t y = by * TILESIZE; y < glm::min(ctx.fbo.h, (by + 1) * TILESIZE); ++y) {
        for (size_t x = bx * TILESIZE; x < glm::min(ctx.fbo.w, (bx + 1) * TILESIZE); ++x) {
            const float err = luma(glm::abs(ctx.fbo.color(x, y) - ctx.fbo.even(x, y))) / fmaxf(1e-5f, luma(ctx.fbo.color(x, y)));
            count += 1;
            const float delta = err - mean;
            mean += delta / count;
            const float delta2 = err - mean;
            m2 += delta * delta2;
        }
    }
    const float f = fmaxf(0.f, 1 - ctx.fbo.num_samples(bx*TILESIZE, by*TILESIZE) / float(1 << 13));
    const float var_crit = f * (m2 / (count - 1)) / fmaxf(1e-5f, sqrtf(mean));
    return (2 * var_crit * mean) / (var_crit + mean);
}

// ---------------------------------------------------------------------------------
// main rendering "loop"

void render(Context &ctx) {
    const std::shared_ptr<Algorithm> algo = Algorithm::algorithms[ctx.algorithm];
    if (!algo) {
        std::cerr << "Error: No rendering algorithm selected!" << std::endl;
        return;
    }

    CLEAR_STATS();
    Timer timings;

    timings.start("commit");
    ctx.scene.commit();
    ctx.cam.commit();
    if (ctx.scene.lights.empty()) {
        std::cerr << "Error: Trying to render scene without light sources." << std::endl;
        return;
    }
    if (ctx.AUTO_FOCUS) {
        ctx.cam.focal_depth = ctx.filter_focal_distance();
    }
    algo->init(ctx);
    timings.stop("commit");

    if (ctx.abort) return;

    timings.start("render");
    const size_t w = ctx.fbo.width(), h = ctx.fbo.height(), sppx = ctx.fbo.samples();

    // push 1sppx quickly
    const auto start = std::chrono::system_clock::now();
    #pragma omp parallel for
    for (int y = 0; y < int(h); ++y) {
        for (int x = 0; x < int(w); ++x) {
            if (ctx.abort) break;
            algo->sample_pixel(ctx, x, y, 1);
        }
    }
    const auto end = std::chrono::system_clock::now();
    const size_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    if (ctx.abort) return;

    printf(
        "Approx. render time using algorithm \"%s\": %zum, %zus\n",
        ctx.algorithm.c_str(), (sppx - 1) * ms / 60000, ((sppx - 1) * ms / 1000) % 60
    );

    // render rest of samples
    #pragma omp parallel for
    for (int y = 0; y < int(h); ++y) {
        for (int x = 0; x < int(w); ++x) {
            if (ctx.abort) break;
            algo->sample_pixel(ctx, x, y, sppx - 1);
        }
    }
    timings.stop("render");

    if (ctx.abort) return;

    if (ctx.BEAUTY_RENDER) {
        timings.start("convergence");
        // init data structure
        MutexPrioQueue unconverged;
        const int TILES_W = (ctx.fbo.w + TILESIZE - 1) / TILESIZE;
        const int TILES_H = (ctx.fbo.h + TILESIZE - 1) / TILESIZE;
        #pragma omp parallel for
        for (int by = 0; by < TILES_H; ++by) {
            for (int bx = 0; bx < TILES_W; ++bx) {
                const float conv = block_convergence(ctx, bx, by);
                if (conv > ctx.ERROR_EPS)
                    unconverged.push(by * TILES_W + bx, conv);
            }
        }
        // render until converged
        printf("Rendering until error < %.3f...\n", ctx.ERROR_EPS);
        #pragma omp parallel
        {
            size_t id;
            while (unconverged.pop(id) && !ctx.abort) {
                const uint32_t bx = id % TILES_W, by = id / TILES_W;
                for (uint32_t y = by * TILESIZE; y < glm::min(ctx.fbo.h, (by + 1) * TILESIZE); ++y)
                    for (uint32_t x = bx * TILESIZE; x < glm::min(ctx.fbo.w, (bx + 1) * TILESIZE); ++x)
                        algo->sample_pixel(ctx, x, y, 32);
                const float conv = block_convergence(ctx, bx, by);
                if (conv > ctx.ERROR_EPS) {
                    unconverged.push(by * TILES_W + bx, conv);
                }
                if (omp_get_thread_num() == 0) {
                    printf("error: %3.3f, #blocks: %4zu\r", conv, unconverged.queue.size());
                    fflush(stdout);
                }
            }
        }
        printf("\n");
        timings.stop("convergence");
    }

    if (ctx.abort) return;

    timings.start("postprocess");
    ctx.fbo.tonemap();
#ifdef WITH_OIDN
    if (ctx.BEAUTY_RENDER)
        ctx.fbo.denoise();
#endif
    timings.stop("postprocess");

    ctx.fbo.save("output.png");
    timings.print();
    PRINT_STATS();

    algo->post_render();
}
