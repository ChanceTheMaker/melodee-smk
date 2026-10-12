// SPDX-License-Identifier: GPL-3.0-only
// Hardware labels match the panel; explanations use the shared language catalog.
(() => {
  const guide = document.querySelector('[data-panel-guide]') || document.getElementById('fm1-guide');
  if (!guide) return;
  const buttons = [...guide.querySelectorAll('[data-guide-section]')];
  const panels = [...guide.querySelectorAll('[data-guide-panel]')];
  for (const button of buttons) {
    button.addEventListener('click', () => {
      const section = button.dataset.guideSection;
      guide.dataset.section = section;
      for (const item of buttons) item.setAttribute('aria-pressed', String(item === button));
      for (const panel of panels) panel.hidden = panel.dataset.guidePanel !== section;
    });
  }
})();
