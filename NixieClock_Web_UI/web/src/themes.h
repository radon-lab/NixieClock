#pragma once

// GP Themes

const char GP_DEFAULT_THEME[] PROGMEM = R"(
:root{color-scheme:light dark;}
html{overscroll-behavior:none;scrollbar-width:none;}
html::-webkit-scrollbar{display:none;}
body{font-family:Verdana;background:#13161a;color:#bbb;margin:0;height:100vh;height:100dvh;width:100%;}
body{-webkit-tap-highlight-color:transparent;}
.mainblock{margin:auto;padding:9px 8px;max-width:100%;overflow-x:hidden;}
#blockBack input[type=password],#blockBack input[type=text],#blockBack input[type=date],#blockBack input[type=time],#blockBack input[type=number],#blockBack input[type=select],#blockBack textarea,#blockBack select,#blockBack .slider{background:#13161a}
#blockBack input:checked+.slider{background-color:#37a93c;}
#blockBack .range{background:#13161a;background-repeat:no-repeat;}
p{display:inline;}
a,a:visited{color:#4CAF50;}
a:hover{color:#3d9941;}
hr{width:95%;margin:5px 0;border:none;border-top:2px solid #394048;}
h2{margin:8px 0;}
span{font-size:16px;}
label{white-space:nowrap;font-size:20px;margin:0 5px;}
.grid{display:flex;justify-content:space-between;margin-top:20px;margin-bottom:20px;}
#grid .block{width:100%;margin:0 10px;}
.blockBase{padding:5px 10px;margin:20px 10px;}
.block{border-radius:20px;background-color:#2a2d35;box-shadow:#00000073 0 0 10px;}
.blockShadow{margin:5px 10px;border-radius:15px;box-shadow:#00000052 0px 5px 10px 3px;}
.blockSpace{display:block;padding:0;flex-grow:1;}
.blockOfst{padding-bottom:10px;padding-top:5px;}
.blockTab{padding-top:0;}
.blockHeader{padding:4px;margin:0 -10px 10px -10px;border-radius:10px 10px 0 0;color:#13161a;font-size:22px;text-align:center;background-color:#4caf50;}
.chartBlock{margin:3px;width:90%;border-radius:10px;}
.thinBlock{background:#13161a;border:2px solid #3c4048;}
.thinBold{padding:5px 5px 5px 15px;border-width:4px;border-radius:25px;}
.thinText{padding:0 7px;font-size:20px;white-space:nowrap;background:linear-gradient(0deg,#13161a 0,#13161a 54%,#13161a94 55%,#00000000 60%);}
.thinTab{display:block;padding:4px 0 0 25px;margin:-20px -10px 5px -10px;border:none;text-align:left;color:#9c9ea1;background:none;}
.inliner{display:flex;flex-direction:row;align-content:center;flex-wrap:nowrap;align-items:center;width:100%;}
input[type=number],input[type=text],input[type=password],input[type=date],input[type=time],input[type=color],input[type=checkbox],input[type=select],select,textarea{
width:90%;max-width:200px;border-radius:10px;padding:3px 10px;color:#bbb;border:none;background-color:#2a2d35;vertical-align:middle;
position:relative;margin:6px 4px;font-size:20px;height:40px;cursor:pointer;}
input[type=color]{width:80px;height:40px;}
input[type=checkbox]{width:27px;height:27px;margin-bottom:10px;}
input[type=time],input[type=date]{border:none!important;}
input[type=number],input[type=text],input[type=password]{padding-top:5px!important;}
input[type=number],input[type=text],input[type=password],input[type=time],input[type=date],input[type=select],select,textarea{text-align:center;appearance:none;}
input[type=number],input[type=text],input[type=password],input[type=select],select,textarea{display:inline-block;border-radius:10px;box-sizing:border-box;cursor:auto;}
input[type=select],select{min-width:130px;cursor:pointer;}
input[type=select]:disabled,select:disabled{filter:brightness(0.6);cursor:default;}
input[type=submit],input[type=button],button{height:55px;font-size:24px;width:90%;max-width:300px;margin:8px 5px;background-color:#4CAF50;border:none;border-radius:28px;line-height:90%;color:#13161a;cursor:pointer;}
input[type=submit]:hover,input[type=button]:hover,button:hover,.i_btn:hover{filter:brightness(0.93);}
input[type=submit]:active,input[type=button]:active,button:active{box-shadow:inset 0 1px 6px #0007;}
textarea{text-align:left;resize:none;overflow:hidden;max-width:100%;width:100%;box-sizing:unset;}
textarea::-webkit-scrollbar{width:6px;}
textarea::-webkit-scrollbar-track{background-color:#0000;}
textarea::-webkit-scrollbar-thumb{background-color:#556;border-radius:3px;}
input:focus,select,textarea:focus{outline:none;}
.led{margin:9px 10px;}
.led:after{display:inline-block;top:-3px;left:-3px;width:20px;height:20px;border-radius:10px;position:relative;content:'';background-color:#333;box-shadow:inset 0 0 5px 2px #0000005c;}
.led:checked:after{box-shadow:0 0 10px 4px;}
.led.red:checked:after{background-color:#d00;box-shadow:0 0 10px 2px #d00;}
.led.green:checked:after{background-color:#3c0;box-shadow:0 0 10px 2px #3c0;}
.ledn{margin:9px 10px;}
.ledn:after{display:inline-block;position:relative;top:-4px;left:-3px;width:20px;height:20px;border-radius:10px;content:'';background-color:#d00;box-shadow:0 0 10px 2px #d00;}
.ledn:checked:after{background-color:#3c0;box-shadow:0 0 10px 2px #3c0;}
.ledc{display:inline-block;width:20px;height:20px;margin-bottom:-3px;border-radius:10px;background-color:#333;box-shadow:inset 0 0 5px 2px #0000005c;}
.microButton.microButton{width:auto;min-height:30px;height:auto;padding:1px 7px;font-size:15px;}
.miniButton.miniButton{width:auto;min-height:40px;height:auto;padding:1px 7px;font-size:20px;}
.areaButton{width:auto;min-height:30px;height:auto;padding:1px 7px;border-radius:10px;}
.areaText.areaText{max-width:100%;height:30px;text-align:left;}
.switch{display:inline-block;position:relative;width:60px;height:34px;margin:14px 6px 4px 2px;}
.switch input{width:0;height:0;opacity:0;}
.slider{position:absolute;top:-5px;left:0;right:0;bottom:5px;border-radius:34px;cursor:pointer;background-color:#54545485;-webkit-transition:.1s;transition:.1s;}
.slider:before{position:absolute;height:26px;width:26px;left:4px;bottom:4px;border-radius:50%;content:'';background-color:#fff;-webkit-transition:.1s;transition:.1s;}
input:checked+.slider{background-color:#37a93c;}
input:checked+.slider:before{-webkit-transform:translateX(26px);-ms-transform:translateX(26px);transform:translateX(26px);}
output{display:inline-block;height:17px;margin:12px 5px 12px 0;padding:1.5px 3px;border-radius:5px;color:#13161a;min-width:50px;background:#37a93c;font-size:14px;cursor:default;}
.range{max-width:250px;width:70%;height:20px;margin:12px 8px;padding:0px;border-radius:5px;background:#2a2d35;background-repeat:no-repeat;background-image:linear-gradient(#37a93c,#37a93c);cursor:pointer;user-select:none;}
.range:hover,.range:active{filter:brightness(0.93);}
.rangeLarge{height:34px;width:100%;max-width:430px;margin:8px 4px;border-radius:20px;box-shadow:#000000b3 0 0 15px;}
.rangeLable{position:relative;z-index:1;left:17px;bottom:1px;width:0;color:#fff;pointer-events:none;}
.rangeColor{height:15px;padding:0;background-color:#2a2d35;border:3px solid #2a2d35;}
.rangeValue{display:inline-flex;justify-content:end;position:relative;right:70px;margin-right:-55px;background:none;color:#fff;pointer-events:none;}
.lineBar{display:block;width:124px;height:8px;margin-top:3px;margin-bottom:6px;border-radius:5px;box-shadow:#000000b3 0 0 15px;background-repeat:no-repeat;background-color:#1a1a1a;}
.display{display:inline-block;margin:2px 3px;padding:0.1em 0.3em;border-radius:8px;font-size:18px;color:#000;background:#37a93c;}
#ubtn{padding:0 10px;min-width:34px;height:40px;font-size:25px;margin:8px 5px;background-color:#4CAF50;border:none;border-radius:25px;color:#13161a;cursor:pointer;text-align:center;vertical-align:middle;line-height:160%;}
#ubtn:hover{filter:brightness(0.9);}
#ubtnclr{height:0;width:0;overflow:hidden;}
.navtab{margin:20px 8px;padding:2px;max-width:95%;border-radius:20px;background:#2a2d35;box-shadow:#00000073 0 0 10px;}
.navtab>ul{display:flex;flex-wrap:nowrap;justify-content:space-around;margin:0;padding:0;border-radius:20px;overscroll-behavior:contain;overflow-x:auto;scrollbar-width:none;background:none;}
.navtab>ul::-webkit-scrollbar{display:none;}
.navtab>ul>li{display:flex;justify-content:center;align-items:center;width:100%;min-width:90px;margin:0;padding:8px 12px;border-radius:0;cursor:pointer;overflow:hidden;background:#13161a;}
.navtab>ul>li:hover{filter:brightness(0.9);}
.navtab>ul>li:not(:last-child){margin-right:2px;}
.navbar{margin:15px 8px;padding:2px;border-radius:35px;transition-duration:.3s;}
.navbar>ul{height:55px;border-radius:35px;}
.navbar>ul>li{display:flex;justify-content:center;min-width:90px;margin:0;border-radius:0;overflow:hidden;box-shadow:none;}
.navblock{display:none;padding:0;}
.navfixed{position:fixed;bottom:0;left:0;z-index:3;width:100%;}
.navopen.navopen{background:#2a2d35;color:#fff;}
i{padding:5px 7px;vertical-align:middle;}
img.colorpick-eyedropper-input-trigger{display:none;}
input::-webkit-outer-spin-button,input::-webkit-inner-spin-button{-webkit-appearance:none;margin:0;}
.spinner{display:inline-flex;align-items:center;margin:-2px -10px;}
.spinBtn.spinBtn{height:40px;width:22px;font-size:24px;position:relative;z-index:1;font-family:monospace;padding-left:3.5px;padding-top:0.5px;}
.spinL.spinL{border-radius:10px 0 0 10px;left:10px;}
.spinR.spinR{border-radius:0 10px 10px 0;right:10px;}
#spinner input[type=number]{width:60px;max-width:72px;min-width:36px;border-radius:0;text-align:center;}
details>summary{font-size:18px;background-color:#4CAF50;color:white;cursor:pointer;padding:10px 15px;border-radius:6px;user-select:none;}
details[open]>summary{border-radius:6px 6px 0 0;}
details>summary>*{display:inline;}
details>div{border:2px solid #4CAF50;padding:10px;border-radius:0 0 4px 4px;}
form{margin:0;}
table{border-collapse:collapse;}
embed{width:90%;border-radius:5px;background:white;}
input[type=number]{-moz-appearance:textfield;}
input[type=text]:focus,input[type=number]:focus,input[type=password]:focus,input[type=date]:focus,input[type=time]:focus{border:2px solid #666}
input[type=checkbox]:disabled,input[type=text]:disabled,input[type=number]:disabled,input[type=password]:disabled,input[type=date]:disabled,input[type=time]:disabled,input[type=color]:disabled,input[type=button]:disabled,button:disabled{filter:brightness(0.6);cursor:default;box-shadow:none;}
.dsbl.dsbl{filter:brightness(0.6);cursor:default;box-shadow:none;}
.pass.pass{width:90.1%;max-width:214px;min-width:180px;padding:3px 35px;}
.eyepass{position:absolute;margin-left:-35px;margin-top:7px;cursor:pointer;font-size:25px;color:#0b0c0e;}
.inlpass{display:inline-block;position:relative}
.header{font-size:25px;color:#fff;}
.header_s{display:none;padding-bottom:5px;}
.headbar{display:flex;justify-content:flex-start;align-items:center;position:fixed;top:0;left:0;width:100%;height:40px;z-index:3;background-color:#2a2d35;box-shadow:#000 0 0 10px;}
.burgbtn{padding:0 8px;cursor:pointer;}
#menuToggle span{display:block;width:20px;height:3px;margin:4px;position:relative;background:#fff;border-radius:3px;}
.overlay{cursor:pointer;position:fixed;left:0;top:0;display:none;width:100%;height:100%;background-color:#0009;z-index:2;animation:opac .2s;}
.sidebar{z-index:3;width:250px;background-color:#2a2d35;top:0;left:-260px;height:100%;position:fixed;box-shadow:#000 0 0 10px;overflow-y:auto;scrollbar-width:none;overscroll-behavior:none;transition-duration:.2s;}
.sidebar::-webkit-scrollbar{display:none;}
.sblock{display:flex;flex-direction:column;min-height:98%;margin:0;padding-top:10px;}
.sblock>a{display:flex;margin:5px 10px;padding:10px 15px;border-radius:18px;font-size:19px;text-align:left;text-wrap:nowrap;cursor:pointer;text-decoration:none;color:#aaa;}
.sblock>a:hover{background-color:#ddd1;filter:brightness(0.95);}
.sblock>a:active{filter:brightness(0.85);}
.sblock>.sbsel:hover{filter:none;}
.sbsel.sbsel{color:#13161a;}
.page{margin-top:40px;margin-left:0;overflow-x:hidden;transition:margin-left .2s;}
.ui_block{display:none;padding:2px 5px;max-width:1000px;opacity:0;transition-duration:.4s;}
.ui_load{display:flex;align-items:center;justify-content:center;opacity:0;margin-top:50vh;transform:translateY(-20px);animation:delay .5s forwards;}
.ui_load>span{height:20px;width:20px;margin:5px;border-radius:25px;background-color:#4CAF50;animation:load 1s linear infinite;}
.ui_load>span:nth-child(1){animation-delay:.2s;}
.ui_load>span:nth-child(2){animation-delay:.1s;}
.i_mask{width:35px;height:35px;vertical-align:middle;display:inline-block;background-color:#fff;margin:5px 7px;}
.i_btn{width:fit-content;cursor:pointer;}
.i_btn:active{filter:brightness(0.8);}
.check_c>input{margin:4px -27px 6px -2px;opacity:0;}
.check_c>span::before{content:'';border-radius:8px;padding:4px 12px;color:#bbb;background-color:#2a2d35;border:1px solid #444;border-radius:0.25em;cursor:pointer;}
.check_c>input:checked+span::before{border-color:#2a2d35;background-image:url("data:image/svg+xml,%3csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 16 16'%3e%3cpath d='m12.82,3l-6.6,6.6l-3,-3-1.9,1.9l5,5.00687l8.5,-8.5-1.9,-1.9z' fill='%23bbb'/%3e%3c/svg%3e");}
.check_c>input:disabled+span::before{background-color:#e9ecef;}
.check_c>input:hover+span::before{filter:brightness(0.85);}
.lineled{display:block;height:30px;margin-top:-13px;}
.lineled>input{margin-left:0;}
.lineled>span::before{border:none;display:inline-block;width:100px;height:0;cursor:default;filter:brightness(1)!important;box-shadow:#000000b3 0 0 15px;}
.lineled>input:checked+span::before{background-image:none;}
.selBlock{border:2px solid #3c4048;max-width:300px;padding:10px}
.selItem.selItem{height:32px;width:100%;margin:0;padding:0 10px;border-radius:0;background:#2a2d35;color:#bbb;font-size:15px;white-space:normal;cursor:pointer;}
.selItem:first-child{border-top-left-radius:15px;border-top-right-radius:15px;}
.selItem:last-child{border-bottom-left-radius:15px;border-bottom-right-radius:15px;}
.selItem:not(:last-child){margin-bottom:3px;}
.selActive.selActive{background:#4CAF50;color:#13161a;}
.vr{border:1px solid #3c4048;height:50px;margin:0 10px;}
.rad{margin:3px 3px;appearance:none;cursor:pointer;}
.rad:after{width:70px;height:30px;border-radius:25px;content:'';display:inline-block;border-style:solid;border-width:2px;border-color:#37a93c;}
.rad:checked:after{background-color:#37a93c;}
.radBlock{width:70px;height:30px;margin:5px 12px 14px 2px;}
.radLable{position:relative;top:-33px;left:5px;cursor:pointer;color:#fff;}
.hintBlock{visibility:hidden;height:20px;color:#505050;font-size:14px;line-height:10px;padding:0 10px;cursor:default;}
.popup{display:none;z-index:100;position:fixed;width:100%;height:100%;overflow-y:auto;scrollbar-width:none;justify-content:center;top:0;left:0;backdrop-filter:blur(0);transition-duration:.3s;}
.popup::-webkit-scrollbar{display:none;}
.popupBlock{z-index:100;margin:auto;min-width:250px;min-height:100px;}
.offlImg{animation:offline .5s infinite alternate;}
.offlAnim{display:none;position:fixed;bottom:-5px;right:0;z-index:99;cursor:default;padding:5px;}
.uploadAnim{width:50px;height:50px;border:10px solid #2a2d35;border-top:10px solid #e67b09;border-radius:100%;margin:auto;animation:upload 1s infinite linear;}
@keyframes opac{from{opacity:0;}to{opacity:1;}}
@keyframes load{0%{transform:translateX(0);}25%{transform:translateX(15px);}50%{transform:translateX(-15px);}100%{transform:translateX(0);}}
@keyframes delay{0%{opacity:0;}30%{opacity:0;}100%{opacity:1;}}
@keyframes offline{0%{fill:#f00;}25%{fill:#f00;}100%{fill:#ff000000;}}
@keyframes upload{from{transform:rotate(0deg);}to{transform:rotate(360deg);}}
@media screen and (max-width:450px){.mainblock{padding:9px 0 9px 0;}}
@media screen and (max-width:1000px){.page{margin-top:35px;}.ui_load{transform:translateY(-40px);}.navbar{max-width:100%!important;}}
@media(min-width:1000px){
.navbar{max-width:50%;margin:30px 8px;}
.navbar>ul{height:40px;}
.burgbtn{display:none!important;}
.page{margin-left:250px;margin-top:0;}
.overlay{display:none!important;}
.headbar{display:none!important;}
.sidebar{left:0!important;}
.header_s{display:block;}}
)";
