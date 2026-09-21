const $=id=>document.getElementById(id), api=async(path,options={})=>{const r=await fetch(path,options);const d=await r.json();if(!r.ok)throw Error(d.error||r.statusText);return d};
const post=(path,body,headers={})=>api(path,{method:'POST',headers:{'Content-Type':'application/json',...headers},body:JSON.stringify(body)});
const KEYS='ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789 ENTER SPACE TAB ESC UP DOWN LEFT RIGHT'.trim().split(/\s+/).concat(Array.from({length:12},(_,i)=>`F${i+1}`));
const GPIOS=[['D0',1],['D1',2],['D2',3],['D3',4],['D4',5],['D5',6],['D6',43],['D7',44],['D8',7],['D9',8],['D10',9]];
const MAX_BUTTONS=8;
let recordingIndex=-1;

function normalizeKey(event){
  if(['Control','Shift','Alt','Meta'].includes(event.key))return null;
  const special={'Enter':'ENTER',' ':'SPACE','Tab':'TAB','Escape':'ESC','ArrowUp':'UP','ArrowDown':'DOWN','ArrowLeft':'LEFT','ArrowRight':'RIGHT'};
  if(special[event.key])return special[event.key];
  if(/^F([1-9]|1[0-2])$/.test(event.key))return event.key.toUpperCase();
  if(event.key.length===1&&/^[a-zA-Z0-9]$/.test(event.key))return event.key.toUpperCase();
  return null;
}
document.addEventListener('keydown',(event)=>{
  if(recordingIndex<0)return;
  event.preventDefault();event.stopPropagation();
  const key=normalizeKey(event);if(!key)return;
  const card=$('buttonList').children[recordingIndex];
  card.querySelector('.key').value=key;
  card.querySelector('.ctrl').checked=event.ctrlKey;card.querySelector('.shift').checked=event.shiftKey;
  card.querySelector('.alt').checked=event.altKey;card.querySelector('.gui').checked=event.metaKey;
  card.querySelector('.record').textContent='Record key press';
  recordingIndex=-1;
},true);

function buttonCardHtml(index){
  return `<div class="button-card"><h3>Button ${index+1}</h3>
    <label>GPIO<select class="gpio">${GPIOS.map(([n,g])=>`<option value="${g}">${n} (GPIO${g})</option>`).join('')}</select></label>
    <label>Action<select class="type"><option value="0">Disabled</option><option value="1">USB HID shortcut</option><option value="2">USB serial event</option></select></label>
    <div class="hid"><label>Key<select class="key">${KEYS.map(k=>`<option value="${k}">${k}</option>`).join('')}</select></label>
      <fieldset><legend>Modifiers</legend><label><input type="checkbox" class="ctrl"> Ctrl</label><label><input type="checkbox" class="shift"> Shift</label><label><input type="checkbox" class="alt"> Alt</label><label><input type="checkbox" class="gui"> GUI / Super</label></fieldset>
      <button type="button" class="record secondary">Record key press</button></div>
    <div class="serial"><label>Action ID<input class="event" maxlength="64" pattern="[A-Za-z0-9_-]+"></label></div>
    <div class="row"><button type="button" class="test secondary">Test</button><button type="button" class="remove danger">Remove</button></div>
    <p class="cardResult note"></p></div>`;
}
function renumberCards(){[...$('buttonList').children].forEach((card,i)=>card.querySelector('h3').textContent=`Button ${i+1}`)}
function wireCard(card){
  const typeSelect=card.querySelector('.type');
  const toggle=()=>{card.querySelector('.hid').hidden=typeSelect.value!='1';card.querySelector('.serial').hidden=typeSelect.value!='2'};
  typeSelect.onchange=toggle;toggle();
  card.querySelector('.record').onclick=()=>{recordingIndex=[...$('buttonList').children].indexOf(card);card.querySelector('.record').textContent='Press a key…'};
  card.querySelector('.test').onclick=async()=>{
    const index=[...$('buttonList').children].indexOf(card),result=card.querySelector('.cardResult');
    try{await post('/api/action/test',{index});result.textContent='Action sent.'}catch(e){result.textContent=e.message}
  };
  card.querySelector('.remove').onclick=()=>{if($('buttonList').children.length<=1)return;card.remove();renumberCards()};
}
function addButtonCard(btn){
  if($('buttonList').children.length>=MAX_BUTTONS)return;
  const wrapper=document.createElement('div');wrapper.innerHTML=buttonCardHtml($('buttonList').children.length);
  const card=wrapper.firstElementChild;$('buttonList').appendChild(card);
  if(btn){
    card.querySelector('.gpio').value=btn.gpio;card.querySelector('.type').value=btn.actionType;
    card.querySelector('.key').value=btn.hidKey;card.querySelector('.event').value=btn.serialAction;
    card.querySelector('.ctrl').checked=btn.modifiers&1;card.querySelector('.shift').checked=btn.modifiers&2;
    card.querySelector('.alt').checked=btn.modifiers&4;card.querySelector('.gui').checked=btn.modifiers&8;
  }
  wireCard(card);
}
$('addButton').onclick=()=>addButtonCard(null);
$('save').onclick=async()=>{
  try{
    const buttons=[...$('buttonList').children].map(card=>({
      gpio:+card.querySelector('.gpio').value,actionType:+card.querySelector('.type').value,
      hidKey:card.querySelector('.key').value,serialAction:card.querySelector('.event').value,
      modifiers:(card.querySelector('.ctrl').checked?1:0)|(card.querySelector('.shift').checked?2:0)|(card.querySelector('.alt').checked?4:0)|(card.querySelector('.gui').checked?8:0)
    }));
    await post('/api/config',{deviceName:$('name').value,buttons});
    $('actionResult').textContent='Saved. Reboot to apply new GPIO pins.';
  }catch(e){$('actionResult').textContent=e.message}
};

