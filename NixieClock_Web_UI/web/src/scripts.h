#pragma once

// GP Scripts

const char GP_JS_TOP[] PROGMEM = R"(
var _tout=2000,_err=0;
var _clkRelList=[],_touch=0,_clkRedrList={},_clkUpdList={},_pressId=null,_spinInt=null,_spinF=0;
document.title='GyverPortalMod';
function GP_send(req,r=null,upd=null){var xhttp=new XMLHttpRequest();xhttp.open(upd?'GET':'POST',req,true);
xhttp.send();xhttp.timeout=_tout;xhttp.onreadystatechange=function(){if(this.status||(++_err>=5)){onlShow(!this.status);_err=0;}if(this.status||upd){
if(this.readyState==4&&this.status==200){if(r){if(r==1)location.reload();else location.href=r;}if(upd)GP_apply(upd,this.responseText);}}}}
function GP_update(ids){ids=ids.replaceAll(' ','');GP_send('/GP_update?'+ids+'=',null,ids);}
function GP_delete(url){if(!confirm('Delete '+url+'?'))return;GP_send('/GP_delete?'+url+'=',1);}
function GP_rename(url){res=prompt('Rename File',url);if(!res)return;GP_send('/GP_rename?'+url+'='+res,1);}
function GP_hint(id,txt){el=getEl(id);if(el.className=='_sw_c'){el=getEl('_'+id)}el.title=txt;}
function GP_press(arg,dir){_pressId=(dir==1)?arg.name:null;if(arg.name)GP_send('/GP_press?'+arg.name+'='+dir);}
function GP_click(arg,r=null){if(!arg.name)arg.name=arg.id;var v;
if(arg.type=='number'){
if(arg.hasAttribute('min')&&Number(arg.value)<=Number(arg.min))arg.value=arg.min;
if(arg.hasAttribute('max')&&Number(arg.value)>=Number(arg.max))arg.value=arg.max;}
if(arg.type=='checkbox')v=arg.checked?'1':'0';
else if(arg.type=='button'||arg.value==undefined)v='';
else v=arg.value;
if(_clkRelList.includes(arg.name))r=1;
GP_send('/GP_click?'+arg.name+'='+encodeURIComponent(v),r);
if(_clkUpdList){for(var key in _clkUpdList){if(key.includes(arg.name))GP_update(_clkUpdList[key]);}}
if(_clkRedrList){for(var key in _clkRedrList){if(_clkRedrList[key].includes(arg.name))GP_update(key);}};}
function GP_clickid(btn,tar){GP_send('/GP_click?'+btn+'='+encodeURIComponent(getEl(tar).value));}
function GP_change(arg){arg.style.backgroundSize=(arg.value-arg.min)*100/(arg.max-arg.min)+'% 100%';
const _output=getEl(arg.id+'_val');const _range=_output.name.split(',');if((arg.value<=Number(arg.min))&&_range[0]){_output.value=_range[0];}
else if((arg.value>=Number(arg.max))&&_range[1]){_output.value=_range[1];}else _output.value=arg.value;}
function GP_wheel(arg){var e=window.event;arg.value-=Math.sign(e.deltaY||e.detail||e.wheelDelta)*Number(arg.step);}
function saveFile(id){getEl(id).click();}
function GP_submId(id){getEl(id).submit();event.preventDefault();}
function openTab(tab,btn,blk){var x=document.getElementsByClassName(blk);
for(var i=0;i<x.length;i++)x[i].style.display='none';
getEl(tab).style.display='block';
x=document.getElementsByClassName(btn.className);
for(var i=0;i<x.length;i++)x[i].style.background='';
btn.style.background='#2a2d35';}
function GP_spinc(arg){if(arg.className=='spin_inp'){arg.value-=arg.value%arg.step;}}
function GP_spinw(arg){if(arg.className=='spin_inp')arg.style.width=((arg.value.length+2)*12)+'px';}
function GP_spin(arg){var num=getEl(arg.name);num.value=(Number(num.value)+Number(arg.min)).toFixed(Number(arg.max));
var e=new Event('change');num.dispatchEvent(e);}
function GP_apply(ids,resps){
resps=resps.split('\1');ids=ids.split(',');if(ids.length!=resps.length)return;
for(let i=0;i<ids.length;i++){
let item=getEl(ids[i]),resp=resps[i];
if(!item){item=getEl(ids[i]+'_'+resp);if(item&&item.type=='radio'){item.checked=1;continue}}
if(!item||!resp)continue;
switch(item.type){
case'hidden':{var val=item.value?item.value:resp;
switch(item.name){
case'_reload':if(resp=='1')location.reload();break;
case'_alert':alert(val);if(_clkRelList.includes(item.id))location.reload();break;
case'_prompt':{let res=prompt(item.value,resp);if(res)GP_send('/GP_click?'+item.id+'='+res,_clkRelList.includes(item.id));}break;
case'_confirm':{let res=confirm(val);GP_send('/GP_click?'+item.id+'='+(res?'1':'0'),res?_clkRelList.includes(item.id):0);}break;
case'_eval':eval(val);break;
case'_title':document.title=resp;break;}}break;
case'checkbox': case'radio':item.checked=Number(resp);break;
case'select-one':document.querySelector('#'+item.id).value=resp;break;
case undefined:item.innerHTML=resp;break;
default:item.value=resp;break;}
switch(item.type){
case'range':GP_change(item);break;
case'number':GP_spinw(item);break;}}}
function GP_sendForm(id,url){
var elms=getEl(id).elements;var qs='';
for(var i=0,elm;elm=elms[i++];){if(elm.name){
var v=elm.value;
if(elm.type=='checkbox')v=elm.checked?1:0;
qs+=elm.name+'='+encodeURIComponent(v)+'&';}}
GP_send(id+'?'+qs.slice(0,-1),url);}
function GP_eye(arg){var p=arg.previousElementSibling;
p.type=p.type=='text'?'password':'text';
arg.style.color=p.type=='text'?'#bbb':'#13161a';}
function getEl(id){return document.getElementById(id);}
function sdbTgl(){let flag=getEl('dashOver').style.display=='block';getEl('dashOver').style.display=flag?'none':'block';
getEl('dashSdb').style.left=flag?'-250px':'0';}
function onlShow(s){getEl('onlBlock').style.right=s?'0px':'-50px';}
function numNext(pr,nx,ch){if(ch)pr.value=0+pr.value;if(pr.value.length>=2){GP_click(pr);pr.placeholder=pr.value;pr.value='';pr.blur();if(nx)getEl(nx).focus();}}
function numConst(arg,min,max){let data=arg.value.replaceAll('-','');if(data<min)data=min;else if(data>max)data=max;arg.value=data;}
function ledColor(id,cl){let el=getEl('led_'+id);if(el){if(cl){el.style.boxShadow='0px 0px 10px 2px '+cl;el.style.backgroundColor=cl;}else el.removeAttribute('style');}}
)";
