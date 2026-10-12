// SPDX-License-Identifier: GPL-3.0-only
// Shared browser audio utilities, adapted from the Felucca/Sloop SMK Studio.
export class BrowserSynth {
  midi(bytes){this.node?.port.postMessage({type:"midi",bytes:Array.from(bytes)});}
  volume(value){this.gain?.gain.setTargetAtTime(Math.max(0,Math.min(1,value)),this.context.currentTime,.015);}
  showScope(canvas) {
    if (this.scopeFrame) cancelAnimationFrame(this.scopeFrame);
    const ctx = canvas.getContext('2d');
    const data = new Float32Array(2048);
    const draw = () => {
      if (!canvas.isConnected || canvas.parentElement.hidden) { this.scopeFrame = null; return; }
      if (!document.hidden) {
        const rect = canvas.getBoundingClientRect(), ratio = Math.min(devicePixelRatio || 1, 2);
        const w = Math.round(rect.width * ratio), h = Math.round(rect.height * ratio);
        if (canvas.width !== w || canvas.height !== h) { canvas.width = w; canvas.height = h; }
        const style = getComputedStyle(canvas);
        ctx.clearRect(0, 0, w, h);
        ctx.strokeStyle = style.getPropertyValue('--scope-grid'); ctx.lineWidth = ratio;
        ctx.beginPath();
        for (let i=1;i<16;i++) { const x=w*i/16; ctx.moveTo(x,0); ctx.lineTo(x,h); }
        for (let i=1;i<4;i++) { const y=h*i/4; ctx.moveTo(0,y); ctx.lineTo(w,y); }
        ctx.stroke();
        data.fill(0);
        if (this.context?.state === 'running') this.analyser?.getFloatTimeDomainData(data);
        // Rising zero-crossing trigger keeps periodic notes visually steady.
        let start=0;
        for(let i=1;i<1024;i++) if(data[i-1]<=0 && data[i]>0.002) { start=i; break; }
        ctx.strokeStyle=style.color; ctx.lineWidth=1.5*ratio;
        ctx.shadowColor=style.color; ctx.shadowBlur=4*ratio;
        ctx.beginPath();
        for(let i=0;i<1024;i++) {
          const x=i*w/1023, y=h/2-Math.max(-1,Math.min(1,data[start+i]))*h*0.46;
          if(i===0) ctx.moveTo(x,y); else ctx.lineTo(x,y);
        }
        ctx.stroke(); ctx.shadowBlur=0;
      }
      this.scopeFrame=requestAnimationFrame(draw);
    };
    draw();
  }
}
