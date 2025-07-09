#include "distribution.h"
#include "buffer.h"
#include "texture.h"
#include "random.h"
#include "color.h"
#include <iostream>

// ----------------------------------------------------
// Distribution1D

Distribution1D::Distribution1D() : f_integral(0) {}

Distribution1D::Distribution1D(const float* f, uint32_t N) : func(f, f + N), cdf(N + 1),
    f_integral(std::reduce(func.begin(), func.end())) {
    // TODO ASSIGNMENT3
    // build a CDF from given discrete function values and ensure a density
    // Hint: take extra care regarding corner-cases!
    assert(f_integral >= 0.f);
    if (f_integral == 0) return;
    float acc = 0.;
    for (size_t i = 0; i < func.size(); i++) {
        assert(func[i] >= 0.f);
        cdf[i] = acc;
        acc += pdf(i);
    }
    cdf.back() = 1.f;
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
    // TODO ASSIGNMENT3
    // draw a sample in [0, 1) according to this distribution and the respective PDF
    // hint: a piecewise constant function is assumed, so you may linearly interpolate between function values
    if (f_integral == 0) return { sample, 1.f / size() };
    size_t start = 0;
    size_t end = cdf.size();
    while (start != end - 1u)
    {
        size_t mid = (end + start) / 2u;
        if (cdf[mid] > sample) end = mid;
        else start = mid;
    }
    float t = cdf[end] == cdf[start] ? 0 : (sample - cdf[start]) / (cdf[end] - cdf[start]);
    assert(t >= 0.f && t < 1.f);
    float result = (start + t) / float(size());
    assert(result >= 0.f && result < 1.f);
    return { result, pdf(result) };
}

std::tuple<uint32_t, float> Distribution1D::sample_index(float sample) const {
    // TODO ASSIGNMENT3
    // sample an index in [0, n) according to this distribution and the respective PDF
    // note: take care about proper normalization of the PDF!
    size_t result = std::get<0>(sample_01(sample)) * size();
    return { result, pdf(result) };
}

// ----------------------------------------------------
// Distribution2D

Distribution2D::Distribution2D(const float* f, uint32_t w, uint32_t h) :
    conditional(h), f_integral {} {
    // TODO ASSIGNMENT3
    // build conditional and marginal distributions from linearized array of function values
    // hint: use f[y * w + x] to get the value at (x, y)
    // hint: you may re-use the Distribution1D
    // hint: use plot_heatmap(*this, w, h) to plot this distribution for debugging or validation
    assert(w > 0 && h > 0);

    std::vector<Distribution1D> rows (h);
    std::vector<float> row_integrals (h);
    for (uint32_t i = 0; i < h; i++) {
        rows[i] = {&f[i * w], w};
        assert(rows[i].integral() >= 0.);
        f_integral += rows[i].integral();
        row_integrals[i] = rows[i].integral();
    }
    marginal_y = {row_integrals.data(), h};

    std::vector<float> cond(w);
    for (uint32_t y = 0; y < h; y++) {
        if (marginal_y.f(y) != 0.f)
        {
            for (uint32_t x = 0; x < w; x++) {
                cond[x] = f[y * w + x] / marginal_y.f(y);
            }
        }
        conditional[y] = {cond.data(), w};
    }

    // plot_heatmap(*this, w, h);
}

Distribution2D::~Distribution2D() {}

double Distribution2D::integral() const {
    // TODO ASSIGNMENT3
    // return the integral here
    return f_integral;
}

double Distribution2D::unit_integral() const {
    // TODO ASSIGNMENT3
    // return the unit integral here
    return f_integral / (marginal_y.size() * conditional[0].size());
}

std::tuple<glm::vec2, float> Distribution2D::sample_01(const glm::vec2& sample) const {
    // TODO ASSIGNMENT3
    // draw a two-dimensional sample in [0, 1) from this distribution and compute its PDF
    auto [y_sample, y_pdf] = marginal_y.sample_01(sample.y);
    auto [x_sample, x_pdf] = conditional[y_sample * conditional.size()].sample_01(sample.x);
    return { {x_sample, y_sample}, x_pdf * y_pdf };
}

float Distribution2D::pdf(const glm::vec2& sample) const {
    throw std::runtime_error("Function not implemented: " + std::string(__FILE__) + ", line: " + std::to_string(__LINE__));
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
