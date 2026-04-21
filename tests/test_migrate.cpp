#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/pool_migrator.hpp>
#include <memforge/slab_cache.hpp>

namespace {
void test_migrate_plan_commit() {
    using namespace memforge;
    SlabCache src;
    SlabCache dst;
    src.init();
    dst.init();
    PoolMigrator m;
    m.init();
    m.plan(src, dst, MigratePlanOp{0, 0, 4, 32});
    m.commit(src, dst);
    m.shutdown();
    src.shutdown();
    dst.shutdown();
}

void test_migrate_invalid_count() {
    using namespace memforge;
    SlabCache a;
    SlabCache b;
    a.init();
    b.init();
    PoolMigrator m;
    m.init();
    assert(!m.plan(a, b, MigratePlanOp{0, 0, 0, 32}).ok());
    m.shutdown();
    a.shutdown();
    b.shutdown();
}

void test_migrate_oversized() {
    using namespace memforge;
    SlabCache a;
    SlabCache b;
    a.init();
    b.init();
    PoolMigrator m;
    m.init();
    assert(!m.plan(a, b, MigratePlanOp{0, 0, kMaxMigrationSlots + 1, 32}).ok());
    m.shutdown();
    a.shutdown();
    b.shutdown();
}

void test_migrate_apply() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::MigratePlan, 0, 0, 4, 32);
    ops.append(OpKind::MigrateCommit, 0, 0, 0, 0);
    SlabCache a;
    SlabCache b;
    a.init();
    b.init();
    PoolMigrator m;
    m.init();
    m.apply(a, b, ops);
    m.shutdown();
    a.shutdown();
    b.shutdown();
}

void test_migrate_commit_empty() {
    using namespace memforge;
    SlabCache a;
    SlabCache b;
    a.init();
    b.init();
    PoolMigrator m;
    m.init();
    assert(m.commit(a, b).ok());
    m.shutdown();
    a.shutdown();
    b.shutdown();
}
}  // namespace

// static memforge_test::Register r1("migrate.plan_commit", &test_migrate_plan_commit);
// static memforge_test::Register r2("migrate.invalid_count", &test_migrate_invalid_count);
// static memforge_test::Register r3("migrate.oversized", &test_migrate_oversized);
// static memforge_test::Register r4("migrate.apply", &test_migrate_apply);
// static memforge_test::Register r5("migrate.commit_empty", &test_migrate_commit_empty);
