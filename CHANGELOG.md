<!--
 * @Description: 
 * @Author: ekibun
 * @Date: 2020-08-08 08:16:50
 * @LastEditors: ekibun
 * @LastEditTime: 2020-12-02 11:36:40
-->

## Unreleased

* 回移已核实的 QuickJS-NG 核心安全修复：新数组在 getter／GC 重入前初始化槽位；
  CallSite 和 Promise／async 的分配失败清理保留正确的所有权；Atomics.store 的
  缓冲失效路径释放转换结果，TypedArray sort 与 Proxy 检查真实字节分配溢出。
* ArrayBuffer.transfer 拒绝外部 backing store，清空旧 TypedArray 视图，并在移动
  缓冲前分配目标元数据；分配失败保留原 backing store。快数组增长限定元素／
  字节范围。保留 Dart Map／TypedData／dispatch／port API 与 0.3.7 包版本。
* 增加有界 getter／GC、真实分配失败、外部缓冲、视图转移和 32 位边界原生回归；
  修复来源与验证范围记录在 CORE_PATCHES.md／JSON，CI 核对完整核心源码摘要。
  VERSION 改为真实 d07f6b5 基线的本地补丁标识。旧泛型 push 未含后来引入的优化缺陷。

* Skip typed numeric buffers when recursively retaining and releasing JS
  references. Promise byte results no longer allocate ordinary lists or scan
  every byte solely for reference management.
* Preserve normal list/map traversal, aliases, cycles, callback ownership, and
  Promise completion behavior. Add regression coverage for byte-buffer views,
  signed and floating-point buffers, ByteData, and mixed cyclic reference graphs.
* This fork patch changes no native code, platform build settings, or package
  version. VeneraNext host measurements are not Android frame-pacing acceptance.

## 0.3.7

* add timeout and memory limit
* fixed compiler error in windows release
* fixed crash when encoding Error object
* updated to latest quickjs

## 0.3.6

* upgrade ffi to 1.0.0.
* nullsafety.

## 0.3.5

* downgrade ffi to 0.1.3.

## 0.3.4

* upgrade ffi to 1.0.0.

## 0.3.3

* remove `JSInvokable.call`.
* fix crash when throw error.
* add reference count and leak detection.

## 0.3.2

* fix Promise reject cannot get Exception string.
* wrap JSError.

## 0.3.1

* code clean up.
* fix isolate wrap error.

## 0.3.0

* breakdown change to remove `channel`.
* convert dart function to js.

## 0.2.7

* fix error in ios build.

## 0.2.6

* fix stack overflow in jsToCString.

## 0.2.5

* remove dart object when jsfree.

## 0.2.4

* wrap dart object to js.
* fix stack overflow when use jsCall nesting.

## 0.2.3

* fix compiler error in windows release.

## 0.2.2

* add option to change max stack size.

## 0.2.1

* code cleanup.

## 0.2.0

* breakdown change with new constructor.
* fix make release in ios.
* fix crash in wrapping js Promise.

## 0.1.4

* fix crash on android x86.

## 0.1.3

* fix randomly crash by stack overflow.

## 0.1.2

* fix qjs memory leak.

## 0.1.1

* run on isolate.

## 0.1.0

* refactor with ffi.

## 0.0.6

* remove handler when destroy.

## 0.0.5

* add js module.

## 0.0.4

* remove C++ std limitation for linux and android.

## 0.0.3

* fix js memory leak.

## 0.0.2

* update example.

## 0.0.1

* initial publish.
