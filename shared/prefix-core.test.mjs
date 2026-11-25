import assert from 'node:assert/strict';
import test from 'node:test';
import { classifyInterface, validateRegistry } from './prefix-core.mjs';
import registry from './prefixes.json' with { type: 'json' };

test('the registry is internally consistent', () => {
  assert.deepEqual(validateRegistry(registry), { ok: true, errors: [] });
});

test('host names resolve to their canonical owner', () => {
  const result = classifyInterface('emblemweave.ui.panel', registry);
  assert.equal(result.kind, 'host');
  assert.equal(result.owner, 'emblemweave');
  assert.equal(result.local, 'ui.panel');
});

test('short aliases resolve but report the canonical prefix', () => {
  const result = classifyInterface('oc.ui.settings', registry);
  assert.equal(result.kind, 'host');
  assert.equal(result.owner, 'obelisk-chamber');
  assert.equal(result.prefix, 'obeliskchamber.');
  assert.equal(result.alias, 'oc.');
});

test('neutral namespaces stay portable', () => {
  assert.equal(classifyInterface('trinity.core.panel', registry).kind, 'portable');
  assert.equal(classifyInterface('nxp.ui', registry).kind, 'abi');
});

test('unregistered and malformed names are rejected', () => {
  assert.equal(classifyInterface('other.ui', registry).kind, 'unknown');
  assert.equal(classifyInterface('trinity.', registry).kind, 'invalid');
  assert.equal(classifyInterface('Trinity.ui', registry).kind, 'invalid');
  assert.equal(classifyInterface('trinity..ui', registry).kind, 'invalid');
});

test('a duplicate claim is rejected', () => {
  const broken = {
    neutral: [{ prefix: 'trinity.', owner: 'Trinity' }],
    hosts: [{ id: 'x', prefix: 'trinity.', owner: 'X' }],
    reserved: []
  };
  const result = validateRegistry(broken);
  assert.equal(result.ok, false);
  assert.match(result.errors.join('\n'), /claimed by both/);
});
