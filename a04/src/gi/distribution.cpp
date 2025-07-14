#include "distribution.h"
#include "buffer.h"
#include "texture.h"
#include "random.h"
#include "color.h"
#include <iostream>

// ----------------------------------------------------
// Distribution1D

Distribution1D::Distribution1D() : f_integral(0) {}

Distribution1D::Distribution1D(const float* f, uint32_t N) : func(f, f + N), cdf(N + 1) {
    // build non-normalized cdf in [0, N)
    cdf[0] = 0;
    for (uint32_t i = 1; i < N + 1; ++i)
        cdf[i] = cdf[i - 1] + func[i - 1];
    f_integral = cdf[N];
    // convert to (discrete) density
    for (uint32_t i = 1; i < N + 1; ++i)
        cdf[i] = f_integral != 0.f ? cdf[i] / f_integral : float(i) / float(N);
    assert(cdf[N] == 1);
}

Distribution1D::~Distribution1D() {}

double Distribution1D::integral() const {
    return f_integral; // Integration in [0, N), area under function value is: 1 * func[i]
}

double Distribution1D::unit_integral() const {
    return f_integral / size(); // Integration in [0, 1), area under function value is: (1 / N) * func[i]
}

float Distribution1D::pdf(float sample) const {
    assert(sample >= 0 && sample < 1);
    return func[sample * size()] / unit_integral();
}

float Distribution1D::pdf(size_t index) const {
    assert(index < size());
    return func[index] / integral();
}

std::tuple<float, float> Distribution1D::sample_01(float sample) const {
    assert(sample >= 0 && sample < 1);
    const uint32_t offset = glm::max(0, int(std::lower_bound(cdf.begin(), cdf.end(), sample) - cdf.begin()) - 1);
    assert(offset <= size());
    assert(sample <= cdf[offset+1]);
    float du = sample - cdf[offset];
    if ((cdf[offset + 1] - cdf[offset]) > 0)
        du /= (cdf[offset + 1] - cdf[offset]);
    const float pdf = f_integral > 0 ? func[offset] / unit_integral() : 0.f;
    assert(std::isfinite(pdf));
    return { (offset + du) / float(func.size()), pdf };
}

std::tuple<uint32_t, float> Distribution1D::sample_index(float sample) const {
    assert(sample >= 0 && sample < 1);
    const uint32_t offset = glm::max(0, int(std::lower_bound(cdf.begin(), cdf.end(), sample) - cdf.begin()) - 1);
    assert(offset <= size());
    assert(sample <= cdf[offset+1]);
    const float pdf = f_integral > 0 ? func[offset] / integral() : 0.f;
    assert(std::isfinite(pdf));
    return { offset, pdf };
}

// ----------------------------------------------------
// Distribution2D

Distribution2D::Distribution2D(const float* f, uint32_t w, uint32_t h) {
    for (uint32_t y = 0; y < h; ++y)
        conditional.push_back(Distribution1D(&f[y * w], w));
    std::vector<float> marginal_func;
    marginal_func.reserve(h);
    for (uint32_t y = 0; y < h; ++y)
        marginal_func.push_back(conditional[y].integral());
    marginal = Distribution1D(marginal_func.data(), h);
}

Distribution2D::~Distribution2D() {}

double Distribution2D::integral() const {
    return marginal.integral();
}

double Distribution2D::unit_integral() const {
    return marginal.integral() / (marginal.size() * conditional[0].size());
}

std::tuple<glm::vec2, float> Distribution2D::sample_01(const glm::vec2& sample) const {
    assert(sample.x >= 0 && sample.x < 1); assert(sample.y >= 0 && sample.y < 1);
    const auto [y, pdf_y] = marginal.sample_01(sample.y);
    const auto [x, pdf_x] = conditional[uint32_t(y * marginal.size())].sample_01(sample.x);
    const float pdf = pdf_y * pdf_x;
    assert(std::isfinite(pdf));
    return { glm::vec2(x, y), pdf };
}

float Distribution2D::pdf(const glm::vec2& sample) const {
    assert(sample.x >= 0 && sample.x < 1); assert(sample.y >= 0 && sample.y < 1);
    const int x = glm::clamp(int(sample.x * conditional[0].size()), 0, int(conditional[0].size()) - 1);
    const int y = glm::clamp(int(sample.y * marginal.size()), 0, int(marginal.size()) - 1);
    return conditional[y].f(x) / marginal.integral();
}

// ----------------------------------------------------
// Debug utilities

