/*
 * Trinity launcher surface.
 *
 * One catalog, one prefix registry, three presentations. The host supplies the
 * frame, type, colours, scroll container and input model. This module supplies
 * only the Trinity content and the shared classification behaviour.
 */
import catalog from './catalog.json';
import prefixes from './prefixes.json';
import { classifyInterface, validateRegistry } from './prefix-core.mjs';

const HOST_COPY = {
  'fefw-recomp': {
    kicker: 'HOUSE OF RECOMPILATION',
    intro: 'Cool surfaces, soft corners and the existing fefw-recomp navigation. Trinity adds a room, not a second launcher.',
    note: 'Built from the upstream launcher source. The packaged 0.2.13 runtime was not rebuilt for this test.'
  },
  'obelisk-chamber': {
    kicker: 'HOUSE OF THE OBELISK',
    intro: 'A framed panel in the manner of ObeliskChamber: title-screen navigation, angular controls and gold rules around the same shared catalog.',
    note: 'Built from the upstream launcher source. The packaged 0.3.2 runtime was not rebuilt for this test.'
  },
  emblemweave: {
    kicker: 'HOUSE OF WEAVING',
    intro: 'A native egui page in the existing EmblemWeave rail. The shared registry is read by the page, while the launcher keeps its own widgets and themes.',
    note: 'The NXP proof panel was loaded by the EmblemWeave development build.'
  }
};

const ICONS = {
  spark: '<path d="M12 2.8l1.8 5.4L19.2 10l-5.4 1.8L12 17.2l-1.8-5.4L4.8 10l5.4-1.8L12 2.8Z"/><path d="M19 16.5l.8 2.2 2.2.8-2.2.8-.8 2.2-.8-2.2-2.2-.8 2.2-.8.8-2.2Z"/>',
  cube: '<path d="M12 2.8l8 4.6v9.2l-8 4.6-8-4.6V7.4l8-4.6Z"/><path d="M4.3 7.6L12 12l7.7-4.4M12 12v9"/>',
  layers: '<path d="M12 3l9 5-9 5-9-5 9-5Z"/><path d="M3 12l9 5 9-5M3 16l9 5 9-5"/>',
  terminal: '<rect x="3" y="4" width="18" height="16" rx="2"/><path d="M7 9l3 3-3 3M13 15h4"/>',
  search: '<circle cx="11" cy="11" r="7"/><path d="M20 20l-4-4"/>',
  arrow: '<path d="M5 12h14M14 7l5 5-5 5"/>',
  check: '<path d="M20 6L9 17l-5-5"/>',
  key: '<circle cx="8" cy="15" r="4"/><path d="M11 12l9-9M17 6l3 3M14 9l2 2"/>',
  compass: '<circle cx="12" cy="12" r="9"/><path d="M15.5 8.5l-2.3 4.7-4.7 2.3 2.3-4.7 4.7-2.3Z"/>'
};

