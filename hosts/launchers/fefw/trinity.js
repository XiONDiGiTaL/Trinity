/**
 * Trinity page adapter for fefw-recomp's Tauri launcher.
 *
 * The host owns the shell. This module only mounts the shared surface, so the
 * existing Play, Setup, Settings and About pages are untouched.
 */
import { renderTrinity } from '../trinity/trinity-ui.js';

export async function render(view) {
  renderTrinity(view, { host: 'fefw-recomp' });
}
