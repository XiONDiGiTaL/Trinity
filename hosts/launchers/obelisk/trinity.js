/**
 * Trinity screen adapter for ObeliskChamber's Tauri launcher.
 *
 * ObeliskChamber treats non-title screens as framed panels. The shared surface
 * detects the host id and drops its radii and soft edges to match that frame.
 */
import { renderTrinity } from '../trinity/trinity-ui.js';

export async function render(view) {
  renderTrinity(view, { host: 'obelisk-chamber' });
}
