#pragma once

// GP Themes

const char GP_DEFAULT_THEME[] PROGMEM = R"(
p{display:inline;}
.mainblock{margin:auto;max-width:100%}
a,a:visited{color:#4CAF50}
a:hover{color:#3d9941}
#blockBack input[type=password],#blockBack input[type=text],#blockBack input[type=date],#blockBack input[type=time],#blockBack input[type=number],#blockBack textarea,#blockBack select,#blockBack .slider{background:#13161a}
#blockBack input[type=range]{background:#13161a;background-repeat:no-repeat;}
#blockBack input:checked+.slider{background-color:#37a93c}
body{font-family:Verdana;background:#13161a;color:#bbb;margin-top:15px;}
hr{width:95%;margin:5px 0;border:none;border-top:2px solid #394048;}
h2{margin:8px 0;}
span{font-size:16px;}
label{white-space:nowrap;font-size:20px;margin:0 5px;}
.chartBlock{border-radius:10px;margin:3px;width:90%;}
.grid{display:flex;justify-content:space-between;}
#grid .block{width:100%;margin:15px 10px;}
.blockBase{padding:5px 10px;margin:20px 10px;}
.block{border-radius:10px;background-color:#2a2d35;box-shadow:#111 0px 0px 20px;}
.blockTab{padding-top:0px;}
.blockHeader{background-color:#4caf50;color:#13161a;font-size:22px;text-align:center;border-radius:10px 10px 0px 0px;padding:4px;margin:0px -10px 10px -10px;}
.thinBlock{background:#13161a;border:2px solid #3c4048;}
.thinText{padding:0px 7px;white-space:nowrap;background:linear-gradient(0deg,#13161a 0,#13161a 54%,#13161a94 55%,#00000000 60%);font-size:20px;}
.thinTab{background:none;border:none;color:#9c9ea1;text-align:left;padding:4px 0 0 25px;margin:-17px -10px 5px -10px;display:block;}
.inliner{display:flex;flex-direction:row;align-content:center;flex-wrap:nowrap;align-items:center;width:100%;}
input[type=number],input[type=text],input[type=password],input[type=date],input[type=time],input[type=color],input[type=checkbox],select,textarea{
width:90%;max-width:200px;border-radius:8px;padding:3px 10px;color:#bbb;border:none;background-color:#2a2d35;vertical-align:middle;position:relative;margin:6px 4px;font-size:20px;height:40px;cursor:pointer;}
input[type=color]{width:80px;height:40px;}
input[type=range]:disabled:disabled{filter:brightness(0.6);cursor:auto;}
input[type=range]::-moz-range-thumb{-moz-appearance:none;border:none;height:0px;width:0px;}
input[type=checkbox]{width:27px;height:27px;margin-bottom:10px;}
input[type=time],input[type=date]{border:none!important;}
input[type=number],input[type=text],input[type=password]{padding-top:5px!important;}
input[type=number],input[type=text],input[type=password],input[type=time],input[type=date],select,textarea{text-align:center;appearance:none;}
input[type=number],input[type=text],input[type=password],select,textarea{display:inline-block;border-radius:8px;box-sizing:border-box;cursor:auto;}
select{min-width:130px;width:200px;cursor:pointer;}
select:disabled{filter:brightness(0.6);cursor:default;}
textarea{text-align:left;resize:none;overflow:hidden;max-width:100%;width:100%;box-sizing:unset;}
input[type=submit],input[type=button],button{height:55px;font-size:24px;width:90%;max-width:300px;margin:8px 5px;background-color:#4CAF50;border:none;border-radius:28px;line-height:90%;color:#13161a;cursor:pointer;}
input[type=submit]:hover,input[type=button]:hover,button:hover{filter:brightness(0.95);}
input[type=submit]:active,input[type=button]:active,button:active{box-shadow:inset 0px 1px 6px #0007;}
input:focus,select,textarea:focus{outline:none;}
.led{margin:9px 10px;}
.led:after{width:20px;height:20px;border-radius:10px;top:-4px;left:-3px;position:relative;content:'';display:inline-block;background-color:#333;box-shadow:inset 0px 3px 0px 0px #fff5}
.led:checked:after{box-shadow:0px 0px 10px 4px;}
.led.red:checked:after{background-color:#d00;box-shadow:inset 0px 3px 0px 0px #fff7,0px 0px 10px 4px #f00b;}
.led.green:checked:after{background-color:#3c0;box-shadow:inset 0px 3px 0px 0px #fff7,0px 0px 10px 4px #4d08;}
.ledn{margin:9px 10px;}
.ledn:after{width:20px;height:20px;border-radius:10px;top:-4px;left:-3px;position:relative;content:'';display:inline-block;background-color:#d00;box-shadow:inset 0px 3px 0px 0px #fff7,0px 0px 10px 2px #f00;}
.ledn:checked:after{background-color: #3c0;box-shadow:inset 0px 3px 0px 0px #fff7,0px 0px 10px 2px #4d0;}
.ledc{width:20px;height:20px;border-radius:10px;display:inline-block;margin-bottom:-3px;background-color:#333;box-shadow:inset 0px 0px 5px 2px #0000005c;}
.microButton.microButton{padding:1px 7px;min-height:30px;height:auto;font-size:15px;width:auto;}
.miniButton.miniButton{padding:1px 7px;min-height:40px;height:auto;font-size:20px;width:auto;}
.areaButton{padding:1px 7px;min-height:30px;height:auto;width:auto;border-radius:8px;}
.areaText.areaText{max-width:100%;text-align:left;height:30px;}
.switch{margin:14px 6px 4px 2px;position:relative;display:inline-block;width:60px;height:34px}
.switch input{opacity:0;width:0;height:0}
.slider{border-radius:34px;position:absolute;cursor:pointer;top:-5px;left:0;right:0;bottom:5px;background-color:#54545485;-webkit-transition:.1s;transition:.1s}
.slider:before{border-radius:50%;position:absolute;content:'';height:26px;width:26px;left:4px;bottom:4px;background-color:#fff;-webkit-transition:.1s;transition:.1s}
input:checked+.slider{background-color:#37a93c;}
input:checked+.slider:before{-webkit-transform:translateX(26px);-ms-transform:translateX(26px);transform:translateX(26px);}
.slMaxInput.slMaxInput{height:34px;width:100%;max-width:430px;margin:8px 4px;border-radius:20px;box-shadow:0 0 15px rgba(0, 0, 0, 0.7);}
.slMaxLable{color:#fff;position:relative;z-index:1;left:17px;bottom:1px;width:0px;pointer-events:none;}
.slMaxValue{position:relative;right:70px;margin-right:-55px;background:none;color:#fff;display:inline-flex;justify-content:end;pointer-events:none;}
.lineBar.lineBar{filter:none!important;box-shadow:0 0 15px rgba(0, 0, 0, 0.7);background-color:#1a1a1a;display:block;width:124px;height:8px;margin-top:3px;margin-bottom:6px;}
output{display:inline-block;font-size:14px;padding:2px 3px;border-radius:5px;color:#13161a;min-width:50px;background:#37a93c;margin-right:5px;}
input[type=range]{max-width:250px;-webkit-appearance:none;width:70%;margin:12px 8px;height:20px;background:#2a2d35;border-radius:5px;background-repeat:no-repeat;cursor:pointer;padding:0px;background-image:linear-gradient(#37a93c,#37a93c);}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;height:15px;width:15px;}
input[type=range]:hover{filter:brightness(0.93);}
.display{display:inline-block;font-size:18px;padding:0.1em 0.2em;color:#000;background:#37a93c;margin:2px 3px;border-radius:5px;}
#ubtn{padding:0px 10px;min-width:34px;height:40px;font-size:25px;margin:8px 5px;background-color:#4CAF50;border:none;border-radius:25px;color:#13161a;cursor:pointer;text-align:center;vertical-align:middle;line-height:160%;}
#ubtn:hover{filter:brightness(0.9);}
#ubtnclr{height:0px;width:0px;overflow:hidden;}
.navtab>ul{display:flex;flex-wrap:wrap;align-items:stretch;justify-content:center;list-style-type:none;margin:15px 0 5px 0;padding:0;overflow:hidden;border-radius:5px;background:none!important;}
.navtab>ul>li{display:block;border-radius:25px;align-items:center;cursor:pointer;background:#13161a;color:#bbb;margin:5px;padding:8px 12px;width:40%;box-shadow:rgba(0, 0, 0, 0.45) 0px 0px 15px;}
.navtab>ul>li:hover{filter:brightness(0.9);}
.navblock{display:none;padding:0;}
.navopen.navopen{background:#2a2d35;color:#fff;}
i{padding:5px 7px;vertical-align:middle;}
img.colorpick-eyedropper-input-trigger{display:none;}
input::-webkit-outer-spin-button,input::-webkit-inner-spin-button{-webkit-appearance:none;margin:0;}
.spinner{display:inline-flex;align-items:center;margin:-2px -10px;}
.spinBtn.spinBtn{height:40px;width:22px;font-size:24px;position:relative;z-index:1;font-family:monospace;padding-left:3.5px;padding-top:0.5px;}
.spinL.spinL{border-radius:8px 0 0 8px;left:10px;}
.spinR.spinR{border-radius:0 8px 8px 0;right:10px;}
#spinner input[type=number]{width:60px;border-radius:0px;text-align:center;}
details>summary{font-size:18px;background-color:#4CAF50;color:white;cursor:pointer;padding:10px 15px;border-radius:6px;user-select:none;}
details[open]>summary{border-radius:6px 6px 0 0;}
details>summary>*{display:inline;}
details>div{border:2px solid #4CAF50;padding:10px;border-radius:0 0 4px 4px;}
textarea::-webkit-scrollbar{width:6px;}
textarea::-webkit-scrollbar-track{background-color:#0000;}
textarea::-webkit-scrollbar-thumb{background-color:#556;border-radius:3px;}
body::-webkit-scrollbar{width:8px;}
body::-webkit-scrollbar-track{background-color:#0000;}
body::-webkit-scrollbar-thumb{background-color:#556;border-radius:4px;}
.dsbl{filter:brightness(0.6);cursor:auto;}
:root{color-scheme:light dark;}
html{scrollbar-width:none;}
body{-webkit-tap-highlight-color:transparent;}
form{margin:0;}
input[type=text]:focus,input[type=number]:focus,input[type=password]:focus,input[type=date]:focus,input[type=time]:focus{border:2px solid #666}
input[type=checkbox]:disabled,input[type=text]:disabled,input[type=number]:disabled,input[type=password]:disabled,input[type=date]:disabled,input[type=time]:disabled,input[type=color]:disabled,input[type=button]:disabled,button:disabled{filter:brightness(0.6);cursor:default;}
.eyepass{position:absolute;margin-left:-35px;margin-top:7px;cursor:pointer;font-size:25px;color:#0b0c0e;}
.inlBlock{display:inline-block;position:relative}
table{border-collapse:collapse;}
input[type=number]{-moz-appearance:textfield;}
.header{font-size:25px;color:#fff;}
.header_s{display:none;padding-bottom:5px;}
.headbar{height:40px;z-index:3;background-color:#2a2d35;position:fixed;top:0;left:0;width:100%;display:flex;justify-content:flex-start;align-items:center;}
.burgbtn{padding:0 8px;cursor:pointer;}
#menuToggle span{display:block;width:20px;height:3px;margin:4px;position:relative;background:#fff;border-radius:3px;}
.sidebar{z-index:3;width:250px;background-color:#2a2d35;top:0;left:-250px;height:100%;position:fixed;overflow:auto;box-shadow:#000 0px 0px 10px;transition-duration:.2s;scrollbar-width:none;}
@keyframes opac{from{opacity:0}to{opacity:1}}
.overlay{cursor:pointer;position:fixed;left:0;top:0;display:none;width:100%;height:100%;background-color:#0009;z-index:2;animation:opac .2s;}
.page{margin-top:40px;margin-left:0px;transition:margin-left .2s;}
.sbsel.sbsel{color:#13161a;}
.sblock{display:flex;flex-direction:column;min-height:98%;margin:0;padding-top:10px;}
.sblock>a{font-size:18px;text-align:left;cursor:pointer;padding:10px 15px;text-decoration:none;display:flex;color:#aaa;letter-spacing:1px;margin:5px 10px;border-radius:25px;}
.sblock>a:active{color:#13161a;}
.sblock>a:hover{background-color:#ddd1;filter:brightness(0.9);}
.ui_block{padding:2px 5px;max-width:1000px;}
@media screen and (max-width:1000px){.page{margin-top:43px;}.onlBlock{top:-5px;}}
@media screen and (max-width:1100px){.grid{display:block;}#grid .block{margin:20px 10px;width:unset;}}
@media(min-width:1000px){
.burgbtn{display:none!important;}
.page{margin-left:250px;margin-top:0px;}
.overlay{display:none!important;}
.headbar{display:none!important;}
.sidebar{left:0!important;}
.header_s{display:block;}}
.i_mask{width:35px;height:35px;vertical-align:middle;display:inline-block;background-color:#fff;margin:5px 7px;}
.i_btn{width:fit-content;cursor:pointer}
.i_btn:hover{filter:brightness(0.8);}
.check_c>input{margin:4px -27px 6px -2px;opacity:0;}
.check_c>span::before{content:'';border-radius:8px;padding:4px 12px;color:#bbb;background-color:#2a2d35;border:1px solid #444;border-radius:0.25em;cursor:pointer;}
.check_c>input:checked+span::before{border-color:#e67b09;background-color:#e67b09;background-image:url("data:image/svg+xml,%3csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 16 16'%3e%3cpath d='m12.82,3l-6.6,6.6l-3,-3-1.9,1.9l5,5.00687l8.5,-8.5-1.9,-1.9z' fill='%23fff'/%3e%3c/svg%3e");}
.check_c>input:disabled+span::before{background-color:#e9ecef;}
.check_c>input:hover+span::before{filter:brightness(0.85);}
.onlBlock{display:none;position:fixed;bottom:0;right:0;z-index:99;cursor:default;padding:5px;}
.ctrlBlock{position:absolute;top:5px;right:9px;}
.areaCtrl{padding:3px;font-size:13px;cursor:pointer;}
.vr{border:1px solid #3c4048;height:50px;margin:0 10px;}
.rad{margin:3px 3px;appearance:none;cursor:pointer;}
.rad:after{width:70px;height:30px;border-radius:25px;content:'';display:inline-block;border-style:solid;border-width:2px;border-color:#37a93c;}
.rad:checked:after{background-color:#37a93c;}
.radBlock{width:70px;height:30px;margin:5px 12px 14px 2px;}
.radLable{position:relative;top:-33px;left:5px;cursor:pointer;color:#fff;}
.popup{display:none;z-index:100;position:fixed;width:100%;height:100%;overflow-y:auto;scrollbar-width:none;justify-content:center;top:0;left:0;backdrop-filter:blur(0);transition-duration:.3s;}
.popupBlock{z-index:100;margin:auto;min-width:250px;min-height:100px;}
.loadBlock{width:50px;height:50px;border:10px solid #2a2d35;border-top:10px solid #e67b09;border-radius:100%;margin:auto;animation:load 1s infinite linear;}
@keyframes load{from{transform:rotate(0deg);}to{transform:rotate(360deg);}
)";
