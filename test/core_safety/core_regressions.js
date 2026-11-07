for (const method of ['with', 'toReversed', 'toSpliced', 'toSorted']) {
  for (let repetition = 0; repetition < 30; ++repetition) {
    const source = {length: 4};
    for (let index = 0; index < 4; ++index) {
      Object.defineProperty(source, index, {get() {gc(); return {index}}, configurable: true});
    }
    let result;
    if (method === 'with') result = Array.prototype.with.call(source, 1, {index: 9});
    else if (method === 'toSpliced') result = Array.prototype.toSpliced.call(source, 1, 1, {index: 9});
    else if (method === 'toSorted') result = Array.prototype.toSorted.call(source, (a, b) => {gc(); return a.index - b.index});
    else result = Array.prototype.toReversed.call(source);
    gc();
    assert(result.length === 4);
    assert(result.every(value => typeof value.index === 'number'));
    Object.defineProperty(source, '3', {get() {gc(); throw new Error('getter failed')}, configurable: true});
    let failed = false;
    try {
      if (method === 'with') Array.prototype.with.call(source, 1, {});
      else if (method === 'toSpliced') Array.prototype.toSpliced.call(source, 1, 1, {});
      else Array.prototype[method].call(source);
    } catch (error) {failed = error.message === 'getter failed'}
    assert(failed);
  }
}

const keep = [];
for (let index = 0; index < 30; ++index) {
  const array = [{a: 1}, {b: 2}, {c: 3}, {d: 4}, {e: 5}];
  array.length = 2;
  array.length = 5;
  assert(array.push({f: 6}) === 6);
  keep.push(array);
}
gc();
for (const array of keep) {
  assert(array[0].a === 1 && array[1].b === 2 && array[5].f === 6);
  assert(!(2 in array) && !(3 in array) && !(4 in array));
}
assert(new Int32Array([12, 5, 8]).sort((a, b) => {gc(); return a - b}).join() === '5,8,12');
assert(Reflect.ownKeys(new Proxy({a: 1}, {ownKeys() {gc(); return ['a']}})).join() === 'a');

assert(new Uint8Array(copiedBuffer)[0] === 1);
const copiedView = new Uint8Array(copiedBuffer);
const copiedDataView = new DataView(copiedBuffer);
const moved = copiedBuffer.transfer(8);
assert(copiedView.length === 0 && copiedView[0] === undefined);
assert(new Uint8Array(moved).join() === '1,2,3,4,0,0,0,0');
let detached = false;
try {copiedDataView.getUint8(0)} catch (error) {detached = error instanceof TypeError}
assert(detached);
for (const length of [0, 8, 16]) {
  let externalRejected = false;
  try {externalBuffer.transfer(length)} catch (error) {externalRejected = error instanceof TypeError}
  assert(externalRejected && externalBuffer.byteLength === 8);
  assert(new Uint8Array(externalBuffer)[0] === 7);
}
const resizable = new ArrayBuffer(16, {maxByteLength: 32});
const tracked = new Uint8Array(resizable);
const fixed = new Uint8Array(resizable, 4, 4);
resizable.resize(8);
assert(tracked.length === 8 && fixed.length === 4);
const resized = resizable.transfer(12);
assert(tracked.length === 0 && fixed.length === 0);
assert(resized.resizable && resized.maxByteLength === 32);
const newTracked = new Uint8Array(resized);
resized.resize(4);
assert(newTracked.length === 4);
const fixedResult = resized.transferToFixedLength(6);
assert(!fixedResult.resizable && fixedResult.byteLength === 6 && newTracked.length === 0);
const emptyView = new Uint8Array(fixedResult);
assert(fixedResult.transfer(0).byteLength === 0 && emptyView.length === 0);

if (typeof Atomics === 'object') {
  for (let repetition = 0; repetition < 30; ++repetition) {
    const buffer = new ArrayBuffer(8);
    const integers = new BigInt64Array(buffer);
    let rejected = false;
    try {Atomics.store(integers, 0, {valueOf() {buffer.transfer(); return (1n << 100n) + 23n}})}
    catch (error) {rejected = error instanceof TypeError}
    assert(rejected);
  }
}