const icon = (name) => `<svg class="trinity-icon" viewBox="0 0 24 24" aria-hidden="true" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">${ICONS[name] ?? ICONS.cube}</svg>`;
const esc = (value) => String(value ?? '').replace(/[&<>"']/g, (char) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[char]));
const labelForHost = (id) => catalog.houses.find((house) => house.id === id)?.name ?? id;

function mark(size = 128, className = 'trinity-mark') {
  return `<svg class="${className}" width="${size}" height="${size}" viewBox="0 0 256 256" aria-hidden="true">
    <defs>
      <linearGradient id="trinity-gold-${size}" x1="38" y1="24" x2="214" y2="232" gradientUnits="userSpaceOnUse"><stop stop-color="#f7e3a1"/><stop offset=".5" stop-color="#d9b45a"/><stop offset="1" stop-color="#9b6f22"/></linearGradient>
      <linearGradient id="trinity-violet-${size}" x1="46" y1="72" x2="210" y2="188" gradientUnits="userSpaceOnUse"><stop stop-color="#54218f"/><stop offset="1" stop-color="#9a68ee"/></linearGradient>
      <linearGradient id="trinity-blue-${size}" x1="52" y1="184" x2="210" y2="70" gradientUnits="userSpaceOnUse"><stop stop-color="#2a6cff"/><stop offset="1" stop-color="#8bc2ff"/></linearGradient>
      <linearGradient id="trinity-teal-${size}" x1="42" y1="128" x2="214" y2="128" gradientUnits="userSpaceOnUse"><stop stop-color="#0f6f78"/><stop offset="1" stop-color="#39d0c8"/></linearGradient>
    </defs>
    <path d="M147.5 45.7a37 37 0 0 0-52.3 45.4l-47.7 47.7a18.7 18.7 0 1 0 26.5 26.5l47.7-47.7a37 37 0 0 0 45.4-52.3l-24.8 24.8-19.6-4.9-4.9-19.6 24.7-24.9Z" fill="none" stroke="url(#trinity-gold-${size})" stroke-width="12" stroke-linejoin="round"/>
    <g fill="none" stroke-linecap="round">
      <ellipse cx="128" cy="128" rx="79" ry="34" stroke="#8a6420" stroke-width="13"/>
      <ellipse cx="128" cy="128" rx="79" ry="34" transform="rotate(60 128 128)" stroke="#8a6420" stroke-width="13"/>
      <ellipse cx="128" cy="128" rx="79" ry="34" transform="rotate(120 128 128)" stroke="#8a6420" stroke-width="13"/>
      <ellipse cx="128" cy="128" rx="79" ry="34" stroke="url(#trinity-violet-${size})" stroke-width="7"/>
      <ellipse cx="128" cy="128" rx="79" ry="34" transform="rotate(60 128 128)" stroke="url(#trinity-blue-${size})" stroke-width="7"/>
      <ellipse cx="128" cy="128" rx="79" ry="34" transform="rotate(120 128 128)" stroke="url(#trinity-teal-${size})" stroke-width="7"/>
    </g>
    <circle cx="128" cy="128" r="10" fill="#08090c" stroke="url(#trinity-gold-${size})" stroke-width="3"/>
  </svg>`;
}

function hostCard(house) {
  return `<article class="trinity-house" data-tone="${esc(house.tone)}" style="--house-accent:${esc(house.accent)}">
    <div class="trinity-house-top">
      <span class="trinity-monogram">${esc(house.monogram)}</span>
      <span class="trinity-status"><i></i>${esc(house.status)}</span>
    </div>
    <div class="trinity-house-name">${esc(house.name)}</div>
    <div class="trinity-house-note">${esc(house.house)}</div>
    <dl class="trinity-house-meta">
      <div><dt>Prefix</dt><dd><code>${esc(house.prefix)}</code></dd></div>
      <div><dt>Host API</dt><dd>${esc(house.host_api)}</dd></div>
      <div><dt>Surface</dt><dd>${esc(house.launcher)}</dd></div>
    </dl>
    <p>${esc(house.evidence)}</p>
  </article>`;
}

function listingRow(item) {
  return `<article class="trinity-listing" data-kind="${esc(item.kind)}" data-hosts="${esc(item.hosts.join(' '))}" data-search="${esc(`${item.id} ${item.name} ${item.summary} ${item.kind} ${item.hosts.map(labelForHost).join(' ')}`.toLowerCase())}">
    <div class="trinity-listing-main">
      <div class="trinity-listing-id">${esc(item.id)}</div>
      <h3>${esc(item.name)}</h3>
      <p>${esc(item.summary)}</p>
    </div>
    <div class="trinity-listing-side">
      <span class="trinity-kind">${esc(item.kind)}</span>
      <span class="trinity-version">v${esc(item.version)}</span>
      <span class="trinity-listing-status" data-tone="${esc(item.tone)}">${esc(item.status)}</span>
      <div class="trinity-hosts">${item.hosts.map((id) => `<span title="${esc(labelForHost(id))}">${esc(catalog.houses.find((h) => h.id === id)?.monogram ?? id)}</span>`).join('')}</div>
    </div>
  </article>`;
}

function prefixRows() {
  return [
    ...prefixes.neutral.map((entry) => ({ ...entry, id: entry.owner, kind: entry.prefix === 'nxp.' ? 'ABI' : 'Portable' })),
    ...prefixes.hosts.map((entry) => ({ ...entry, owner: labelForHost(entry.id), kind: 'Host' }))
  ].map((entry) => `<div class="trinity-prefix-row"${entry.accent ? ` style="--prefix-accent:${esc(entry.accent)}"` : ''}>
    <code>${esc(entry.prefix)}</code>
    <span>${esc(entry.owner)}</span>
    <span class="trinity-kind">${esc(entry.kind)}</span>
  </div>`).join('');
}

export function renderTrinity(view, options = {}) {
  const hostId = options.host ?? 'file';
  const copy = HOST_COPY[hostId] ?? {
    kicker: 'CROSS-HOST PLUGIN LAYER',
    intro: 'One catalog, three native presentations, and a namespace that tells every interface which house it belongs to.',
    note: 'Standalone preview of the shared Trinity surface.'
  };
  const current = catalog.houses.find((house) => house.id === hostId);
  const validation = validateRegistry(prefixes);
  const total = catalog.listings.length;

  view.innerHTML = `<div class="page trinity-page" data-trinity-host="${esc(hostId)}"${current ? ` style="--t-host-accent:${esc(current.accent)}"` : ''}>
    <section class="trinity-hero">
      <div class="trinity-hero-copy">
        <div class="trinity-eyebrow"><i></i>${esc(copy.kicker)} <span>/</span> NXP LAYER ${esc(catalog.version)}</div>
        <div class="trinity-wordmark" aria-label="Trinity">TRINITY</div>
        <p class="trinity-tagline">${esc(catalog.tagline)}</p>
        <p class="trinity-intro">${esc(copy.intro)}</p>
        <div class="trinity-badges">
          <span>${icon('cube')} 3 houses</span>
          <span>${icon('layers')} 1 catalog</span>
          <span>${icon('key')} ${prefixes.hosts.length} prefixes</span>
          <span class="${validation.ok ? 'ok' : 'warn'}">${icon(validation.ok ? 'check' : 'compass')} ${validation.ok ? 'Registry valid' : 'Registry needs attention'}</span>
        </div>
        <button type="button" class="trinity-jump" id="trinity-jump">${icon('arrow-down')} Browse the registry</button>
      </div>
      <div class="trinity-hero-mark">
        ${mark(184)}
        <span class="trinity-orbit orbit-one"></span>
        <span class="trinity-orbit orbit-two"></span>
        <div class="trinity-host-chip">${esc(current?.monogram ?? 'T')}<span>${esc(current?.name ?? 'Shared surface')}</span></div>
      </div>
    </section>

    <section class="trinity-metrics" aria-label="Trinity at a glance">
      <div><strong>${catalog.houses.length}</strong><span>Houses connected</span><em>one shared contract</em></div>
      <div><strong>${total}</strong><span>Listings</span><em>runtimes, bridges, standards</em></div>
      <div><strong>${prefixes.hosts.length + prefixes.neutral.length}</strong><span>Owned prefixes</span><em>no silent collisions</em></div>
      <div><strong>${catalog.capabilities.length}</strong><span>Core capabilities</span><em>UI, settings, files, frame</em></div>
    </section>

    <section class="trinity-section">
      <div class="trinity-section-head">
        <div><span class="trinity-kicker">Browse by house</span><h2>Three rooms, one door</h2></div>
        <span class="trinity-count">${catalog.houses.length} hosts <b>one shared catalog</b></span>
      </div>
      <div class="trinity-houses">${catalog.houses.map(hostCard).join('')}</div>
    </section>

    <section class="trinity-section" id="trinity-registry">
      <div class="trinity-section-head">
        <div><span class="trinity-kicker">Registry</span><h2>What Trinity carries</h2></div>
        <span class="trinity-count" id="trinity-result-count">${total} listings</span>
      </div>
      <div class="trinity-toolbar">
        <label class="trinity-search">${icon('search')}<input id="trinity-search" type="search" placeholder="Search listings, hosts or prefixes" autocomplete="off" spellcheck="false"></label>
        <div class="trinity-filters" role="group" aria-label="Filter listings">
          <button type="button" class="active" data-filter="all">All</button>
          <button type="button" data-filter="runtime">Runtime</button>
          <button type="button" data-filter="launcher">Launcher</button>
          <button type="button" data-filter="standard">Standard</button>
          <button type="button" data-filter="bridge">Bridge</button>
        </div>
      </div>
      <div class="trinity-list">${catalog.listings.map(listingRow).join('')}</div>
      <p class="trinity-empty" id="trinity-empty" hidden>No listing matches that filter yet.</p>
    </section>

    <section class="trinity-lower">
      <div class="trinity-panel trinity-prefix-panel">
        <div class="trinity-panel-head">
          <span class="trinity-panel-icon">${icon('key')}</span>
          <div><span class="trinity-kicker">Prefix registry</span><h2>Name the owner before the interface</h2></div>
        </div>
        <p class="trinity-panel-copy">The issue #165 proposal, made concrete: a host claims one prefix, portable services use <code>trinity.</code>, and the ABI keeps <code>nxp.</code>. Existing symbols stay valid.</p>
        <div class="trinity-prefix-table">${prefixRows()}</div>
        <div class="trinity-checker">
          <label for="trinity-prefix-check">Try an interface name</label>
          <input id="trinity-prefix-check" type="text" value="emblemweave.ui.panel" autocomplete="off" spellcheck="false">
          <div id="trinity-prefix-result" class="trinity-check-result" aria-live="polite"></div>
        </div>
      </div>

      <div class="trinity-panel trinity-contract-panel">
        <div class="trinity-panel-head">
          <span class="trinity-panel-icon">${icon('terminal')}</span>
          <div><span class="trinity-kicker">Compatibility contract</span><h2>Additive by design</h2></div>
        </div>
        <ul class="trinity-rules">
          ${catalog.capabilities.map((capability) => `<li><span>${icon('check')}</span><div><strong>${esc(capability.name)}</strong><p>${esc(capability.detail)}</p></div></li>`).join('')}
        </ul>
        <div class="trinity-note">${esc(copy.note)}</div>
      </div>
    </section>

    <footer class="trinity-footer">
      <span><i class="${validation.ok ? 'ok' : 'warn'}"></i>${validation.ok ? 'Prefix registry consistent' : 'Prefix registry has errors'}</span>
      <span>Catalog ${esc(catalog.schema)} / ${esc(catalog.revision)}</span>
      <span class="trinity-footer-link">github.com/XiONDiGiTaL/Trinity</span>
    </footer>
  </div>`;

  const search = view.querySelector('#trinity-search');
  const list = view.querySelector('.trinity-list');
  const empty = view.querySelector('#trinity-empty');
  const count = view.querySelector('#trinity-result-count');
  let kind = 'all';

  const applyFilter = () => {
    const query = search.value.trim().toLowerCase();
    let shown = 0;
    for (const row of list.querySelectorAll('.trinity-listing')) {
      const kindMatch = kind === 'all' || row.dataset.kind === kind;
      const queryMatch = !query || row.dataset.search.includes(query);
      row.hidden = !(kindMatch && queryMatch);
      if (!row.hidden) shown += 1;
    }
    empty.hidden = shown !== 0;
    count.textContent = `${shown} ${shown === 1 ? 'listing' : 'listings'}`;
  };

  view.querySelector('#trinity-jump')?.addEventListener('click', () => {
    view.querySelector('#trinity-registry')?.scrollIntoView({ behavior: 'smooth', block: 'start' });
  });

  search.addEventListener('input', applyFilter);
  view.querySelectorAll('[data-filter]').forEach((button) => {
    button.addEventListener('click', () => {
      kind = button.dataset.filter;
      view.querySelectorAll('[data-filter]').forEach((other) => other.classList.toggle('active', other === button));
      applyFilter();
    });
  });

  const checker = view.querySelector('#trinity-prefix-check');
  const checkerResult = view.querySelector('#trinity-prefix-result');
  const check = () => {
    const result = classifyInterface(checker.value, prefixes);
    const tone = result.kind === 'invalid' || result.kind === 'unknown' ? 'warn' : 'ok';
    const owner = result.kind === 'host' ? labelForHost(result.owner) : result.owner;
    checkerResult.dataset.tone = tone;
    checkerResult.innerHTML = result.kind === 'invalid' || result.kind === 'unknown'
      ? `<span>${icon('compass')}</span><div><strong>Not accepted</strong><p>${esc(result.reason)}.</p></div>`
      : `<span>${icon('check')}</span><div><strong>${esc(result.kind.toUpperCase())} / ${esc(owner)}</strong><p>Local name <code>${esc(result.local)}</code>${result.alias ? `, canonical prefix <code>${esc(result.prefix)}</code>` : ''}.</p></div>`;
  };
  checker.addEventListener('input', check);
  check();

  return view.querySelector('.trinity-page');
}
