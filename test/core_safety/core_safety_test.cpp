#include "quickjs.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <malloc.h>
#include <sstream>
#include <string>

#define CHECK(condition) do { if (!(condition)) { \
  std::cerr << "Failed: " << #condition << " at " << __LINE__ << '\n'; \
  std::abort(); } } while (0)

struct Allocator {
  size_t active = 0;
  size_t calls = 0;
  size_t fail_at = std::numeric_limits<size_t>::max();
  size_t oversized_requests = 0;
  bool failed = false;
};

static bool bounded_request(Allocator* state, size_t count, size_t size) {
  if ((size && count > std::numeric_limits<size_t>::max() / size) ||
      (size && count * size > 64 * 1024 * 1024)) {
    ++state->oversized_requests;
    return false;
  }
  return true;
}

static bool fail(Allocator* state) {
  if (state->calls++ != state->fail_at) return false;
  state->failed = true;
  return true;
}

static void* allocate(void* opaque, size_t size) {
  auto* state = static_cast<Allocator*>(opaque);
  if (!bounded_request(state, 1, size)) return nullptr;
  if (fail(state)) return nullptr;
  auto* result = std::malloc(size);
  if (result) ++state->active;
  return result;
}

static void* allocate_zero(void* opaque, size_t count, size_t size) {
  auto* state = static_cast<Allocator*>(opaque);
  if (!bounded_request(state, count, size)) return nullptr;
  if (fail(state)) return nullptr;
  auto* result = std::calloc(count, size);
  if (result) ++state->active;
  return result;
}

static void release(void* opaque, void* pointer) {
  if (!pointer) return;
  auto* state = static_cast<Allocator*>(opaque);
  CHECK(state->active > 0);
  --state->active;
  std::free(pointer);
}

static void* reallocate(void* opaque, void* pointer, size_t size) {
  auto* state = static_cast<Allocator*>(opaque);
  if (!size) {
    release(opaque, pointer);
    return nullptr;
  }
  if (!bounded_request(state, 1, size)) return nullptr;
  if (fail(state)) return nullptr;
  auto* result = std::realloc(pointer, size);
  if (result && !pointer) ++state->active;
  return result;
}

static size_t usable_size(const void* pointer) {
#ifdef _WIN32
  return _msize(const_cast<void*>(pointer));
#else
  return malloc_usable_size(const_cast<void*>(pointer));
#endif
}

static const JSMallocFunctions allocation = {
  allocate_zero, allocate, release, reallocate, usable_size
};

static JSValue collect(JSContext* context, JSValue, int, JSValue*) {
  JS_RunGC(JS_GetRuntime(context));
  return JS_UNDEFINED;
}

static JSValue require(JSContext* context, JSValue, int argc, JSValue* argv) {
  if (!(argc && JS_ToBool(context, argv[0]) > 0)) {
    JS_ThrowInternalError(context, "core regression assertion failed");
    auto exception = JS_GetException(context);
    auto stack = JS_GetPropertyStr(context, exception, "stack");
    auto* message = JS_ToCString(context, stack);
    std::cerr << (message ? message : "assertion without stack") << '\n';
    JS_FreeCString(context, message);
    JS_FreeValue(context, stack);
    JS_FreeValue(context, exception);
    CHECK(false);
  }
  return JS_UNDEFINED;
}

static JSContext* create_context(JSRuntime* runtime) {
  auto* context = JS_NewContext(runtime);
  CHECK(context);
  auto global = JS_GetGlobalObject(context);
  CHECK(JS_SetPropertyStr(context, global, "gc",
    JS_NewCFunction(context, collect, "gc", 0)) >= 0);
  CHECK(JS_SetPropertyStr(context, global, "assert",
    JS_NewCFunction(context, require, "assert", 1)) >= 0);
  JS_FreeValue(context, global);
  return context;
}

static void clear_exception(JSContext* context) {
  auto exception = JS_GetException(context);
  JS_FreeValue(context, exception);
}

static void check_result(JSContext* context, JSValue value) {
  if (JS_IsException(value)) {
    auto exception = JS_GetException(context);
    auto* message = JS_ToCString(context, exception);
    std::cerr << (message ? message : "native exception") << '\n';
    JS_FreeCString(context, message);
    JS_FreeValue(context, exception);
    CHECK(false);
  }
  JS_FreeValue(context, value);
}

static void drain(JSRuntime* runtime) {
  JSContext* context = nullptr;
  unsigned jobs = 0;
  while (JS_IsJobPending(runtime)) {
    CHECK(++jobs <= 128);
    if (JS_ExecutePendingJob(runtime, &context) < 0) clear_exception(context);
  }
}

static void external_free(JSRuntime*, void* opaque, void* pointer) {
  if (pointer) ++*static_cast<unsigned*>(opaque);
  std::free(pointer);
}

