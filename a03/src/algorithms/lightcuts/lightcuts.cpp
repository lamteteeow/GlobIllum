#include "driver/context.h"
#include "gi/algorithm.h"
#include "gi/bdpt.h"

#include "lc_light_tree.h"
#include "lc_print.h"

struct LightCuts final : Algorithm {
    inline static const std::string name = "LightCuts";

    void read_config(const json11::Json &cfg) override {
        json_set_bool(cfg, "lightcuts_print_status", m_settings.print_status);
        json_set_bool(cfg, "lightcuts_print_tree_bin", m_settings.print_tree_bin);
        json_set_bool(cfg, "lightcuts_print_tree_dot", m_settings.print_tree_dot);
        json_set_bool(cfg, "lightcuts_debug_cut_size", m_settings.debug_cut_size);
        json_set_bool(cfg, "lightcuts_override_params", m_settings.override_params);
        json_set_size(cfg, "lightcuts_vpl_path_count", m_settings.vpl_path_count);
        json_set_size(cfg, "lightcuts_vpl_path_length", m_settings.vpl_path_len);
    }

    void init(Context &context) override {
        if (m_settings.override_params) {
            std::cout << "\033[33m"
                      << "LightCuts: Warning, overriding scene params!" << std::endl
                      << "  camera.lens_radius = 0" << std::endl
                      << "  framebuffer.sppx = 1" << std::endl
                      << "  -> set 'lightcuts_override_params = false' to disable" << std::endl
                      << "\033[00m";

            context.cam.lens_radius = 0.f;
            context.fbo.sppx        = 1;
        }

        const size_t scene_hash = make_scene_hash(context.scene);
        if (!m_lightTree.is_empty() && m_scene_hash == scene_hash) {
            std::cout << "LightCuts: scene has not changed, reusing light tree" << std::endl;
            return;
        }
        m_scene_hash = scene_hash;

        std::vector<VirtualLight> virtual_lights;

        // step 1: trace virtual lights
        {
            LcTimer timer;

            if (m_settings.print_status) {
                std::cout << "LightCuts: Tracing light paths..." << std::endl
                          << "  path count:\t" << m_settings.vpl_path_count << std::endl
                          << "  path length:\t" << m_settings.vpl_path_len << std::endl;
            }

            timer.begin();
            trace_virtual_lights(context, virtual_lights, m_settings.print_status);
            timer.end();

            if (m_settings.print_status) {
                std::cout << "  vpl count:\t" << virtual_lights.size() << std::endl;
                timer.report();
            }
        }

        // step 2: build light tree
        {
            LcTimer timer;

            if (m_settings.print_status) {
                std::cout << "LightCuts: Building light tree..." << std::endl;
            }

            timer.begin();
            m_lightTree.build(std::move(virtual_lights), m_settings.print_status);
            timer.end();

            if (m_settings.print_status) {
                timer.report();
            }

            if (m_settings.print_tree_bin) m_lightTree.print_binary("lights.bin");
            if (m_settings.print_tree_dot) m_lightTree.print_dot("lights.dot");
        }

        if (m_lightTreeArena) LightTree::free_memory_arena(m_lightTreeArena);
        m_lightTreeArena = m_lightTree.allocate_memory_arena();
    }

    void sample_pixel(Context &context, const uint32_t x, const uint32_t y, const uint32_t samples) override {
        RandomWalkCam cam_walk;
        cam_walk.init(samples);

        std::vector<PathVertex> cam_path;

        for (uint32_t s = 0; s < samples; ++s) {
            trace_cam_path(context, x, y, cam_path, cam_walk, 8, 8, 0.f, true);

            // ray escaped scene
            if (cam_path.empty()) {
                continue;
            }

            assert(cam_path.size() == 1 && "cam path size was larger than one");
            context.fbo.add_sample(x, y, shade_vertex(context, cam_path[0]));
        }
    }

    void post_render() override {
    }

  private:
    void trace_virtual_lights(
        const Context &context, std::vector<VirtualLight> &virtual_lights, const bool print_progress
    ) const {
        virtual_lights.clear();
        virtual_lights.reserve(m_settings.vpl_path_count * m_settings.vpl_path_len);

        RandomWalkLight light_walk;
        light_walk.init(m_settings.vpl_path_count);

        std::vector<PathVertex> path_vertices;
        path_vertices.reserve(m_settings.vpl_path_len);

        LcProgressBar progress_bar;
        if (print_progress) progress_bar.init(m_settings.vpl_path_count);

        for (uint32_t i = 0; i < m_settings.vpl_path_count; ++i) {
            trace_light_path(context, path_vertices, light_walk, m_settings.vpl_path_len, m_settings.vpl_path_len, 0.f);

            for (const auto &vertex : path_vertices) {
                if (vertex.hit.valid) {
                    virtual_lights.emplace_back(vertex);
                }
            }

            if (print_progress) progress_bar.advance();
        }

        if (print_progress) progress_bar.end();
    }

    [[nodiscard]] glm::vec3 shade_vertex(const Context &context, const PathVertex &cam_vertex) const {
        // catch direct light source hits (d == 0 only)
        if (cam_vertex.escaped || cam_vertex.on_light) {
            return cam_vertex.throughput;
        }

        if (!cam_vertex.hit.valid) {
            return glm::vec3(0.f);
        }

        return m_lightTree.eval(m_lightTreeArena, context, cam_vertex, m_settings.debug_cut_size);
    }

    static size_t make_scene_hash(const Scene &scene) {
        const auto hash_combine = [](std::size_t &seed, const size_t v) {
            seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        };

        size_t hash = 0;
        for (const auto &mesh : scene.meshes) {
            hash_combine(hash, std::hash<const Mesh *>{}(mesh.get()));
        }
        for (const auto &material : scene.materials) {
            hash_combine(hash, std::hash<const Material *>{}(material.get()));
        }
        return hash;
    }

  private:
    struct Settings {
        bool print_status     = true;
        bool print_tree_bin   = false;
        bool print_tree_dot   = false;
        bool debug_cut_size   = false;
        bool override_params  = true;
        size_t vpl_path_count = 32768;
        size_t vpl_path_len   = 2;
    } m_settings;

    LightTree m_lightTree;
    LightTree::MemoryArenaRef m_lightTreeArena{};

    size_t m_scene_hash = 0;
};

[[maybe_unused]] static AlgorithmRegistrar<LightCuts> registrar;
