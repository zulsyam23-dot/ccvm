using Pkg
Pkg.activate(".")

include("src/BenchmarkingSuite.jl")
using .BenchmarkingSuite

function main()
    println("=== CCVM Benchmarking System ===")
    
    # Create example suite
    suite = BenchmarkingSuite.create_example_benchmark_suite()
    
    # Run benchmarks
    results = run_full_benchmark(suite)
    
    # Generate report
    output_dir = "results/$(Dates.format(now(), "yyyy-mm-dd_HH-MM-SS"))"
    generate_performance_report(results, output_dir)
    
    println("\n✅ Benchmarking completed!")
end

if abspath(PROGRAM_FILE) == @__FILE__
    main()
end
