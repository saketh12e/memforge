add_executable(memforge_tests
    test_main.cpp
    test_arena.cpp
    test_pool.cpp
    test_handle.cpp
    test_epoch.cpp
    test_scope.cpp
    test_migrate.cpp
    test_stats.cpp
    test_checkpoint.cpp
    test_pin.cpp
    test_defer.cpp
    test_compact.cpp
    test_shard.cpp
    test_guard.cpp
    test_batch.cpp
    test_teardown.cpp
    test_op_stream.cpp
    test_op_stream_full.cpp
    test_resource_hub.cpp
    test_cli.cpp
    test_lifecycle.cpp
    test_stress.cpp
    test_stress_extended.cpp
    test_util.cpp
    test_diagnostics.cpp
    test_integration.cpp
    test_validator.cpp
    test_module.cpp
    test_cli_args.cpp
    test_serialize.cpp
    test_persistence.cpp
    test_smoke.cpp
    test_freelist.cpp
    test_profiler.cpp
    test_dump.cpp
)
target_link_libraries(memforge_tests PRIVATE memforge_core)
target_include_directories(memforge_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
add_test(NAME memforge_tests COMMAND memforge_tests)

# not built or shipped with the public repo. The public repo only contains
# regression tests that exercise the public surface under normal op flow.
