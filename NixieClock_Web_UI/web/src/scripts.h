#pragma once

// GP Scripts

const char GP_JS_TOP[] PROGMEM = R"(
var _tout=2000,_zoom=1,_err=0;
var _popupStack=[],_clkRelList=[],_clkCloseList=[],_clkUpdList={},_pressId=null,_spinInt=null,_spinF=0,_touch=0;
document.title='GyverPortalMod';
function EVsend(req,r=null,upd=null){var xhttp=new XMLHttpRequest();xhttp.open(upd?'GET':'POST',req,true);
xhttp.send();xhttp.timeout=_tout;xhttp.onreadystatechange=function(){if(this.status||(++_err>=5)){onlShow(!this.status);_err=0;}if(this.status||upd){
if(this.readyState==4&&this.status==200){if(r){if(r==1)location.reload();else location.href=r;}if(upd)EVapply(upd,this.responseText);}}}}
function EVupdate(ids){ids=ids.replaceAll(' ','');EVsend('/EV_update?'+ids+'=',null,ids);}
function EVapply(ids,resps){
resps=resps.split('\1');ids=ids.split(',');if(ids.length!=resps.length)return;
for(let i=0;i<ids.length;i++){
let item=getEl(ids[i]),resp=resps[i];
if(!item){item=getEl(ids[i]+'_'+resp);if(item&&item.type=='radio'){item.checked=1;continue}}
if(!item||!resp)continue;
switch(item.type){
case'hidden':{var val=item.value?item.value:resp;
switch(item.name){
case'_eval':eval(val);break;
case'_reload':if(resp=='1')location.reload();break;
case'_alert':alert(val);if(_clkRelList.includes(item.id))location.reload();break;
case'_prompt':{let res=prompt(item.value,resp);if(res)EVsend('/EV_click?'+item.id+'='+res,_clkRelList.includes(item.id));}break;
case'_confirm':{let res=confirm(val);EVsend('/EV_click?'+item.id+'='+(res?'1':'0'),res?_clkRelList.includes(item.id):0);}break;
case'_title':document.title=resp;break;
case'_line':lineChange(item,resp);break;
case'_range':if(_rangeFocus!=2)rangeChange(item,resp);break;}}break;
case'checkbox':case'radio':item.checked=Number(resp);break;
case'select':item.max=Number(resp);selectUpdate(item);break;
case'select-one':document.querySelector('#'+item.id).value=resp;break;
case undefined:switch(item.className){
case'_popup':if(resp=='1')popupOpen(item.innerHTML,item.id);else if(resp=='-1')popupClose();break;
case'_link':linkUpdate(item.id,resp);break;
case'ledc':ledColor(item.id,resp);break;
default:item.innerHTML=resp;break;}break;
default:item.value=resp;break;}
switch(item.type){
case'number':EVspinw(item);break;
case'textarea':logScroll(item);break;
}}}
function EVpress(arg,dir){_pressId=(dir==1)?arg.name:null;if(arg.name)EVsend('/EV_press?'+arg.name+'='+dir);}
function EVclick(arg,r=null,s=null){var val;var name=(!arg.name)?arg.id:arg.name;
if(arg.type=='number'){
if(arg.hasAttribute('min')&&Number(arg.value)<=Number(arg.min))arg.value=arg.min;
if(arg.hasAttribute('max')&&Number(arg.value)>=Number(arg.max))arg.value=arg.max;}
if(name){if(arg.type=='checkbox')val=arg.checked?'1':'0';
else if(arg.type=='button'||arg.value==undefined)val=(s!=null)?s:'';else val=arg.value;
if(_clkRelList.includes(name))r=1;
if(_clkCloseList.includes(name))popupClose();
EVsend('/EV_click?'+name+'='+encodeURIComponent(val),r);
if(_clkUpdList){for(var key in _clkUpdList){if(key.includes(name))EVupdate(_clkUpdList[key]);}}}}
function EVclickVal(arg){EVsend('/EV_click?'+arg.id+'='+arg.value);}
function EVclickId(btn,tar,r){EVsend('/EV_click?'+btn+'='+encodeURIComponent(getEl(tar).value),r);}
function EVsubmId(id){getEl(id).submit();event.preventDefault();}
function EVsendForm(id,url){
var elms=getEl(id).elements;var qs='';
for(var i=0,elm;elm=elms[i++];){if(elm.name){
var v=elm.value;
if(elm.type=='checkbox')v=elm.checked?1:0;
qs+=elm.name+'='+encodeURIComponent(v)+'&';}}
EVsend(id+'?'+qs.slice(0,-1),url);}
function EVsaveFile(id){getEl(id).click();}
function EVdelete(url){if(!confirm('Delete '+url+'?'))return;EVsend('/EV_delete?'+url+'=',1);}
function EVrename(url){res=prompt('Rename File',url);if(!res)return;EVsend('/EV_rename?'+url+'='+res,1);}
function EVopenTab(tab,btn,blk){var x=document.getElementsByClassName(blk);
for(var i=0;i<x.length;i++)x[i].style.display='none';
getEl(tab).style.display='block';
x=document.getElementsByClassName(btn.className);
for(var i=0;i<x.length;i++)x[i].classList.remove('navopen');
btn.classList.add('navopen');}
function EVhint(id,txt){el=getEl(id);if(el.className=='_sw_c'){el=getEl('_'+id)}el.title=txt;}
function EVhintBox(min,max,box){_min=getEl(min);_max=getEl(max);_box=getEl(box);
_box.style.visibility=(_min.value==_max.value)?'visible':'hidden';}
function EVhintLoad(min,max,func){func();getEl(min).addEventListener("change", func);getEl(max).addEventListener("change", func);}
function EVspinc(arg){if(arg.className=='spin_inp'){arg.value-=arg.value%arg.step;}}
function EVspinw(arg){if(arg.className=='spin_inp')arg.style.width=((arg.value.length+2)*12)+'px';}
function EVspin(arg){var num=getEl(arg.name);num.value=(Number(num.value)+Number(arg.min)).toFixed(Number(arg.max));
var e=new Event('change');num.dispatchEvent(e);}
function EVeye(arg){var p=arg.previousElementSibling;
p.type=p.type=='text'?'password':'text';
arg.style.color=p.type=='text'?'#bbb':'#13161a';}
function addEv(ev,fn){document.body.addEventListener(ev,fn,{passive:false});}
function delEv(ev,fn){document.body.removeEventListener(ev,fn,{passive:false});}
function setZoom(w,z){if(window.innerWidth<=w){_zoom=z/100;document.querySelector(':root').style.zoom=_zoom;}}
function setOvf(f){document.body.style.overflow=f?null:'hidden';}
function getEl(id){return document.getElementById(id);}
function getPop(id){return getEl(id).innerHTML;}
function swUpd(id,val){id=id.split(',');for(let i=0;i<id.length;i++){getEl(id[i]).checked=val;}}
function sdbTgl(){let flag=getEl('dashOver').style.display=='block';getEl('dashOver').style.display=flag?'none':'block';getEl('dashSdb').style.left=flag?'-260px':'0';setOvf(flag);}
function onlShow(s){getEl('offlAnim').style.display=s?'block':'none';}
function numNext(pr,nx,ch){if(ch)pr.value=0+pr.value;if(pr.value.length>=2){EVclick(pr);pr.placeholder=pr.value;pr.value='';pr.blur();if(nx)getEl(nx).focus();}}
function numConst(arg,min,max){let data=arg.value.replaceAll('-','');if(data.length){if(data<min)data=min;else if(data>max)data=max;}arg.value=data;}
function lineChange(arg,val=null){if(val!=null)arg.value=limit(val,arg.min,arg.max);const dsp=getEl(arg.id+'_dsp');dsp.style.backgroundSize=(arg.value-arg.min)*100/(arg.max-arg.min)+'% 100%';}
function ledColor(id,c){let el=getEl('led_'+id);if(el){if(c){el.style.boxShadow='0px 0px 10px 2px '+c;el.style.backgroundColor=c;}else el.removeAttribute('style');}}
function textEn(arg){arg.value=arg.value.replace(/[^\w\s\-\_\(\)\.\"\']/g,'');}
function textBlink(id){let el=getEl(id);let val=el.value;if(val.charAt(val.length-1)=='|')EVupdate(id);else el.value+='|';}
function logScroll(id){id.scrollTop=id.scrollHeight;}
function popupOpen(val,id=null){let el=getEl('_popup');if((el.innerHTML=='')||(id==null)){setOvf(0);el.innerHTML=val;el.style.display='flex';setTimeout(function(){el.style.backdropFilter='blur(5px)';},15);}
if(id!=null){if(!_popupStack.includes(id)&&(_popupStack.length<=10))_popupStack.push(id);}else if(!_popupStack.includes('&pop&'))_popupStack.unshift('&pop&');}
function popupClose(){let el=getEl('_popup');if(el.style.display!='none'){el.innerHTML='';el.removeAttribute('onclick');el.style.backdropFilter='blur(0)';setTimeout(function(){if(el.innerHTML==''){el.style.display='none';setOvf(1);}},300);}
_popupStack.shift();if(_popupStack.length>0){let pop=getEl(_popupStack[0]);popupOpen(pop.innerHTML,pop.id);}}
function linkUpdate(id,val){val=val.split(',');var block='';var data='';for(let i=0;i<val.length;i++){data=val[i];data=data.split(':');if((data.length==2)&&(data[0].length)){block+='<a href=\"http://';
block+=data[0];block+='\"';if(data[0]==window.location.hostname)block+=' class=\"sbsel\" style=\"background:#e67b09!important;\"';block+='>';block+=data[1].length?data[1]:data[0];block+='</a>';}}
let el=getEl(id);el.querySelector('#_link_block').innerHTML=block;el.style.display=block.length?'block':'none';}
function selectUpdate(arg){let val=arg.placeholder.split(',');let num=(Number(val.length)>Number(arg.max))?arg.max:0;arg.value=((arg.min!=0)?((Number(arg.min)+num)+'. '+val[num]):val[num]).replaceAll('&dsbl&','');}
function selectClick(arg){if(arg.id=='_popup')popupClose();else if(arg.id.includes('_sel_')){popupClose();let el=getEl(arg.name);if(Number(arg.max)!=Number(el.max)){el.max=Number(arg.max);el.value=arg.value;EVclick(arg,Number(el.step),Number(arg.max));}}}
function selectList(arg){arg.blur();const list=document.createElement('div');list.className='blockBase block thinBlock selBlock';let val=arg.placeholder.split(',');
for(let i=0;i<val.length;i++){const item=document.createElement('input');item.className='selItem';item.id='_sel_'+i;item.name=arg.id;item.type='button';item.max=i;
item.value=(arg.min!=0)?((Number(arg.min)+i)+'. '+val[i]):val[i];item.setAttribute('onclick','selectClick(this)');if(i==arg.max)item.className+=' selActive';list.appendChild(item);
if(item.value.includes('&dsbl&')){item.value=item.value.replaceAll('&dsbl&','');item.disabled=true;}}
const pop=document.createElement('div');pop.className='popupBlock';pop.appendChild(list);popupOpen(pop.outerHTML);getEl('_popup').setAttribute('onclick','selectClick(event.target)');}
function pageUpdate(){document.querySelectorAll('.range_inp').forEach(x=>{rangeActiv(x);rangeChange(x)});document.querySelectorAll('.spin_inp').forEach(x=>EVspinw(x));document.querySelectorAll('input[type=select]').forEach(x=>selectUpdate(x));
let el=document.querySelector('.ui_block');if(el!=null){document.querySelector('.ui_load').remove();el.style.display='block';setTimeout(function(){el.style.opacity=1;},15);}}

var _rangeId=null,_rangeFocus=0,_rangeWidth=0,_rangePos=0,_rangeX=0,_rangeY=0;
function limit(n,min,max){n=Number(n);min=Number(min);max=Number(max);return(n>max)?max:(n<min)?min:n;}
function steplim(n,step){n=Number(n);step=Number(step);if(step>=0.01){let val=n;while(n>=step)n-=step;return(val-n).toFixed(2);}else return n.toFixed(2);}
function rangeActiv(arg){let el=getEl(arg.id+'_dsp');if(!el.classList.contains('dsbl')){el.addEventListener('touchstart',rangeStart,{passive:true});el.addEventListener('mousedown',rangeStart,{passive:true});}}
function rangeChange(arg,val=null){lineChange(arg,val);const out=getEl(arg.id+'_val');if(arg.placeholder){const col=arg.placeholder.split(',');let pos=arg.value-Number(arg.min);out.style.backgroundColor=(col.length>pos)?col[pos]:'#2a2d35';}
else{const lim=out.name.split(',');if((arg.value<=Number(arg.min))&&lim[0]){out.value=lim[0];}else if((arg.value>=Number(arg.max))&&lim[1]){out.value=lim[1];}else out.value=Number(arg.value);}}
function rangeClick(pos,fs=null){let step=_rangeWidth/Number(_rangeId.max-_rangeId.min);let val=(limit(Math.round((pos-_rangePos)+(step/2)),0,_rangeWidth)/step)+Number(_rangeId.min);
val=steplim(val,_rangeId.step);if(Number(_rangeId.value)!=val){rangeChange(_rangeId,val);if(_rangeId.checked)fs=1;}if(fs)EVclickVal(_rangeId);}
function rangeStop(){delEv('touchend',rangeEvent);delEv('touchcancel',rangeEvent);delEv('touchmove',rangeEvent);delEv('mousemove',rangeEvent);delEv('mouseup',rangeEvent);_rangeId=null;_rangeFocus=0;}
function rangeStart(ev){_rangeFocus=0;if((ev.type=='touchstart')||((ev.type=='mousedown')&&(ev.which==1))){
_rangeId=getEl(this.id.replaceAll('_dsp',''));_rangeWidth=Math.round(this.offsetWidth)*_zoom;let rect=this.getBoundingClientRect();_rangePos=rect.left;
if(ev.type=='touchstart'){_rangeFocus=1;_rangeX=ev.touches[0].clientX;_rangeY=ev.touches[0].clientY;addEv('touchend',rangeEvent);addEv('touchcancel',rangeEvent);addEv('touchmove',rangeEvent);}
else{_rangeFocus=2;_rangeX=ev.clientX;addEv('mouseup',rangeEvent);addEv('mousemove',rangeEvent);}}}
function rangeEvent(ev){if(ev.cancelable==true)ev.preventDefault();switch(ev.type){
case'mousemove':if(ev.which==1){rangeClick(ev.clientX);_rangeX=ev.clientX;}else{rangeClick(_rangeX);rangeStop();}break;
case'touchmove':if(_rangeFocus==2){rangeClick(ev.touches[0].clientX);_rangeX=ev.touches[0].clientX;}
else if(_rangeFocus==1){if(Math.abs(_rangeY-ev.touches[0].clientY)>=5)rangeStop();
else if(Math.abs(_rangeX-ev.touches[0].clientX)>=5){_rangeFocus=2;_rangeX=ev.touches[0].clientX;}}break;
default:rangeClick(_rangeX,1);rangeStop();break;}}
)";
