#pragma once

// GP Scripts

const char GP_JS_TOP[] PROGMEM = R"(
var _tout=2000,_err=0;
var _clkRelList=[],_clkCloseList=[],_clkUpdList={},_pressId=null,_spinInt=null,_spinF=0,_touch=0;
document.title='GyverPortalMod';
function EVhintBox(min,max,box){_min=getEl(min);_max=getEl(max);_box=getEl(box);
_box.style.visibility=(_min.value==_max.value)?'visible':'hidden';}
function EVhintLoad(min,max,func){func();getEl(min).addEventListener("change", func);getEl(max).addEventListener("change", func);}
function EVsend(req,r=null,upd=null){var xhttp=new XMLHttpRequest();xhttp.open(upd?'GET':'POST',req,true);
xhttp.send();xhttp.timeout=_tout;xhttp.onreadystatechange=function(){if(this.status||(++_err>=5)){onlShow(!this.status);_err=0;}if(this.status||upd){
if(this.readyState==4&&this.status==200){if(r){if(r==1)location.reload();else location.href=r;}if(upd)EVapply(upd,this.responseText);}}}}
function EVupdate(ids){ids=ids.replaceAll(' ','');EVsend('/GP_update?'+ids+'=',null,ids);}
function EVdelete(url){if(!confirm('Delete '+url+'?'))return;EVsend('/GP_delete?'+url+'=',1);}
function EVrename(url){res=prompt('Rename File',url);if(!res)return;EVsend('/GP_rename?'+url+'='+res,1);}
function EVhint(id,txt){el=getEl(id);if(el.className=='_sw_c'){el=getEl('_'+id)}el.title=txt;}
function EVpress(arg,dir){_pressId=(dir==1)?arg.name:null;if(arg.name)EVsend('/GP_press?'+arg.name+'='+dir);}
function EVclick(arg,r=null,s=null){if(!arg.name)arg.name=arg.id;var v;
if(arg.type=='number'){
if(arg.hasAttribute('min')&&Number(arg.value)<=Number(arg.min))arg.value=arg.min;
if(arg.hasAttribute('max')&&Number(arg.value)>=Number(arg.max))arg.value=arg.max;}
if(arg.type=='checkbox')v=arg.checked?'1':'0';
else if(arg.type=='button'||arg.value==undefined)v=(s!=null)?s:'';
else v=arg.value;
if(_clkRelList.includes(arg.name))r=1;
if(_clkCloseList.includes(arg.name))popupClose();
EVsend('/GP_click?'+arg.name+'='+encodeURIComponent(v),r);
if(_clkUpdList){for(var key in _clkUpdList){if(key.includes(arg.name))EVupdate(_clkUpdList[key]);}}}
function EVclickId(btn,tar,r){EVsend('/GP_click?'+btn+'='+encodeURIComponent(getEl(tar).value),r);}
function EVchange(arg){arg.style.backgroundSize=(arg.value-arg.min)*100/(arg.max-arg.min)+'% 100%';
const _output=getEl(arg.id+'_val');const _range=_output.name.split(',');if((arg.value<=Number(arg.min))&&_range[0]){_output.value=_range[0];}
else if((arg.value>=Number(arg.max))&&_range[1]){_output.value=_range[1];}else _output.value=arg.value;}
function EVwheel(arg){var e=window.event;arg.value-=Math.sign(e.deltaY||e.detail||e.wheelDelta)*Number(arg.step);}
function EVsaveFile(id){getEl(id).click();}
function EVsubmId(id){getEl(id).submit();event.preventDefault();}
function EVopenTab(tab,btn,blk){var x=document.getElementsByClassName(blk);
for(var i=0;i<x.length;i++)x[i].style.display='none';
getEl(tab).style.display='block';
x=document.getElementsByClassName(btn.className);
for(var i=0;i<x.length;i++)x[i].classList.remove('navopen');
btn.classList.add('navopen');}
function EVspinc(arg){if(arg.className=='spin_inp'){arg.value-=arg.value%arg.step;}}
function EVspinw(arg){if(arg.className=='spin_inp')arg.style.width=((arg.value.length+2)*12)+'px';}
function EVspin(arg){var num=getEl(arg.name);num.value=(Number(num.value)+Number(arg.min)).toFixed(Number(arg.max));
var e=new Event('change');num.dispatchEvent(e);}
function EVapply(ids,resps){
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
case'_prompt':{let res=prompt(item.value,resp);if(res)EVsend('/GP_click?'+item.id+'='+res,_clkRelList.includes(item.id));}break;
case'_confirm':{let res=confirm(val);EVsend('/GP_click?'+item.id+'='+(res?'1':'0'),res?_clkRelList.includes(item.id):0);}break;
case'_eval':eval(val);break;
case'_title':document.title=resp;break;}}break;
case'checkbox': case'radio':item.checked=Number(resp);break;
case'select-one':document.querySelector('#'+item.id).value=resp;break;
case'button':if(item.name=='_select'){item.max=Number(resp);selectUpdate(item);}else item.value=resp;break;
case undefined:switch(item.title){
case'_popup':if(resp=='1')popupOpen(item.innerHTML);else if(resp=='-1')popupClose();break;
case'_link':linkUpdate(item.id,resp);break;
default:item.innerHTML=resp;break;}break;
default:item.value=resp;break;}
switch(item.type){
case'range':EVchange(item);break;
case'number':EVspinw(item);break;
case'textarea':logScroll(item);break;
}}}
function EVsendForm(id,url){
var elms=getEl(id).elements;var qs='';
for(var i=0,elm;elm=elms[i++];){if(elm.name){
var v=elm.value;
if(elm.type=='checkbox')v=elm.checked?1:0;
qs+=elm.name+'='+encodeURIComponent(v)+'&';}}
EVsend(id+'?'+qs.slice(0,-1),url);}
function EVeye(arg){var p=arg.previousElementSibling;
p.type=p.type=='text'?'password':'text';
arg.style.color=p.type=='text'?'#bbb':'#13161a';}
function getEl(id){return document.getElementById(id);}
function sdbTgl(){let flag=getEl('dashOver').style.display=='block';getEl('dashOver').style.display=flag?'none':'block';getEl('dashSdb').style.left=flag?'-250px':'0';}
function onlShow(s){getEl('onlBlock').style.display=s?'block':'none';}
function numNext(pr,nx,ch){if(ch)pr.value=0+pr.value;if(pr.value.length>=2){EVclick(pr);pr.placeholder=pr.value;pr.value='';pr.blur();if(nx)getEl(nx).focus();}}
function numConst(arg,min,max){let data=arg.value.replaceAll('-','');if(data.length){if(data<min)data=min;else if(data>max)data=max;}arg.value=data;}
function ledColor(id,cl){let el=getEl('led_'+id);if(el){if(cl){el.style.boxShadow='0px 0px 10px 2px '+cl;el.style.backgroundColor=cl;}else el.removeAttribute('style');}}
function textBlink(id){let el=getEl(id);let val=el.value;if(val.charAt(val.length-1)=='|')EVupdate(id);else el.value+='|';}
function logScroll(id){id.scrollTop=id.scrollHeight;}
function popupClose(){let el=getEl('_popup');if(el.style.display!='none'){el.innerHTML='';el.removeAttribute('onclick');el.style.backdropFilter='blur(0)';setTimeout(function(){if(el.innerHTML==''){el.style.display='none';document.body.style.overflow=null;}},300);}}
function popupOpen(val){let el=getEl('_popup');if(el.innerHTML==''){document.body.style.overflow='hidden';el.innerHTML=val;el.style.display='flex';setTimeout(function(){el.style.backdropFilter='blur(5px)';},15);}}
function linkUpdate(id,val){val=val.split(',');var block='';var data='';for(let i=0;i<val.length;i++){data=val[i];data=data.split(':');if((data.length==2)&&(data[0].length)){block+='<a href=\"http://';
block+=data[0];block+='\"';if(data[0]==window.location.hostname)block+=' class=\"sbsel\" style=\"background:#e67b09!important;\"';block+='>';block+=data[1].length?data[1]:data[0];block+='</a>';}}
let el=getEl(id);el.querySelector('#_link_block').innerHTML=block;el.style.display=block.length?'block':'none';}
function selectUpdate(arg){let val=arg.placeholder.split(',');let num=(Number(val.length)>Number(arg.max))?arg.max:0;arg.value=((arg.min!=0)?((Number(arg.min)+num)+'. '+val[num]):val[num]).replaceAll('&dsbl&','');}
function selectClick(arg){if(arg.id=='_popup')popupClose();else if(arg.id.includes('_sel_')){popupClose();let el=getEl(arg.name);if(Number(arg.max)!=Number(el.max)){el.max=Number(arg.max);el.value=arg.value;EVclick(arg,Number(el.step),Number(arg.max));}}}
function selectList(arg){const list=document.createElement('div');list.className='blockBase block thinBlock selList';let val=arg.placeholder.split(',');
for(let i=0;i<val.length;i++){const item=document.createElement('input');item.className='selItem';item.id='_sel_'+i;item.name=arg.id;item.type='button';item.max=i;
item.value=(arg.min!=0)?((Number(arg.min)+i)+'. '+val[i]):val[i];item.setAttribute('onclick','selectClick(this)');if(i==arg.max)item.className+=' selActive';list.appendChild(item);
if(item.value.includes('&dsbl&')){item.value=item.value.replaceAll('&dsbl&','');item.disabled=true;}}
const pop=document.createElement('div');pop.className='popupBlock';pop.appendChild(list);popupOpen(pop.outerHTML);getEl('_popup').setAttribute('onclick','selectClick(event.target)');}
function pageUpdate(){document.querySelectorAll('input[type=range]').forEach(x=>{EVchange(x)});document.querySelector('.ui_load').remove();document.querySelectorAll('.spin_inp').forEach(x=>EVspinw(x));document.querySelectorAll('.sel_btn').forEach(x=>selectUpdate(x));
let el=document.querySelector('.ui_block');el.style.display='block';setTimeout(function(){el.style.opacity=1;},15);}
)";
