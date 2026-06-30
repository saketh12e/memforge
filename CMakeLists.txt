cmake_minimum_required(VERSION 3.16)
project(memforge CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor
                        -Woverloaded-virtual -Wcast-align -Wunused -Wformat=2
                        -Wmisleading-indentation -Wduplicated-cond
                        -Wnull-dereference -Wdouble-promotion -fvisibility=hidden)
endif()

option(MEMFORGE_BUILD_TESTS "Build unit and integration tests" ON)
option(MEMFORGE_BUILD_EXAMPLES "Build worked examples" ON)
option(MEMFORGE_BUILD_CLI "Build command-line tool" ON)
option(MEMFORGE_SANITIZE "Compile memforge library with sanitizers" OFF)

if(MEMFORGE_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

add_library(memforge_core
    src/core/error.cpp
    src/core/span.cpp
    src/core/checked_math.cpp
    src/core/op_stream.cpp
    src/core/op_validator.cpp
    src/core/span_guard.cpp
    src/core/free_list.cpp
    src/core/profiler.cpp
    src/core/settings.cpp
    src/core/dump.cpp
    src/core/rate_counter.cpp
    src/core/hex.cpp
    src/arena/arena_chain.cpp
    src/arena/arena_chain_detail.cpp
    src/arena/batch_allocator.cpp
    src/arena/batch_allocator_detail.cpp
    src/pool/slab_cache.cpp
    src/pool/slab_cache_detail.cpp
    src/pool/shard_map.cpp
    src/handle/handle_table.cpp
    src/handle/handle_table_detail.cpp
    src/epoch/epoch_queue.cpp
    src/epoch/epoch_queue_detail.cpp
    src/epoch/deferred_free_list.cpp
    src/epoch/deferred_free_list_detail.cpp
    src/scope/tx_scope.cpp
    src/scope/tx_scope_detail.cpp
    src/migrate/pool_migrator.cpp
    src/migrate/pool_migrator_detail.cpp
    src/pin/pin_registry.cpp
    src/pin/pin_registry_detail.cpp
    src/stats/stats_aggregator.cpp
    src/stats/stats_aggregator_detail.cpp
    src/checkpoint/checkpoint_writer.cpp
    src/checkpoint/checkpoint_writer_detail.cpp
    src/checkpoint/checkpoint_reader.cpp
    src/checkpoint/checkpoint_reader_detail.cpp
    src/hub/resource_hub.cpp
    src/hub/resource_hub_detail.cpp
    src/replay/replay.cpp
    src/replay/replay_workflows.cpp
    src/cli/args.cpp
)

target_include_directories(memforge_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(memforge_core PUBLIC cxx_std_17)

set_target_properties(memforge_core PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    POSITION_INDEPENDENT_CODE ON
)

if(MEMFORGE_BUILD_CLI)
    add_executable(memforge_cli src/cli/memforge.cpp)
    target_link_libraries(memforge_cli PRIVATE memforge_core)
endif()

if(MEMFORGE_BUILD_EXAMPLES)
    add_executable(memforge_example_server examples/server_pool_workflow.cpp)
    add_executable(memforge_example_epoch examples/epoch_reclamation_demo.cpp)
    add_executable(memforge_example_checkpoint examples/checkpoint_roundtrip_workflow.cpp)
    add_executable(memforge_example_handle examples/handle_table_churn.cpp)
    add_executable(memforge_example_tx examples/tx_rollback_demo.cpp)
    add_executable(memforge_example_pin examples/pin_registry_alias.cpp)
    add_executable(memforge_example_shard examples/shard_rebalance_workflow.cpp)
    add_executable(memforge_example_stats examples/stats_aggregation_workflow.cpp)
    add_executable(memforge_example_pool examples/pool_allocation_patterns.cpp)
    add_executable(memforge_example_handle_demo examples/handle_table_demographics.cpp)
    add_executable(memforge_example_epoch_progress examples/epoch_progression.cpp)
    add_executable(memforge_example_defer examples/deferred_free_cycle.cpp)
    add_executable(memforge_example_guard examples/span_guard_attach_verify.cpp)
    add_executable(memforge_example_batch examples/batch_allocator_stage_commit.cpp)
    add_executable(memforge_example_ckpt_size examples/checkpoint_size.cpp)
    add_executable(memforge_example_shard_rebal examples/shard_attach_rebalance.cpp)
    add_executable(memforge_example_stats_fold examples/stats_fold_workflow.cpp)
    add_executable(memforge_example_hub_metrics examples/hub_metrics_summary.cpp)
    foreach(ex memforge_example_server memforge_example_epoch memforge_example_checkpoint
              memforge_example_handle memforge_example_tx memforge_example_pin
              memforge_example_shard memforge_example_stats
              memforge_example_pool memforge_example_handle_demo
              memforge_example_epoch_progress memforge_example_defer
              memforge_example_guard memforge_example_batch
              memforge_example_ckpt_size memforge_example_shard_rebal
              memforge_example_stats_fold memforge_example_hub_metrics)
        target_link_libraries(${ex} PRIVATE memforge_core)
    endforeach()
endif()

if(MEMFORGE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

install(TARGETS memforge_core
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
install(DIRECTORY include/memforge DESTINATION include)
