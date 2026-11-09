import {test} from 'node:test';
import assert from 'node:assert/strict';
import {codexTransition,foldCodex,visibleStatus,openStatus,outcomeStatus,orderAgents,lanAddress,displayTitle} from '../bridge.mjs';
test('chat titles are bounded, sanitized, with folder fallback',()=>{
  assert.equal(displayTitle('  Тесты\n оплаты  ','folder'),'Тесты оплаты');
  assert.equal(displayTitle('', 'folder'),'folder');
  assert.equal(displayTitle(undefined,'folder'),'folder');
  assert.equal([...displayTitle('Я'.repeat(200),'folder')].length,96);
});
test('LAN binding follows Wi-Fi DHCP, never tunnel or public interfaces',()=>{
  const net=ip=>({en0:[{family:'IPv4',address:ip,internal:false}],utun14:[{family:'IPv4',address:'10.9.0.1',internal:false}]});
  assert.equal(lanAddress(net('192.168.0.158')),'192.168.0.158');
  assert.equal(lanAddress(net('192.168.0.195')),'192.168.0.195');
  assert.equal(lanAddress(net('8.8.8.8')),null);
  assert.equal(lanAddress({}),null);
});
test('done remains confirmed after timeout',()=>assert.equal(visibleStatus('DONE',0,999999),'DONE'));
test('explicit OpenCode outcome only',()=>{
  assert.equal(outcomeStatus({outcome:'succeeded',time:{busy:10,idle:20}}),'DONE');
  assert.equal(outcomeStatus({outcome:'succeeded',time:{busy:30,idle:20}}),'UNKNOWN');
  assert.equal(outcomeStatus({time:{idle:20}}),'UNKNOWN');
  assert.equal(outcomeStatus({outcome:'failed',time:{busy:10,idle:20}}),'ERROR');
});
test('attention first, unknown hidden, private directory excluded',()=>{
  const mk=(id,status)=>({id,status,directory:'/private',updatedAt:'2026-11-08T12:00:00Z'});
  const result=orderAgents([mk('d','DONE'),mk('u','UNKNOWN'),mk('w','WORKING'),mk('q','WAITING')]);
  assert.deepEqual(result.map(x=>x.id),['q','w','d']);
  assert.equal('directory' in result[0],false);
});
test('Codex explicit transitions, not silence',()=>{assert.equal(codexTransition({type:'event_msg',payload:{type:'task_started'}}),'WORKING');assert.equal(codexTransition({type:'event_msg',payload:{type:'task_complete'}}),'DONE');assert.equal(codexTransition({type:'event_msg',payload:{type:'turn_aborted'}}),'UNKNOWN');assert.equal(codexTransition({type:'response_item',payload:{type:'function_call'}}),null);assert.equal(visibleStatus('WORKING',0,301000,300000),'UNKNOWN');});
test('independent parallel sessions',()=>{const mk=(id,project,type)=>[{type:'session_meta',payload:{id,cwd:'/work/'+project}},{type:'event_msg',timestamp:'2026-11-08T12:00:00Z',payload:{type}}].map(JSON.stringify).join('\n'); const t=Date.parse('2026-11-08T12:00:01Z');assert.equal(foldCodex(mk('a','one','task_started'),t).status,'WORKING');assert.equal(foldCodex(mk('b','two','task_complete'),t).status,'DONE');});
test('OpenCode confirmed permission takes precedence',()=>{assert.equal(openStatus({type:'running'},true),'WAITING');assert.equal(openStatus({type:'running'},false),'WORKING');assert.equal(openStatus({type:'idle'},false),'IDLE');assert.equal(openStatus(undefined,false),'UNKNOWN');});
