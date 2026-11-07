# Embedded QuickJS-NG core patches

The Dart package remains `flutter_qjs 0.3.7`. Its embedded C core is based on
[`quickjs-ng/quickjs` at `d07f6b5232dd92ba712fe729bd018aae3a1b328f`](https://github.com/quickjs-ng/quickjs/tree/d07f6b5232dd92ba712fe729bd018aae3a1b328f),
not the unrelated `2024-02-14` label previously left in `VERSION`. The 18 source
and header files in the previous fork commit
`295f77421eedff356a02e92987181b525970c780` match that official commit. `VERSION`
now identifies this backport without claiming to be a newer official release.
`CORE_PATCHES.json` records the baseline and canonical source checksums.

VeneraNext maintainers retain this fork because the official stable wrapper
does not provide its current Dart/FFI, Map, TypedData, dispatch and port lifetime
contract. The official unpublished wrapper also needs a different dependency
graph and fails the current application's Map native probe. Those constraints
do not justify leaving matched core defects unaddressed.

## Reviewed changes

| Area | Backport and source |
| --- | --- |
| Array copy constructors | Initialize every new slot to `JS_UNDEFINED` before publishing the count and invoking getters or comparators. Covers `with`, `toReversed`, `toSpliced` and `toSorted`; follows [GHSA-m53g-qx47-3h38](https://github.com/quickjs-ng/quickjs/security/advisories/GHSA-m53g-qx47-3h38). |
| Fast array growth | Bound cell count and byte allocation and use wide growth arithmetic. Adapted to this older implementation from [a653771](https://github.com/quickjs-ng/quickjs/commit/a65377157e1cc4b538cfe540bdc45bcabc08ffc4). |
| CallSite ownership | Property insertion consumes its value even on failure; clear source fields when their ownership moves to the CallSite. Follows [c846cb1](https://github.com/quickjs-ng/quickjs/commit/c846cb1364272de07bcc42e5a385ccdadf6b8409). |
| Promise and async OOM | Initialize resolving slots and clear released values on failure, including async functions and generators. Follows [776d724](https://github.com/quickjs-ng/quickjs/commit/776d724cfa46a3b5bb045012ef4c838eae71ebaf). |
| TypedArray sorting | Use the existing checked `js_calloc` multiplication for the index table. Follows [e93cca6](https://github.com/quickjs-ng/quickjs/commit/e93cca611908e12b824ec85fbee6b31a5ee61568). |
| Proxy ownKeys | Use the same checked allocation for the older table implementation. This is a related size overflow fix, not a reproduction of the advisory's newer `extra_len` implementation. |
| Atomics.store | Release the converted value when coercion detaches the buffer. Follows [62951e1](https://github.com/quickjs-ng/quickjs/commit/62951e1bc68711d37aad986ff84ef0fe17038a7e). |
| ArrayBuffer transfer | Reject foreign backing stores, invalidate old typed views and retain the source if destination metadata or reallocation fails. Old-view invalidation follows [c952c93](https://github.com/quickjs-ng/quickjs/commit/c952c935faf2b8949352758b6c3ee22dea753c43); external ownership and allocation ordering are adapted to this baseline. |

The later optimized `Array.prototype.push` defect in
[GHSA-r6vf-p8wr-4vg9](https://github.com/quickjs-ng/quickjs/security/advisories/GHSA-r6vf-p8wr-4vg9)
does **not** apply to this baseline: its implementation uses generic
`JS_SetPropertyInt64`, without the later fast path. Do not backport the unrelated
`array_len == count` condition. Shrink/grow/push/GC regression remains included.
The reported rope-string implementation is also absent. The newer RegExp
advisory's version range does not establish this older compiler's exposure.
These statements do not constitute an audit of all possible core defects.

## Validation

`test/core_safety` compiles the actual patched core and `cxx/ffi.cpp`. The bounded
native test exercises getter/comparator GC reentry and exceptions, shrink/grow
with holes, checked allocation boundaries, old views after transfer, foreign
buffers and custom free callbacks. Five programs each inject a single allocator
failure at 256 consecutive positions, then drain jobs, collect and destroy the
runtime while checking that every tracked allocation is released. Failed
regression assertions abort the native test instead of being swallowed as OOM.

Build and run locally:

```sh
cmake -S test/core_safety -B build/core-safety -DCMAKE_BUILD_TYPE=RelWithDebInfo -DQJS_ENABLE_SANITIZERS=ON
cmake --build build/core-safety --config RelWithDebInfo
ctest --test-dir build/core-safety -C RelWithDebInfo --output-on-failure -V
```

The native workflow runs real 64-bit and 32-bit Linux builds with ASan/UBSan and
Windows builds with MSVC ASan. The current MSVC C11 configuration has no Atomics
API; that absence is logged, and the Linux test requires and exercises Atomics.
RelWithDebInfo avoids MSVC Debug `/RTC1` conflicting with ASan. Windows ASan is
not UBSan or Android/OHOS runtime validation.

VeneraNext integration separately requires rebuilding the plugin, resolving the
full pinned dependency graph and running its Map/TypedData, Promise/cancellation
and engine lifecycle regressions against that new library. Cross-platform
application builds and HarmonyOS packaging remain application acceptance gates.
No executable exploit, host-command payload or host-secret extraction is used.

## Exit condition

Prefer an official stable wrapper/core combination once it passes the same
dependency solver, native core, Dart/FFI and platform acceptance. Otherwise use
an official immutable commit containing the required fixes. Keep this fork only
while those necessary differences remain, and keep historical commits available
for reproducible old builds.