void plot_histogram(const Distribution1D& dist, const std::string& name) {
    uint32_t N = 250000, w = std::min(1000u, dist.size()), h = w / 2;
    Buffer<float> results(w);
    Buffer<float> pdfs(w);
    results = 0;
    UniformSampler1D sampler;
    sampler.init(N);
    for (uint32_t i = 0; i < N; ++i) {
        const auto [sample, pdf] = dist.sample_01(sampler.next());
        results[uint32_t(sample * w) % w] += 1;
        pdfs[uint32_t(sample * w) % w] += pdf;
    }
    // scale values
    float max_val = FLT_MIN;
    for (uint32_t x = 0; x < w; ++x) {
        pdfs(x) /= fmaxf(1.f, results(x));
        max_val = fmaxf(results(x), max_val);
    }
    for (uint32_t x = 0; x < w; ++x) {
        results(x) = results(x) / max_val;
        pdfs(x) = pdfs(x) / 10.f;
    }
    // build histogram
    Buffer<glm::vec3> buffer(w, h);
    Buffer<glm::vec3> buffer_pdf(w, h);
    buffer = glm::vec3(0);
    buffer_pdf = glm::vec3(0);
    for (uint32_t x = 0; x < w; ++x) {
        for (uint32_t y = 0; y < h; ++y) {
            if (y < results(x) * h)
                buffer(x, y) = heatmap(results(x));
            if (y < pdfs(x) * h)
                buffer_pdf(x, y) = heatmap(pdfs(x));
        }
    }
    // output
    static uint32_t i = 0;
    Texture::save_png(std::string("dist1D_") + name + "_hits.png", w, h, buffer.data());
    Texture::save_png(std::string("dist1D_") + name + "_pdf.png", w, h, buffer_pdf.data());
}

void plot_heatmap(const Distribution2D& dist, uint32_t w, uint32_t h) {
    uint32_t N = 100000;
    Buffer<glm::vec3> buffer_n(w, h), buffer_pdf(w, h);
    buffer_n = glm::vec3(0);
    buffer_pdf = glm::vec3(0);
    UniformSampler2D sampler;
    sampler.init(N);
    for (uint32_t i = 0; i < N; ++i) {
        const auto [sample, pdf]= dist.sample_01(sampler.next());
        uint32_t x = sample.x * w;
        uint32_t y = sample.y * h;
        if (x >= 0 && y >= 0 && x < w && y < h) {
            buffer_n(x, y) += glm::vec3(1);
            buffer_pdf(x, y) += glm::vec3(pdf);
        }
    }
    // scale values
    glm::vec3 max_val = glm::vec3(FLT_MIN);
    for (uint32_t y = 0; y < h; ++y)
        for (uint32_t x = 0; x < w; ++x)
            max_val = max(buffer_n(x, y), max_val);
    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            buffer_pdf(x, y) = heatmap(buffer_pdf(x, y).x / (buffer_n(x, y).x * 100)); // scale for visibility
            buffer_n(x, y) = heatmap((buffer_n(x, y) / (0.5f * max_val)).x); // scale for visibility
        }
    }
    // output
    static uint32_t i = 0;
    Texture::save_png(std::string("dist2D_") + std::to_string(i) + "_hits.png", w, h, buffer_n.data(), false);
    Texture::save_png(std::string("dist2D_") + std::to_string(i++) + "_pdf.png", w, h, buffer_pdf.data(), false);
}

void debug_distributions() {
    {
        // debug 1D
        const int N = 1000;
        std::vector<float> values(N);
        // const func
        for (int i = 0; i < N; ++i)
            values[i] = 1;
        Distribution1D dist(values.data(), N);
        plot_histogram(dist, "const");
        // step func
        for (int i = 0; i < N; ++i)
            values[i] = (i+1) / float(N);
        dist = Distribution1D(values.data(), N);
        plot_histogram(dist, "gradient");
        // power func 
        for (int i = 0; i < N; ++i)
            values[i] = pow((i+1) / float(N), 4);
        dist = Distribution1D(values.data(), N);
        plot_histogram(dist, "pow");
        // triangle func
        for (int i = 0; i < N; ++i)
            values[i] = N/2 - std::abs(i - N/2);
        dist = Distribution1D(values.data(), N);
        plot_histogram(dist, "abs");
    }
    {
        // debug 2D
        const int W = 1280, H = 720;
        std::vector<float> values(W*H);
        // SDF field
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
                values[y*W+x] = pow(length(glm::vec2(W/4,H/4)) - length(glm::vec2(x, y) - glm::vec2(W/2, H/2)), 4);
        Distribution2D dist(values.data(), W, H);
        plot_heatmap(dist, W, H);
    }
}
