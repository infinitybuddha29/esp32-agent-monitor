import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {fileURLToPath} from 'node:url';
const exec = promisify(execFile);
const home = os.homedir();
const root = process.env.CODEX_SESSIONS_DIR || path.join(home,'.codex/sessions');
const stale = Number(process.env.STALE_MS || 300000);
const retain = Number(process.env.RETAIN_MS || 1800000);
export function displayTitle(title, fallback) {
  const clean=typeof title==='string' ? title.replace(/[\x00-\x1f\x7f]/g,' ').replace(/\s+/g,' ').trim() : '';
  return [...(clean || fallback)].slice(0,96).join('');
}
function codexTitles() {
  const titles=new Map();
  try {for(const line of fs.readFileSync(process.env.CODEX_INDEX_FILE || path.join(path.dirname(root),'session_index.jsonl'),'utf8').split('\n')) {
    try {const row=JSON.parse(line);if(typeof row.id==='string' && typeof row.thread_name==='string') titles.set(row.id,row.thread_name);}catch{}
  }}catch{}
  return titles;
}
export function codexTransition(record) {
  const p=record.payload || {};
  if (record.type !== 'event_msg') return null;
  return ({task_started:'WORKING', task_complete:'DONE', turn_aborted:'UNKNOWN', error:'ERROR'})[p.type] || null;
}
export function visibleStatus(status, time, now=Date.now(), limit=stale) {
  if(status==='DONE' || status==='ERROR') return status;
  return now-time > limit ? 'UNKNOWN' : status;
}
export function foldCodex(text, now=Date.now()) {
  let id, project='unknown', status='UNKNOWN', updatedAt, startedAt, lastActivity=0;
  for (const line of text.split('\n')) {
    if (!line) continue;
    let r; try { r=JSON.parse(line); } catch { continue; }
    if (r.type==='session_meta') { id=r.payload?.id; project=path.basename(r.payload?.cwd || '') || 'unknown'; }
    const s=codexTransition(r); const t=Date.parse(r.timestamp);
    if(Number.isFinite(t)) lastActivity=Math.max(lastActivity,t);
    if(s==='WORKING') startedAt=r.timestamp;
    if (s && Number.isFinite(t)) { status=s; updatedAt=t; }
  }
  if (!id || !updatedAt || now-updatedAt>retain) return null;
  return {id,provider:'codex',project,status:visibleStatus(status,lastActivity,now),updatedAt:new Date(updatedAt).toISOString(),startedAt};
}
function findRecent(dir, now, out=[]) {
  let entries; try {entries=fs.readdirSync(dir,{withFileTypes:true});} catch{return out;}
  for (const e of entries) { const p=path.join(dir,e.name); if(e.isDirectory()) findRecent(p,now,out); else if(e.name.startsWith('rollout-')&&e.name.endsWith('.jsonl')) {try {if(now-fs.statSync(p).mtimeMs<retain) out.push(p);}catch{}} }
  return out;
}
async function api(url, directory) {
  const args=['api','get',url]; if(directory) args.push('--header',`x-opencode-directory:${directory}`);
  const {stdout}=await exec(process.env.OPENCODE_BIN || 'opencode',args,{timeout:12000,maxBuffer:5*1024*1024});
  const parsed=JSON.parse(stdout); return parsed.data ?? parsed;
}
export function openStatus(active, waiting) {
  if(waiting) return 'WAITING';
  if(active?.type==='running' || active?.type==='busy' || active?.type==='retry') return 'WORKING';
  if(active?.type==='idle') return 'IDLE';
  return 'UNKNOWN';
}
let oc=new Map();
async function pollOpenCode() {
  // Discover only a running service; unlike `opencode api` alone this avoids starting one.
  await exec(process.env.OPENCODE_BIN || 'opencode',['service','status'],{timeout:5000});
  const locations=await api('/api/debug/location');
  const next=new Map();
  for (const location of locations) {
    const dir=location.directory; if(!dir) continue;
    const [active,permissions,forms]=await Promise.all([api('/api/session/active',dir),api('/api/permission/request',dir),api('/api/form',dir)]);
    const pending=[...(Array.isArray(permissions)?permissions:[]),...(Array.isArray(forms)?forms:[])];
    const ids=new Set([...Object.keys(active || {}),...pending.map(x=>x.sessionID || x.sessionId).filter(Boolean),...([...oc.values()].filter(x=>x.directory===dir).map(x=>x.id))]);
    for(const id of ids) {
      const detail=await api('/api/session/'+encodeURIComponent(id),dir);
      const isWaiting=pending.some(x=>(x.sessionID||x.sessionId)===id);
      const status=openStatus(active?.[id],isWaiting)==='UNKNOWN' ? outcomeStatus(detail) : openStatus(active?.[id],isWaiting);
      const old=oc.get(id); const updatedAt=old?.status===status ? old.updatedAt : new Date().toISOString();
      const project=path.basename(detail.location?.directory || dir)||'unknown';
      next.set(id,{id,provider:'opencode',project,title:displayTitle(detail.title,project),status,updatedAt,startedAt:detail.time?.busy ? new Date(detail.time.busy).toISOString() : undefined,directory:dir});
    }
  }
  oc=next;
}
let codex=[]; let scanning=false, polling=false;
export function outcomeStatus(detail) {
  if(!detail.time?.idle || detail.time.idle < (detail.time.busy || 0)) return 'UNKNOWN';
  return ({succeeded:'DONE',failed:'ERROR',interrupted:'UNKNOWN'})[detail.outcome] || 'UNKNOWN';
}
export function orderAgents(agents) {
  const priority={WAITING:0,ERROR:1,WORKING:2,IDLE:3,DONE:4};
  const titles=codexTitles();
  return agents.filter(a=>a.status!=='UNKNOWN').sort((a,b)=>priority[a.status]-priority[b.status] || Date.parse(b.updatedAt)-Date.parse(a.updatedAt)).slice(0,80).map(({directory,...a})=>({...a,title:displayTitle(a.provider==='codex'?titles.get(a.id):a.title,a.project || '?'),elapsedSeconds:a.status==='WORKING' && a.startedAt ? Math.max(0,Math.floor((Date.now()-Date.parse(a.startedAt))/1000)) : null}));
}
let system={clock:'--:--',vpn:'UNKNOWN',internet:'UNKNOWN',battery:null,charging:false};
async function pollSystem() {
  const [vpn,battery]=await Promise.allSettled([exec('/usr/sbin/scutil',['--nc','list'],{timeout:3000}),exec('/usr/bin/pmset',['-g','batt'],{timeout:3000})]);
  system.vpn=vpn.status==='fulfilled' ? (/\(Connected\).*"Happ"/i.test(vpn.value.stdout)?'ON':/Happ/i.test(vpn.value.stdout)?'OFF':'UNKNOWN') : 'UNKNOWN';
  const b=battery.status==='fulfilled' ? battery.value.stdout : '';
  system.battery=b.match(/(\d+)%;/) ? Number(b.match(/(\d+)%;/)[1]) : null;
  system.charging=/AC Power/.test(b);
  try {const r=await fetch('https://www.apple.com/library/test/success.html',{signal:AbortSignal.timeout(4000),redirect:'error'}); system.internet=r.ok && (await r.text()).includes('<TITLE>Success</TITLE>') ? 'ON':'OFF';}catch{system.internet='OFF';}
}
function scan() {if(scanning)return; scanning=true; try {const now=Date.now(); const next=new Map(codex.filter(a=>a.status==='DONE'||a.status==='ERROR').map(a=>[a.id,a])); for(const p of findRecent(root,now)){try{const a=foldCodex(fs.readFileSync(p,'utf8'),now);if(a)next.set(a.id,a);}catch{}} codex=[...next.values()].sort((a,b)=>Date.parse(b.updatedAt)-Date.parse(a.updatedAt)).slice(0,80);}finally{scanning=false;}}
async function poll() {if(polling)return; polling=true; try {await pollOpenCode();} catch {oc=new Map([...oc].map(([id,v])=>[id,{...v,status:'UNKNOWN'}]));} finally {polling=false;}}
export function start() {
  scan(); void poll(); void pollSystem(); setInterval(scan,2000); setInterval(poll,3000); setInterval(()=>void pollSystem(),30000);
  const host=process.env.HOST || 'auto', port=Number(process.env.PORT || 8787);
  const server=http.createServer((req,res)=>{ if(req.method!=='GET'||req.url!=='/api/agents'){res.writeHead(404);res.end();return;} const agents=orderAgents([...oc.values(),...codex]); const body=JSON.stringify({agents,system:{...system,clock:new Date().toLocaleTimeString('ru-RU',{hour:'2-digit',minute:'2-digit'})}}); res.writeHead(200,{'Content-Type':'application/json','Content-Length':Buffer.byteLength(body),'Cache-Control':'no-store','X-Content-Type-Options':'nosniff'});res.end(body); });
  let binding=false;
  const rebind=()=>{
    if(binding)return;
    const address=host==='auto' ? lanAddress(os.networkInterfaces(),process.env.LAN_INTERFACE || 'en0') : host;
    if((server.address()?.address || null)===address)return;
    binding=true;
    const listen=()=>{
      if(!address){binding=false;return;}
      server.listen(port,address,()=>{binding=false;console.log(`Agent Bridge listening on http://${address}:${port}/api/agents`);});
    };
    if(server.listening){server.close(listen);server.closeAllConnections();}else listen();
  };
  server.on('error',error=>{binding=false;console.error(`HTTP listener: ${error.code || 'error'}`);});
  rebind();setInterval(rebind,5000);
}
export function lanAddress(interfaces, name='en0') {
  return interfaces[name]?.find(x=>x.family==='IPv4' && !x.internal && /^(10\.|192\.168\.|172\.(1[6-9]|2\d|3[01])\.)/.test(x.address))?.address || null;
}
if(process.argv[1] && path.resolve(process.argv[1])===fileURLToPath(import.meta.url)) start();