static void regressions(const std::string& script) {
  Allocator state;
  auto* runtime = JS_NewRuntime2(&allocation, &state);
  CHECK(runtime);
  auto* context = create_context(runtime);
  auto global = JS_GetGlobalObject(context);
  auto atomics = JS_GetPropertyStr(context, global, "Atomics");
#ifndef _WIN32
  CHECK(JS_IsObject(atomics));
#endif
  std::cout << (JS_IsObject(atomics) ? "Atomics regression enabled\n" :
    "Atomics unavailable in this MSVC core; Linux CI must cover it\n");
  JS_FreeValue(context, atomics);
  unsigned external_releases = 0;
  auto* bytes = static_cast<uint8_t*>(std::malloc(8));
  CHECK(bytes);
  bytes[0] = 7;
  CHECK(JS_SetPropertyStr(context, global, "externalBuffer",
    JS_NewArrayBuffer(context, bytes, 8, external_free,
      &external_releases, false)) >= 0);
  uint8_t copy[] = {1, 2, 3, 4};
  CHECK(JS_SetPropertyStr(context, global, "copiedBuffer",
    JS_NewArrayBufferCopy(context, copy, sizeof(copy))) >= 0);
  copy[0] = 9;
  check_result(context, JS_Eval(context, script.data(), script.size(),
    "core_regressions.js", JS_EVAL_TYPE_GLOBAL));
  auto external = JS_GetPropertyStr(context, global, "externalBuffer");
  JS_DetachArrayBuffer(context, external);
  CHECK(external_releases == 1);
  JS_FreeValue(context, external);
  JS_FreeValue(context, global);
  drain(runtime);
  JS_FreeContext(context);
  JS_FreeRuntime(runtime);
  CHECK(external_releases == 1);
  CHECK(state.active == 0);
}

static void allocation_failure(const char* script) {
  unsigned failures = 0;
  for (size_t offset = 0; offset < 256; ++offset) {
    Allocator state;
    auto* runtime = JS_NewRuntime2(&allocation, &state);
    CHECK(runtime);
    auto* context = create_context(runtime);
    auto compiled = JS_Eval(context, script, std::strlen(script),
      "allocation_failure.js", JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);
    CHECK(!JS_IsException(compiled));
    state.fail_at = state.calls + offset;
    auto result = JS_EvalFunction(context, compiled);
    if (JS_IsException(result)) clear_exception(context);
    else JS_FreeValue(context, result);
    drain(runtime);
    state.fail_at = std::numeric_limits<size_t>::max();
    failures += state.failed;
    JS_RunGC(runtime);
    JS_FreeContext(context);
    JS_FreeRuntime(runtime);
    CHECK(state.active == 0);
  }
  CHECK(failures > 0);
  std::cout << "Allocation failures exercised: " << failures << '\n';
}

static void allocation_boundaries() {
  Allocator state;
  auto* runtime = JS_NewRuntime2(&allocation, &state);
  CHECK(runtime);
  auto* context = create_context(runtime);
  const auto before = state.calls;
  auto* value = js_calloc_rt(runtime,
    std::numeric_limits<size_t>::max() / 4 + 1, 4);
  CHECK(value == nullptr);
  CHECK(state.calls == before);
  value = js_calloc(context, std::numeric_limits<size_t>::max() / 4 + 1, 4);
  CHECK(value == nullptr);
  CHECK(state.oversized_requests == 0);
  clear_exception(context);
  JS_SetMemoryLimit(runtime, 256 * 1024);
  const char* source = "Reflect.ownKeys(new Proxy({}, {ownKeys() {return {"
    "length: 1073741825, get 0() {throw new Error('keys were read')}}}}))";
  auto result = JS_Eval(context, source, std::strlen(source),
    "proxy_boundary.js", JS_EVAL_TYPE_GLOBAL);
  CHECK(JS_IsException(result));
  auto exception = JS_GetException(context);
  auto* message = JS_ToCString(context, exception);
  CHECK(message && std::string(message).find("keys were read") == std::string::npos);
  CHECK(state.oversized_requests == 0);
  JS_FreeCString(context, message);
  JS_FreeValue(context, exception);
  JS_SetMemoryLimit(runtime, 0);
  JS_FreeContext(context);
  JS_FreeRuntime(runtime);
  CHECK(state.active == 0);
}

int main(int argc, char** argv) {
  CHECK(argc == 3);
  CHECK(std::atoi(argv[2]) == sizeof(void*) * 8);
  std::ifstream input(argv[1]);
  CHECK(input.good());
  std::ostringstream contents;
  contents << input.rdbuf();
  regressions(contents.str());
  allocation_boundaries();
  allocation_failure("new Promise((resolve) => resolve({value: 1})).then(() => {});");
  allocation_failure("(async () => {await 1; return {value: 2}})().then(() => {});");
  allocation_failure("(async function* () {yield await 1})().next().then(() => {});");
  allocation_failure("Error.prepareStackTrace = (error, frames) => frames;"
    "function recurse(n) {return n ? recurse(n - 1) : new Error('bounded').stack}"
    "recurse(3);");
  allocation_failure("const buffer = new ArrayBuffer(8); new Uint8Array(buffer)[0] = 7;"
    "try {buffer.transfer(16)} catch (error) {"
    "assert(buffer.byteLength === 8); assert(new Uint8Array(buffer)[0] === 7)};");
  std::cout << "Native core safety passed on " << sizeof(void*) * 8 << " bits\n";
}
