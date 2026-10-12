// SPDX-License-Identifier: GPL-3.0-only
// Product finish animation only. No firmware selection or MIDI access.
(() => {
 const instrument=document.querySelector('.smk-instrument');
 if(!instrument)return;
 const finishes=[['red','Pro \u00b7 Red'],['white','Pro \u00b7 White'],['elite','Elite \u00b7 Slate / orange'],
  ['black','Pro \u00b7 All black'],['pink','Pro \u00b7 Pink'],['charcoal','Pro \u00b7 Charcoal']];
 const motion=matchMedia('(prefers-reduced-motion: reduce)');
 const layers=[...instrument.querySelectorAll('.smk-finish-layer')];
 let timer,index=0,active=0;
 function advance(){
  index=(index+1)%finishes.length;
  const [finish,name]=finishes[index];
  const next=1-active;
  layers[next].dataset.finish=finish;
  layers[next].dataset.model=finish==='elite'?'elite':'pro';
  layers[next].classList.add('is-visible');
  layers[active].classList.remove('is-visible');
  layers[next].removeAttribute('aria-hidden');
  layers[active].setAttribute('aria-hidden','true');
  active=next;
  instrument.dataset.currentFinish=finish;
  document.getElementById('smk-finish-caption').textContent=`SMK-37 ${name}`;
  document.getElementById('smk-hero-title').textContent=`SMK-37 ${name}`;
 }
 function schedule(){
  clearInterval(timer);
  if(!motion.matches&&!document.hidden)timer=setInterval(advance,5000);
 }
 document.addEventListener('visibilitychange',schedule);
 motion.addEventListener('change',schedule);
 schedule();
})();