async function status(){try{const s=await api('/api/status');$('subtitle').textContent=`${s.deviceName} · ${s.ip}`;$('status').innerHTML=Object.entries({Firmware:s.firmware,Uptime:`${s.uptimeSeconds}s`,WiFi:s.ssid,IP:s.ip,RSSI:`${s.rssi} dBm`,USB:s.usbConnected?'connected':'not open',Buttons:s.buttonCount,'Last event':s.lastEvent}).map(([k,v])=>`<div class="metric"><b>${k}</b>${String(v).replace(/[<>&]/g,'')}</div>`).join('');$('fwCurrent').textContent=s.firmware}catch(e){}};
async function load(){const c=await api('/api/config');$('name').value=c.deviceName;$('buttonList').innerHTML='';c.buttons.forEach(b=>addButtonCard(b))}
$('scan').onclick=async()=>{
  const btn=$('scan');btn.disabled=true;btn.textContent='Scanning…';$('ssid').innerHTML='<option value="">Scanning…</option>';
  try{
    for(;;){
      const d=await api('/api/wifi/scan');
      if(Array.isArray(d)){
        $('ssid').innerHTML='<option value="">Select or scan…</option>';
        d.sort((a,b)=>b.rssi-a.rssi).forEach(n=>$('ssid').add(new Option(`${n.ssid} (${n.rssi} dBm)${n.secure?'':' \u{1F513}'}`,n.ssid)));
        break;
      }
      await new Promise(r=>setTimeout(r,700));
    }
  }catch(e){$('wifiResult').textContent=e.message}
  finally{btn.disabled=false;btn.textContent='Scan networks';}
};
$('saveWifi').onclick=async()=>{try{const ssid=$('ssidManual').value.trim()||$('ssid').value;await post('/api/wifi',{ssid,password:$('password').value});$('wifiResult').textContent='Saved. Rebooting…';setTimeout(()=>$('reboot').click(),500)}catch(e){$('wifiResult').textContent=e.message}};
$('forgetWifi').onclick=async()=>{if(!confirm('Wi-Fi credentials will be erased and the device restarts its setup hotspot.'))return;try{await post('/api/wifi',{ssid:'',password:''});$('wifiResult').textContent='Forgotten. Rebooting…';setTimeout(()=>$('reboot').click(),500)}catch(e){$('wifiResult').textContent=e.message}};
$('reboot').onclick=()=>post('/api/system/reboot',{});$('export').onclick=()=>location='/api/config/export';
$('import').onclick=async()=>{try{const f=$('importFile').files[0];if(!f)throw Error('Choose a JSON file.');if(f.size>4096)throw Error('Configuration is too large.');const value=JSON.parse(await f.text());await post('/api/config',value);$('systemResult').textContent='Configuration imported.';await load()}catch(e){$('systemResult').textContent=e.message}};
$('reset').onclick=async()=>{if(prompt('Type RESET to erase all settings')==='RESET')await post('/api/system/factory-reset',{}, {'X-Confirm-Reset':'RESET'})};
$('upload').onclick=()=>{const f=$('firmware').files[0];if(!f||!f.name.endsWith('.bin'))return $('otaResult').textContent='Choose a .bin file.';const x=new XMLHttpRequest(),form=new FormData();form.append('firmware',f);x.open('POST','/api/ota');x.upload.onprogress=e=>{if(e.lengthComputable)$('progress').value=e.loaded/e.total*100};x.onload=()=>$('otaResult').textContent=x.status===200?'Updated; device is rebooting.':'Update failed.';x.send(form)};

$('checkUpdate').onclick=async()=>{
  const btn=$('checkUpdate');btn.disabled=true;$('updateStatus').textContent='Checking GitHub…';$('installUpdate').hidden=true;
  try{
    const d=await post('/api/update/check',{});
    $('updateStatus').textContent=d.available?`Update available: ${d.current} \u2192 ${d.latest} (${Math.round(d.size/1024)} KB)`:`Firmware is up to date (${d.current}).`;
    $('installUpdate').hidden=!d.available;
  }catch(e){$('updateStatus').textContent=e.message}
  btn.disabled=false;
};
$('installUpdate').onclick=async()=>{
  if(!confirm('Install firmware update now? The device will restart automatically.'))return;
  $('installUpdate').disabled=true;$('checkUpdate').disabled=true;$('updateProgress').hidden=false;$('updateProgress').value=0;
  try{await post('/api/update/install',{});monitorUpdate()}
  catch(e){$('updateStatus').textContent=e.message;$('installUpdate').disabled=false;$('checkUpdate').disabled=false}
};
async function monitorUpdate(){
  try{
    const d=await api('/api/update/status');
    $('updateProgress').value=d.progress||0;
    if(d.message)$('updateStatus').textContent=d.message;
    if(d.state==='error'){$('installUpdate').disabled=false;$('checkUpdate').disabled=false;return}
  }catch(e){/* device may be restarting */}
  setTimeout(monitorUpdate,2000);
}

load();status();setInterval(status,5000);
