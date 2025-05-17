#include "context.h"
#include "gi/random.h"
#include "gi/distribution.h"

int main(int argc, char** argv) {
    plot_all_samplers2D();
    perform_sampler_benchmarks();

    // init context
    Context context;
    
    // attempt to load all provided arguments
    for (int i = 1; i < argc; ++i)
        context.load(argv[i]);

    // enter main loop
    context.run();

    return 0;
}
