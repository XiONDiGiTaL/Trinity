/*
 * Trinity prefix core. No host APIs and no DOM, so the same rules can run in a
 * launcher page, a Node test or a future host loader.
 *
 * A canonical interface name is a dotted lowercase namespace. The first
 * segment is the owner. The last segment is the local interface name.
 */

const SEGMENT = /^[a-z0-9][a-z0-9-]*$/;

export const PREFIX_STANDARD = 'trinity-prefix/1';

export function parseInterfaceName(name) {
  if (typeof name !== 'string') return { ok: false, reason: 'not a string' };
  const value = name.trim();
  if (!value) return { ok: false, reason: 'empty name' };
  if (value !== value.toLowerCase()) return { ok: false, reason: 'uppercase characters' };
  if (value.endsWith('.')) return { ok: false, reason: 'trailing dot' };
  const parts = value.split('.');
  if (parts.length < 2) return { ok: false, reason: 'a prefix and an interface name are required' };
  if (parts.some((part) => !SEGMENT.test(part))) return { ok: false, reason: 'invalid segment' };
  return { ok: true, value, parts, owner: parts[0], local: parts.slice(1).join('.') };
}

export function classifyInterface(name, registry) {
  const parsed = parseInterfaceName(name);
  if (!parsed.ok) return { kind: 'invalid', owner: null, local: null, reason: parsed.reason };

  const neutral = registry?.neutral ?? [];
  const hosts = registry?.hosts ?? [];
  const reserved = new Set(registry?.reserved ?? []);

  const neutralHit = neutral.find((entry) => parsed.value === entry.prefix.slice(0, -1) || parsed.value.startsWith(entry.prefix));
  if (neutralHit) {
    return {
      kind: neutralHit.prefix === 'nxp.' ? 'abi' : 'portable',
      owner: neutralHit.owner,
      prefix: neutralHit.prefix,
      local: parsed.value.slice(neutralHit.prefix.length),
      value: parsed.value
    };
  }

  for (const host of hosts) {
    const prefixes = [host.prefix, ...(host.aliases ?? [])];
    const hit = prefixes.find((prefix) => parsed.value.startsWith(prefix));
    if (hit) {
      return {
        kind: 'host',
        owner: host.id,
        prefix: host.prefix,
        alias: hit === host.prefix ? null : hit,
        local: parsed.value.slice(hit.length),
        value: parsed.value
      };
    }
  }

  return {
    kind: 'unknown',
    owner: null,
    local: parsed.local,
    value: parsed.value,
    reason: reserved.has(parsed.owner) ? 'reserved first segment' : 'unregistered prefix'
  };
}

export function canonicalPrefix(hostId, registry) {
  const host = (registry?.hosts ?? []).find((entry) => entry.id === hostId);
  return host ? host.prefix : null;
}

export function validateRegistry(registry) {
  const errors = [];
  const seen = new Map();
  const all = [...(registry?.neutral ?? []), ...(registry?.hosts ?? [])];
  for (const entry of all) {
    if (!entry.prefix?.endsWith('.')) errors.push(`${entry.prefix ?? '?'}: prefix must end with a dot`);
    const first = entry.prefix?.slice(0, -1);
    if (!first || !SEGMENT.test(first)) errors.push(`${entry.prefix ?? '?'}: invalid first segment`);
    if (seen.has(entry.prefix)) errors.push(`${entry.prefix}: claimed by both ${seen.get(entry.prefix)} and ${entry.owner}`);
    seen.set(entry.prefix, entry.owner);
    for (const alias of entry.aliases ?? []) {
      if (!alias.endsWith('.')) errors.push(`${alias}: alias must end with a dot`);
      if (seen.has(alias)) errors.push(`${alias}: claimed more than once`);
      seen.set(alias, entry.owner);
    }
  }
  return { ok: errors.length === 0, errors };
}
