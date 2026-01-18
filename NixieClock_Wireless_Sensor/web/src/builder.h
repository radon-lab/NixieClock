#pragma once

// GP Builder

#include <FS.h>

struct Builder {
  uint8_t _gp_nav_pos = 0;
  uint8_t _gp_nav_id = 0;
  int _spin_prd = 200;

  // время
  uint32_t _timeUpdPrd = 10 * 60 * 1000ul;
  void setTimeUpdatePeriod(uint32_t t) {
    _timeUpdPrd = t;
  }
  void updateTime(void) {
    if (!_gp_unix_tmr || millis() - _gp_unix_tmr >= _timeUpdPrd) {
      SEND(F("<script>EVsend('/EV_time?unix='+Math.round(new Date().getTime()/1000)+'&gmt='+(-new Date().getTimezoneOffset()));</script>\n"));
    }
  }

  // период изменения значения при удержании кнопки спиннера
  void setSpinnerPeriod(int prd) {
    _spin_prd = prd;
  }

  // установить таймаут ожидания отправки килков и апдейтов
  void setTimeout(int tout) {
    *_GPP += F("<script>_tout=");
    *_GPP += tout;
    *_GPP += F(";</script>\n");
  }

  void floatDec(float val, uint16_t dec) {
    if (!dec) *_GPP += (int)round(val);
    else *_GPP += String(val, (uint16_t)dec);
  }

  // ===================== ОТПРАВКА RAW =====================
  void SEND(const String& s) {
    *_GPP += s;
    send();
  }
  void SEND_P(PGM_P s) {
    send(true);
    _gp_server->sendContent_P(s);
  }

  void send(bool force = 0) {
    if ((int)_GPP->length() > (force ? 0 : _gp_bufsize)) {
      _gp_server->sendContent(*_GPP);
      *_GPP = "";
    }
  }

  // ======================= БИЛДЕР =======================
  void BUILD_BEGIN(void) {
    PAGE_BEGIN();
    JS_TOP();
  }
  void BUILD_BEGIN(PGM_P style) {
    PAGE_BEGIN();
    THEME(style);
    JS_TOP();
  }

  void BUILD_BEGIN_FILE(void) {
    PAGE_BEGIN();
    JS_TOP_FILE();
  }
  void BUILD_BEGIN_FILE(const String& style) {
    PAGE_BEGIN();
    THEME_FILE(style);
    JS_TOP_FILE();
  }

  void BUILD_END(void) {
    JS_BOTTOM();
    PAGE_END();
  }

  void PAGE_ZOOM(int zoom, int width = 0) {
    *_GPP += F("<script>");
    *_GPP += F("setZoom(");
    *_GPP += width;
    *_GPP += ',';
    *_GPP += zoom;
    *_GPP += F(");</script>\n");
  }

  void PAGE_TITLE(const String& text = "", const String& name = "") {
    if (name.length()) HIDDEN(name, F("_title"), text);
    if (text.length()) {
      *_GPP += F("<script>document.title='");
      *_GPP += text;
      *_GPP += F("';</script>\n");
    }
  }

  void PAGE_MIDDLE_ALIGN(void) {
    SEND(F("<style>body{display:flex;justify-content:center;}.mainblock{width:100%;}</style>\n"));
  }

  // ========================== UI БЛОК ==========================
  PGM_P _ui_style = GP_GREEN;

  void UI_BEGIN(const String& title, const String& urls, const String& names, PGM_P st = GP_GREEN, int w = 1000) {
    UI_MENU(title, st);
    GP_parser n(names);
    GP_parser u(urls);
    while (n.parse()) {
      u.parse();
      UI_LINK(u.str, n.str);
    }
    UI_BODY(w);
  }

  void UI_MENU(const String& title, PGM_P st = GP_GREEN) {
    _ui_style = st;
    *_GPP += F("<style>@media screen and (max-width:1000px){.offlAnim{top:-5px;}}.mainblock{width:auto!important;max-width:100%!important;}</style>\n");
    *_GPP += F("<div class='headbar'><div class='burgbtn' id='menuToggle' onclick='sdbTgl()'><span></span><span></span><span></span></div>\n<div class='header'>");
    *_GPP += title;
    *_GPP += F("</div></div>\n<nav class='sidebar' id='dashSdb'><div class='sblock'><div class='header header_s'>");
    *_GPP += title;
    *_GPP += F("</div>\n");
    send();
  }
  void UI_MENU(const String& title, const String& name, PGM_P st_1 = GP_GREEN, PGM_P st_2 = GP_GRAY) {
    _ui_style = st_1;
    *_GPP += F("<style>.mainblock{width:auto!important;max-width:100%!important;}</style>\n");
    *_GPP += F("<div class='headbar'><div class='burgbtn' id='menuToggle' onclick='sdbTgl()'><span></span><span></span><span></span></div>\n<div class='header'>");
    if (name.length()) *_GPP += name;
    else *_GPP += title;
    *_GPP += F("</div></div>\n<nav class='sidebar' id='dashSdb'><div class='sblock'><div class='header'>");
    *_GPP += title;
    *_GPP += F("</div><div class='header header_s' style='padding:0'>");
    if (name.length()) {
      *_GPP += F("<label style='color:");
      *_GPP += FPSTR(st_2);
      *_GPP += F("'>");
      *_GPP += name;
      *_GPP += F("</label>");
    }
    *_GPP += F("</div>\n");
    send();
  }

  void UI_BODY(int w = 1000, PGM_P st = GP_DEFAULT) {
    *_GPP += F("</div></nav>\n<div class='overlay' onclick='sdbTgl()' id='dashOver'></div><div class='page'>\n"
               "<div class='ui_load'><span></span><span></span><span></span>");
    if (st != GP_DEFAULT) {
      *_GPP += F("<style>.ui_load>span{background-color:");
      *_GPP += FPSTR(st);
      *_GPP += F(";}</style>\n");
    }
    *_GPP += F("</div>\n<div class='ui_block'");
    if (w != 1000) {
      *_GPP += F(" style='max-width:");
      *_GPP += w;
      *_GPP += F("px'");
    }
    *_GPP += ">\n";
    send();
  }
  void UI_END(void) {
    SEND(F("</div></div>\n"));
  }

  void UI_LINK(const String& url, const String& name) {
    *_GPP += F("<a href='");
    *_GPP += url;
    *_GPP += "'";
    if (_gp_uri->equals(url)) {
      *_GPP += F(" class='sbsel' style='background:");
      *_GPP += FPSTR(_ui_style);
      *_GPP += F(" !important;'");
    }
    *_GPP += ">";
    *_GPP += name;
    *_GPP += F("</a>\n");
    send();
  }

  void UI_LINKS_BEGIN(const String& id) {
    *_GPP += F("<div class='_link' id='");
    *_GPP += id;
    *_GPP += F("' style='display:none'>");
    send();
  }
  void UI_LINKS_BLOCK(void) {
    SEND(F("<div id='_link_block' class='sblock' style='padding:0'></div>"));
  }
  void UI_LINKS_END(void) {
    SEND(F("</div>"));
  }
  void UI_LINKS_SEND(const String& id, String list = "") {
    if (list.length()) {
      list.replace("\"", "\\\"");
      *_GPP += F("<script>linkUpdate('");
      *_GPP += id;
      *_GPP += F("',\"");
      *_GPP += list;
      *_GPP += F("\");</script>");
      send();
    }
  }

  // ======================= СТРАНИЦА =======================
  void PAGE_BEGIN(void) {
    _gp_nav_id = 0;
    SEND(F("<!DOCTYPE HTML><html><head>\n"
           "<meta charset='utf-8'>\n"
           "<meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=0'>\n"
           "<meta name='apple-mobile-web-app-capable' content='yes'/>\n"
           "<meta name='mobile-web-app-capable' content='yes'/>\n"
           "</head><body>\n"));
  }
  void PAGE_END(void) {
    SEND(F("</body></html>"));
  }

  void PAGE_BLOCK_BEGIN(int width = 350) {
    *_GPP += F("<div id='_popup' class='popup'></div>\n<div align='center' class='mainblock'");
    if (width) {
      *_GPP += F(" style='max-width:");
      *_GPP += width;
      *_GPP += F("px'");
    }
    *_GPP += ">\n";
    send();
  }
  void PAGE_BLOCK_END(void) {
    *_GPP += F("</div>\n<div id='offlAnim' class='offlAnim'>");
    *_GPP += F("<svg width='40px' height='40px' xmlns='http://www.w3.org/2000/svg' xml:space='preserve' viewBox='0 0 1024 1024'>"
               "<path d='M928.99 755.83 574.6 203.25c-12.89-20.16-36.76-32.58-62.6-32.58s-49.71 12.43-62.6 32.58L95.01 755.83c-12.91 "
               "20.12-12.9 44.91.01 65.03 12.92 20.12 36.78 32.51 62.59 32.49h708.78c25.82.01 49.68-12.37 62.59-32.49 12.91-20.12 "
               "12.92-44.91.01-65.03zM554.67 768h-85.33v-85.33h85.33V768zm0-426.67v298.66h-85.33V341.32l85.33.01z' "
               "fill='#f00' class='offlImg'></path></svg></div>\n"
               "<div class='_popup' id='uploadAnim' style='display:none'><div class='popupBlock'><div class='uploadAnim'></div></div></div>\n");
  }

  void THEME(PGM_P style) {
    *_GPP += F("<link rel='stylesheet' href='/GP_STYLE.css?v" GP_VERSION "=");
    *_GPP += ((unsigned long)style) & 0xFFFF;
    *_GPP += "'";
    *_GPP += ">\n";
    _gp_style = style;
  }
  void THEME_FILE(const String& style) {
    *_GPP += F("<link rel='stylesheet' href='/gp_data/");
    *_GPP += style;
    *_GPP += F(".css?=" GP_VERSION "'>\n");
    send();
  }

  void JS_TOP_FILE(void) {
    SEND(F("<script src='/gp_data/scripts.js?=" GP_VERSION "'></script>\n"));
    updateTime();
  }
  void JS_TOP(void) {
    *_GPP += F("<script src='/GP_SCRIPT.js?v" GP_VERSION "=");
    *_GPP += _gp_seed;
    *_GPP += F("'></script>\n");
    updateTime();
  }
  void JS_BOTTOM(void) {
    SEND(F("<script>pageUpdate();</script>\n"));
  }

  void SPOILER_BEGIN(const String& text, PGM_P st = GP_GREEN) {
    *_GPP += F("<details><summary align='left' style='");
    if (st != GP_GREEN) {
      *_GPP += F("background-color:");
      *_GPP += FPSTR(st);
      *_GPP += ';';
    }
    *_GPP += F("'>");
    *_GPP += text;
    *_GPP += F("</summary><div align='center' style='");
    if (st != GP_GREEN) {
      *_GPP += F("border-color:");
      *_GPP += FPSTR(st);
      *_GPP += ';';
    }
    *_GPP += F("'>\n");
    send();
  }
  void SPOILER_END(void) {
    SEND(F("</div></details>\n"));
  }

  void HINT(const String& name, const String& txt) {
    *_GPP += F("<script>EVhint('");
    *_GPP += name;
    *_GPP += F("','");
    *_GPP += txt;
    *_GPP += F("');</script>\n");
    send();
  }

  void HINT_BOX(const String& name, const String& min, const String& max, const String& txt) {
    *_GPP += F("<div id='");
    *_GPP += name;
    *_GPP += F("' class='hintBlock'>");
    *_GPP += txt;
    *_GPP += F("</div>\n<script>function ");
    *_GPP += name;
    *_GPP += F("(){EVhintBox('");
    *_GPP += min;
    *_GPP += F("','");
    *_GPP += max;
    *_GPP += F("','");
    *_GPP += name;
    *_GPP += F("');}\nEVhintLoad('");
    *_GPP += min;
    *_GPP += F("','");
    *_GPP += max;
    *_GPP += F("',");
    *_GPP += name;
    *_GPP += F(");</script>\n");
    send();
  }

  void JS_BEGIN(void) {
    *_GPP += F("<script>\n");
  }
  void JS_END(void) {
    *_GPP += F("\n</script>\n");
  }

  void ONLINE_CHECK(int prd = 3000) {
    *_GPP += F("<script>setInterval(function(){if(!document.hidden){var xhttp=new XMLHttpRequest();xhttp.timeout=");
    *_GPP += prd;
    *_GPP += F(";xhttp.open('GET','/EV_ping?',true);xhttp.send();\n"
               "xhttp.onreadystatechange=function(){onlShow(!this.status)}}},");
    *_GPP += prd;
    *_GPP += F(");</script>\n");
    send();
  }

  // ======================= HIDDEN =======================
  void HIDDEN(const String& name, const String& value) {
    *_GPP += F("<input type='hidden' name='");
    *_GPP += name;
    *_GPP += F("' value='");
    *_GPP += value;
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += F("'>\n");
    send();
  }
  void HIDDEN(const String& id, const String& name, const String& value) {
    *_GPP += F("<input type='hidden' name='");
    *_GPP += name;
    *_GPP += F("' value=\"");
    *_GPP += value;
    *_GPP += F("\" id='");
    *_GPP += id;
    *_GPP += F("'>\n");
    send();
  }

  // ======================= UPDATE =======================
  void UPDATE(const String& list, int prd = 1000) {
    *_GPP += F("<script>setInterval(function(){if(!document.hidden)EVupdate('");
    *_GPP += list;
    *_GPP += F("')},");
    *_GPP += prd;
    *_GPP += F(");</script>\n");
    send();
  }

  // перезагрузка. Имя нужно также указать в списке update
  void RELOAD(const String& name) {
    HIDDEN(name, F("_reload"), "");
  }

  // список имён компонентов, изменение (клик) которых приведёт к перезагрузке страницы
  void RELOAD_CLICK(const String& list) {
    *_GPP += F("<script>_clkRelList=_clkRelList.concat('");
    *_GPP += list;
    *_GPP += F("'.split(','));</script>\n");
    send();
  }

  // вызвать update у компонентов в списке list при клике по компонентам из списка names
  void UPDATE_CLICK(const String& list, const String& names) {
    *_GPP += F("<script>_clkUpdList['");
    *_GPP += names;
    *_GPP += F("']='");
    *_GPP += list;
    *_GPP += F("';</script>\n");
    send();
  }

  // выполнить код по ответу на обновление
  void EVAL(const String& name, const String& code = "") {
    HIDDEN(name, F("_eval"), code);
  }

  // ====================== ТАБЛИЦЫ ======================
  GPalign* _als = nullptr;
  int _alsCount = 0;

  void TABLE_BORDER(bool show) {
    *_GPP += F("<style>td{border:");
    *_GPP += show ? F("1px solid") : F("none");
    *_GPP += F("}</style>\n");
  }

  void TABLE_BEGIN(const String& tdw = "", GPalign* als = nullptr, const String& w = "100%") {
    _als = als;
    *_GPP += F("<table width='");
    *_GPP += w;
    *_GPP += F("'>\n");
    send();

    if (tdw.length()) {
      //TR();
      *_GPP += F("<tr style='visibility:collapse;'>\n");
      GP_parser p(tdw);
      while (p.parse()) {
        if (p.str.length()) {
          *_GPP += F("<td width='");
          *_GPP += p.str;
          *_GPP += F("'>\n");
        }
      }
    }
  }
  void TR(GPalign al = GP_CENTER) {
    _alsCount = 0;
    *_GPP += F("<tr align='");
    *_GPP += FPSTR(GPgetAlign(al));
    *_GPP += F("'>\n");
  }
  void TD(GPalign al = GP_CENTER, uint8_t cs = 1, uint8_t rs = 1) {
    *_GPP += F("<td");
    if (al != GP_CENTER || _als) {
      *_GPP += F(" align=");
      if (al == GP_CENTER && _als && _als[_alsCount] >= 0 && _als[_alsCount] <= 3) *_GPP += FPSTR(GPgetAlign(_als[_alsCount++]));
      else *_GPP += FPSTR(GPgetAlign(al));
    }
    if (cs > 1) {
      *_GPP += F(" colspan=");
      *_GPP += cs;
    }
    if (rs > 1) {
      *_GPP += F(" rowspan=");
      *_GPP += rs;
    }
    *_GPP += ">\n";
    send();
  }
  void TABLE_END() {
    _als = nullptr;
    SEND(F("</table>\n"));
  }


  // ====================== ПОПАПЫ =======================
  void ALERT(const String& name) {
    HIDDEN(name, F("_alert"), "");
    send();
  }
  void ALERT(const String& name, const String& text) {
    HIDDEN(name, F("_alert"), text);
    send();
  }

  // отправит null если нажать отмена
  void PROMPT(const String& name) {
    HIDDEN(name, F("_prompt"), "");
    send();
  }
  void PROMPT(const String& name, const String& text) {
    HIDDEN(name, F("_prompt"), text);
    send();
  }

  void CONFIRM(const String& name) {
    HIDDEN(name, F("_confirm"), "");
    send();
  }
  void CONFIRM(const String& name, const String& text) {
    HIDDEN(name, F("_confirm"), text);
    send();
  }

  void POPUP_BEGIN(const String& id, const String& width = "") {
    *_GPP += F("<div class='_popup' id='");
    *_GPP += id;
    *_GPP += F("' style='display:none'>\n<div class='popupBlock'");
    if (width.length()) {
      *_GPP += F(" style='width:");
      *_GPP += width;
      *_GPP += '\'';
    }
    *_GPP += '>';
    send();
  }
  void POPUP_END(void) {
    SEND(F("</div>\n</div>\n"));
  }

  // список имён компонентов, изменение (клик) которых приведёт к закрытию popup
  void POPUP_CLOSE(const String& list) {
    *_GPP += F("<script>_clkCloseList=_clkCloseList.concat('");
    *_GPP += list;
    *_GPP += F("'.split(','));</script>\n");
    send();
  }

  void POPUP_OPEN(const String& id) {
    *_GPP += F("<script>popupOpen(getPop('");
    *_GPP += id;
    *_GPP += F("'));</script>\n");
    send();
  }

  // ======================= ФОРМА =======================
  void FORM_BEGIN(const String& name) {
    *_GPP += F("<form action='");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += F("' method='POST'>\n<input type='hidden' name='EV_form' value='");
    *_GPP += name;
    *_GPP += F("'>\n");
    send();
  }
  void FORM_END(void) {
    SEND(F("</form>\n"));
  }

  void SUBMIT(const String& text, PGM_P st = GP_GREEN, const String& cls = "") {
    *_GPP += F("<input type='submit' value='");
    *_GPP += text;
    *_GPP += F("' style='background:");
    *_GPP += FPSTR(st);
    if (cls.length()) {
      *_GPP += F("' class='");
      *_GPP += cls;
    }
    *_GPP += F("'>\n");
    send();
  }
  void SUBMIT_MINI(const String& text, PGM_P st = GP_GREEN) {
    SUBMIT(text, st, F("miniButton"));
  }

  void FORM_SEND(const String& text, const String& url = "", PGM_P st = GP_GREEN, const String& cls = "") {
    *_GPP += F("<input type='button' onclick='EVsendForm(this.parentNode.id,\"");
    *_GPP += url;
    *_GPP += F("\")' value='");
    *_GPP += text;
    if (st != GP_GREEN) {
      *_GPP += F("' style='background:");
      *_GPP += FPSTR(st);
    }
    if (cls.length()) {
      *_GPP += F("' class='");
      *_GPP += cls;
    }
    *_GPP += F("'>\n");
    send();
  }
  void FORM_SEND_MINI(const String& text, const String& url = "", PGM_P st = GP_GREEN) {
    FORM_SEND(text, url, st, F("miniButton"));
  }

  void FORM_SUBMIT(const String& name, const String& text, PGM_P st = GP_GREEN) {
    FORM_BEGIN(name);
    SUBMIT(text, st);
    FORM_END();
  }
  void FORM_SUBMIT(const String& name, const String& text, const String& namehidden, const String& valuehidden, PGM_P st = GP_GREEN) {
    FORM_BEGIN(name);
    HIDDEN(namehidden, valuehidden);
    SUBMIT(text, st);
    FORM_END();
  }

  // ======================= ОФОРМЛЕНИЕ =======================
  void GRID_BEGIN(int width = 0) {
    *_GPP += F("<div class='grid' id='grid'");
    if (width) {
      *_GPP += F(" style='max-width:");
      *_GPP += width;
      *_GPP += F("px'");
    }
    *_GPP += ">\n";
    send();
  }
  void GRID_END(void) {
    BLOCK_END();
  }
  void GRID_RESPONSIVE(int width) {
    *_GPP += F("<style type='text/css'>@media screen and (max-width:");
    *_GPP += width;
    *_GPP += F("px){\n.grid{display:block;}\n#grid .block{margin:20px 5px;width:unset;}}</style>\n");
    send();
  }

  void BLOCK_BEGIN(GPblock type, const String& width = "", const String& text = "", PGM_P st = GP_DEFAULT, PGM_P bg = GP_DEFAULT) {
    *_GPP += F("<div class='");
    if (type != GP_DIV_RAW) *_GPP += F("blockBase");
    if (type != GP_DIV && type != GP_DIV_RAW) {
      *_GPP += F(" block");
      if (text.length()) *_GPP += F(" blockTab");
      if (type == GP_THIN) *_GPP += F(" thinBlock");
    }
    *_GPP += '\'';
    if (type == GP_TAB) *_GPP += F(" id='blockBack'");

    *_GPP += F(" style='");
    if (type == GP_THIN && st != GP_DEFAULT) {
      *_GPP += F("border:2px solid ");
      *_GPP += FPSTR(st);
      *_GPP += ';';
    }
    if (bg != GP_DEFAULT) {
      *_GPP += F("background:");
      *_GPP += FPSTR(bg);
      *_GPP += ';';
    }
    if (width.length()) {
      *_GPP += F("max-width:");
      *_GPP += width;
      *_GPP += ';';
    }
    *_GPP += F("'>\n");

    if (text.length()) {
      if (type == GP_DIV || type == GP_DIV_RAW) {
        LABEL(text);
        HR();
      } else if (type == GP_TAB) {
        *_GPP += F("<div class='blockHeader'");
        if (st != GP_DEFAULT) {
          *_GPP += F(" style='background:");
          *_GPP += FPSTR(st);
          *_GPP += '\'';
        }
        *_GPP += '>';
        *_GPP += text;
        *_GPP += F("</div>\n");
      } else if (type == GP_THIN) {
        *_GPP += F("<div class='blockHeader thinTab'>");
        *_GPP += F("<span class='thinText' style='");
        if (st != GP_DEFAULT) {
          *_GPP += F("color:");
          *_GPP += FPSTR(st);
          *_GPP += ';';
        }
        if (bg != GP_DEFAULT) {
          *_GPP += F("background:linear-gradient(0deg,");
          *_GPP += FPSTR(bg);
          *_GPP += F(" 0%,");
          *_GPP += FPSTR(bg);
          *_GPP += F(" 53%,#00000000 53%,#00000000 100%);");
        }
        *_GPP += F("'>");
        *_GPP += text;
        *_GPP += F("</span></div>\n");
      }
    }
    send();
  }

  void BLOCK_BEGIN(const String& width = "") {
    BLOCK_BEGIN(GP_TAB, width);
  }
  void BLOCK_END(void) {
    SEND(F("</div>\n"));
  }

  void BLOCK_OFFSET_BEGIN(void) {
    SEND(F("<div class='blockOfst'>\n"));
  }
  void BLOCK_HIDE_BEGIN(void) {
    SEND(F("<div style='overflow:hidden'>\n"));
  }

  void BLOCK_TAB_BEGIN(const String& label, const String& width = "", PGM_P st = GP_DEFAULT) {
    BLOCK_BEGIN(GP_TAB, width, label, st);
  }
  void BLOCK_THIN_BEGIN(const String& width = "") {
    BLOCK_BEGIN(GP_THIN, width);
  }
  void BLOCK_THIN_TAB_BEGIN(const String& label, const String& width = "") {
    BLOCK_BEGIN(GP_THIN, width, label);
  }

  void BLOCK_SHADOW_BEGIN(void) {
    SEND(F("<div class='blockShadow'>\n"));
  }
  void BLOCK_SHADOW_END(void) {
    SEND(F("</div>\n"));
  }

  void BOX_BEGIN(GPalign al = GP_JUSTIFY, const String& w = "100%", bool top = 0) {
    *_GPP += F("<div style='justify-content:");
    *_GPP += FPSTR(GPgetAlignFlex(al));
    if (top) *_GPP += F(";align-items: flex-start");
    if (w.length()) {
      *_GPP += F(";max-width:");
      *_GPP += w;
    }
    *_GPP += F("' class='inliner'>\n");
    send();
  }
  void VOID_BOX_BEGIN(void) {
    SEND(F("<div>\n"));
  }
  void VOID_BOX(const String& w) {
    if (w.length()) {
      *_GPP += F("<div style='width:");
      *_GPP += w;
      *_GPP += F("'></div>\n");
      send();
    }
  }
  void BOX_END(void) {
    SEND(F("</div>\n"));
  }

  void FOOTER_BEGIN(void) {
    SEND("<div class='blockSpace'></div>\n<footer>");
  }
  void FOOTER_END(void) {
    SEND("</footer>");
  }

  void BREAK(const String& h = "") {
    *_GPP += F("<br");
    if (h.length()) {
      *_GPP += F(" style='line-height:");
      *_GPP += h;
      *_GPP += '\'';
    }
    *_GPP += F(">\n");
    send();
  }

  void VR(PGM_P st = GP_DEFAULT, int height = 0) {
    *_GPP += F("<div class='vr' style='");
    if (st != GP_DEFAULT) {
      *_GPP += F("border-color:");
      *_GPP += FPSTR(st);
      *_GPP += ';';
    }
    if (height) {
      *_GPP += F("height:");
      *_GPP += height;
      *_GPP += F("px");
    }
    *_GPP += F("'></div>\n");
    send();
  }

  void HR(void) {
    SEND(F("<hr>\n"));
  }
  void HR(PGM_P st) {
    *_GPP += F("<hr style='border-color:");
    *_GPP += FPSTR(st);
    *_GPP += F("'>\n");
    send();
  }
  void HR(PGM_P st, int width) {
    *_GPP += F("<hr style='border-color:");
    *_GPP += FPSTR(st);
    *_GPP += F(";margin:5px ");
    *_GPP += width;
    *_GPP += F("px'>\n");
    send();
  }
  void HR_TEXT(const String& text, PGM_P st_0, PGM_P st_1, const String& name = "", GPalign al = GP_RIGHT) {
    *_GPP += F("<div style='justify-content:");
    *_GPP += FPSTR(GPgetAlignFlex(al));
    *_GPP += F(";height:35px' class='inliner'><div style='width:5%'></div><label id='");
    *_GPP += name;
    *_GPP += F("' class='thinText' style='color:");
    *_GPP += FPSTR(st_0);
    *_GPP += F("'>");
    *_GPP += text;
    *_GPP += F("</label><div style='width:5%'></div></div>\n");
    *_GPP += F("<hr style='border-color:");
    *_GPP += FPSTR(st_1);
    *_GPP += F(";margin-top:-18px;padding-bottom:12px'>\n");
    send();
  }

  // ======================= ТЕКСТ =======================
  void TAG_RAW(const String& tag, const String& val, const String& name = "", PGM_P st = GP_DEFAULT, int size = 0, bool bold = 0, bool wrap = 0, PGM_P back = GP_DEFAULT, int width = 0, int align = GP_CENTER) {
    *_GPP += F("<");
    *_GPP += tag;
    if (back != GP_DEFAULT) *_GPP += F(" class='display'");
    if (name.length()) {
      *_GPP += F(" id='");
      *_GPP += name;
      *_GPP += "'";
    }
    *_GPP += F(" style='");
    if (st != GP_DEFAULT) {
      *_GPP += F("color:");
      *_GPP += FPSTR(st);
      *_GPP += ';';
    }
    if (back != GP_DEFAULT) {
      *_GPP += F("background-color:");
      *_GPP += FPSTR(back);
      *_GPP += ';';
    }
    if (align != GP_CENTER) {
      *_GPP += F("text-align:");
      *_GPP += (align == GP_RIGHT) ? F("right") : F("left");
      *_GPP += ';';
    }
    if (size) {
      *_GPP += F("font-size:");
      *_GPP += size;
      *_GPP += F("px;");
    }
    if (bold) *_GPP += F("font-weight:bold;");
    if (wrap) *_GPP += F("white-space:normal;");
    if (width) {
      *_GPP += F("width:");
      *_GPP += width;
      *_GPP += F("px;");
    }
    *_GPP += F("'>");
    *_GPP += val;
    *_GPP += F("</");
    *_GPP += tag;
    *_GPP += F(">\n");
    send();
  }

  void LABEL(const String& val, const String& name = "", PGM_P st = GP_DEFAULT, int size = 0, bool bold = 0, bool wrap = 0) {
    TAG_RAW(F("label"), val, name, st, size, bold, wrap);
  }
  void LABEL_W(const String& val, const String& name = "", PGM_P st = GP_DEFAULT, int width = 0, int align = GP_CENTER, int size = 0, bool bold = 0, bool wrap = 0) {
    TAG_RAW(F("label"), val, name, st, size, bold, wrap, GP_DEFAULT, width, align);
  }
  void LABEL_M(const String& val, int mr, int w) {
    *_GPP += F("<label style='margin:");
    *_GPP += mr;
    *_GPP += F("px;width:");
    *_GPP += w;
    *_GPP += F("px'>");
    *_GPP += val;
    *_GPP += F("</label>");
    send();
  }

  void LABEL_BLOCK(const String& val, const String& name = "", PGM_P st = GP_DEFAULT, int size = 0, bool bold = 0) {
    TAG_RAW(F("label"), val, name, GP_DEFAULT, size, bold, 0, st);
  }
  void LABEL_BLOCK_W(const String& val, const String& name = "", PGM_P st = GP_DEFAULT, int size = 0, bool bold = 0) {
    TAG_RAW(F("label"), val, name, GP_WHITE, size, bold, 0, st);
  }

  void TITLE(const String& val, const String& name = "", PGM_P st = GP_DEFAULT, int size = 0, bool bold = 0) {
    TAG_RAW(F("h2"), val, name, st, size, bold);
  }
  void PLAIN(const String& text, const String& name = "", PGM_P st = GP_DEFAULT) {
    TAG_RAW(F("p"), text, name, st);
  }
  void BOLD(const String& text, const String& name = "", PGM_P st = GP_DEFAULT) {
    TAG_RAW(F("strong"), text, name, st);
  }

  void SPAN(const String& text, GPalign al = GP_CENTER, const String& name = "", PGM_P st = GP_DEFAULT, int size = 0, bool bold = 0) {
    *_GPP += F("<div style='text-align:");
    *_GPP += FPSTR(GPgetAlign(al));
    *_GPP += F("'>");
    TAG_RAW(F("span"), text, name, st, size, bold);
    *_GPP += F("</div>\n");
    send();
  }

  // ======================= ЛЕДЫ =======================
  void LED(const String& name, bool state = 0) {
    *_GPP += F("<input class='ledn' type='radio' disabled ");
    if (state) *_GPP += F("checked ");
    *_GPP += F("name='");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += F("'>\n");
    send();
  }
  void LED(const String& name, bool state, PGM_P st) {
    *_GPP += F("<style>.led_");
    *_GPP += name;
    *_GPP += F(":checked:after{background-color:");
    *_GPP += FPSTR(st);
    *_GPP += F(";box-shadow:0 0 10px 2px ");
    *_GPP += FPSTR(st);
    *_GPP += F(";}</style>\n");

    *_GPP += F("<input class='led led_");
    *_GPP += name;
    *_GPP += F("' type='radio' disabled ");
    if (state) *_GPP += F("checked ");
    *_GPP += F("name='");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += F("'>\n");
    send();
  }

  void LED_RED(const String& name, bool state = 0) {
    *_GPP += F("<input class='led red' type='radio' disabled ");
    if (state) *_GPP += F("checked ");
    *_GPP += F("name='");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += F("'>\n");
    send();
  }
  void LED_GREEN(const String& name, bool state = 0) {
    *_GPP += F("<input class='led green' type='radio' disabled ");
    if (state) *_GPP += F("checked ");
    *_GPP += F("name='");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += F("'>\n");
    send();
  }

  void LED_COLOR(const String& name, PGM_P st = GP_DEFAULT) {
    *_GPP += F("<div id='");
    *_GPP += name;
    *_GPP += F("' class='ledc");
    if (st != GP_DEFAULT) {
      *_GPP += F("' style='box-shadow:0 0 10px 2px '");
      *_GPP += FPSTR(st);
      *_GPP += F("background-color:");
      *_GPP += FPSTR(st);
    }
    *_GPP += F("'></div>\n");
    send();
  }

  // ======================= ИНДИКАТОРЫ =======================

  void LINE_BAR(const String& name, int value = 0, int min = 0, int max = 100, PGM_P st = GP_GREEN) {
    *_GPP += F("<div class='lineBar");
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += F("_dsp' style='background-image:linear-gradient(");
    *_GPP += FPSTR(st);
    *_GPP += ',';
    *_GPP += FPSTR(st);
    *_GPP += F(");background-size:");
    *_GPP += map(value, min, max, 0, 100);
    *_GPP += F("% 100%'></div>\n");

    *_GPP += F("<input type='hidden' name='_line' id='");
    *_GPP += name;
    *_GPP += F("' value='");
    *_GPP += value;
    *_GPP += F("' min='");
    *_GPP += min;
    *_GPP += F("' max='");
    *_GPP += max;
    *_GPP += F("'>\n");
    send();
  }

  void LINE_LED(const String& name, bool state = 0, PGM_P st_0 = GP_RED, PGM_P st_1 = GP_GREEN) {
    *_GPP += F("<style>#__");
    *_GPP += name;
    *_GPP += F(" input:checked+span::before{background-color:");
    *_GPP += FPSTR(st_1);
    *_GPP += F("}\n#__");
    *_GPP += name;
    *_GPP += F(" span::before{background-color:");
    *_GPP += FPSTR(st_0);
    *_GPP += F("}\n</style>\n<label id='__");
    *_GPP += name;
    *_GPP += F("' class='check_c lineled'><input type='checkbox' name='");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += '\'';
    if (state) *_GPP += F(" checked");
    *_GPP += F(" disabled><span></span></label>\n");
    send();
  }

  // ======================= ИКОНКИ =======================
  void ICON_SUPPORT(void) {
    SEND(F("<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css'>\n"));
  }

  String ICON(const String& faname, int size = 20, PGM_P st = GP_DEFAULT) {
    String s(F("<i class='fa fa-"));
    s += faname;
    s += F("' style='");
    if (size) {
      s += F("font-size:");
      s += size;
      s += F("px;");
    }
    if (st != GP_DEFAULT) {
      s += F("color:");
      s += FPSTR(st);
      s += ";";
    }
    s += F("'></i>");
    return s;
  }
  String ICON_FILE(const String& uri, int size = 20, PGM_P st = GP_DEFAULT) {
    String s(F("<i class='i_mask' style='-webkit-mask:center/contain no-repeat url("));
    s += uri;
    s += F(");");
    if (st != GP_DEFAULT) {
      s += F("background-color:");
      s += FPSTR(st);
      s += ";";
    }
    if (size) {
      s += F("width:");
      s += size;
      s += F("px;height:");
      s += size;
      s += F("px;");
    }
    s += F("'></i>");
    return s;
  }

  void ICON_BUTTON_RAW(const String& name, const String& icon) {
    *_GPP += F("<div class='i_btn' id='");
    *_GPP += name;
    *_GPP += F("' name='");
    *_GPP += name;
#ifndef GP_NO_PRESS
    *_GPP += F("' onmousedown='if(!_touch)EVpress(this,1)' onmouseup='if(!_touch&&_pressId)EVpress(this,2)' onmouseleave='if(_pressId&&!_touch)EVpress(this,2);' "
               "ontouchstart='_touch=1;EVpress(this,1)' ontouchend='EVpress(this,2)' onclick='EVclick(this)'>");
#else
    *_GPP += F("' onclick='EVclick(this)'>");
#endif
    *_GPP += icon;
    *_GPP += F("</div>");
  }

  void ICON_BUTTON(const String& name, const String& faname, int size = 0, PGM_P st = GP_DEFAULT) {
    ICON_BUTTON_RAW(name, ICON(faname, size, st));
  }
  void ICON_FILE_BUTTON(const String& name, const String& uri, int size = 0, PGM_P st = GP_DEFAULT) {
    ICON_BUTTON_RAW(name, ICON_FILE(uri, size, st));
  }

  void ICON_CHECK(const String& name, const String& uri, bool state = 0, int size = 30, PGM_P st_0 = GP_GRAY, PGM_P st_1 = GP_GREEN, bool dis = false) {
    *_GPP += F("<style>#__");
    *_GPP += name;
    *_GPP += F(" span::before{background-color:");
    *_GPP += FPSTR(st_0);
    *_GPP += F(";border:none;padding:");
    *_GPP += (size / 2) + 2;
    *_GPP += F("px;}</style>\n");

    *_GPP += F("<style>#__");
    *_GPP += name;
    *_GPP += F(" input:checked+span::before{background-color:");
    *_GPP += FPSTR(st_1);
    *_GPP += F(";background-image:none;}</style>\n");

    *_GPP += F("<label id='__");
    *_GPP += name;
    *_GPP += F("' class='check_c'");
    *_GPP += F(" style='-webkit-mask:center/contain no-repeat url(");
    *_GPP += uri;
    *_GPP += F(");display:inline-block;width:");
    *_GPP += size;
    *_GPP += F("px;'>");
    *_GPP += F("<input type='checkbox' name='");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += "' ";
    if (state) *_GPP += F("checked ");
    if (dis) *_GPP += F("disabled ");
    *_GPP += F("onclick='EVclick(this)' style='height:");
    *_GPP += size;
    *_GPP += F("px;'><span></span></label>\n");
    send();
  }

  // ======================= НАВИГАЦИЯ =======================
  void NAV_BAR_LINKS(const String& urls, const String& names, int width = 0, PGM_P st = GP_GREEN) {
    *_GPP += F("<style>@media screen and (max-width:1000px){.offlAnim{bottom:68px;}}.mainblock{padding-bottom:95px;}</style>\n");
    *_GPP += F("<div class='navfixed'><div class='navtab navbar' ");
    if (width) {
      *_GPP += F("style='max-width:");
      *_GPP += width;
      *_GPP += F("px'");
    }
    *_GPP += F("><ul id='navbar' onWheel='barScroll(this,event)'>\n");
    GP_parser n(names);
    GP_parser u(urls);
    while (n.parse()) {
      u.parse();
      *_GPP += F("<li ");
      if (_gp_uri->equals(u.str)) {
        if (st != GP_DEFAULT) {
          *_GPP += F("style='background:");
          *_GPP += FPSTR(st);
          *_GPP += F(";color:#13161a' ");
        }
        else *_GPP += F("class='navopen' ");
      }
      *_GPP += F("onclick='saveNav();location.href=\"");
      *_GPP += u.str;
      *_GPP += F("\";'>");
      *_GPP += n.str;
      *_GPP += F("</li>\n");
    }
    *_GPP += F("</ul></div></div>\n");

    *_GPP += F("<script>restoreNav();</script>\n");
    send();
  }

  void NAV_TABS_LINKS(const String& urls, const String& names, PGM_P st = GP_DEFAULT) {
    *_GPP += F("<div class='navtab'");
    if (st != GP_DEFAULT) {
      *_GPP += F(" style='background:");
      *_GPP += FPSTR(st);
      *_GPP += "'";
    }
    *_GPP += "><ul>\n";
    GP_parser n(names);
    GP_parser u(urls);
    while (n.parse()) {
      u.parse();
      *_GPP += F("<li ");
      if (_gp_uri->equals(u.str)) *_GPP += F("class='navopen' ");
      *_GPP += F("onclick='location.href=\"");
      *_GPP += u.str;
      *_GPP += F("\";'>");
      *_GPP += n.str;
      *_GPP += F("</li>\n");
    }
    *_GPP += F("</ul></div>\n");
    send();
  }

  void NAV_TABS_M(const String& name, const String& list, int disp, PGM_P st = GP_DEFAULT) {
    if (st != GP_DEFAULT) {
      *_GPP += F("<style>.navopen.navopen{background:");
      *_GPP += st;
      *_GPP += F(";color:#13161a;}</style>\n");
    }
    *_GPP += F("<div class='navtab'><ul>\n");
    GP_parser tab(list);
    while (tab.parse()) {
      *_GPP += F("<li ");
      *_GPP += F("class='");
      *_GPP += name;
      if (tab.count == disp) *_GPP += F(" navopen");
      *_GPP += F("' onclick='EVopenTab(\"");
      *_GPP += name;
      *_GPP += '/';
      *_GPP += tab.count;
      *_GPP += F("\",this,\"block_");
      *_GPP += name;
      *_GPP += F("\");EVsend(\"/EV_click?");
      *_GPP += name;
      *_GPP += '/';
      *_GPP += tab.count;
      *_GPP += F("=\");'>");
      *_GPP += tab.str;
      *_GPP += F("</li>\n");
    }
    *_GPP += F("</ul></div>\n");
    send();
  }

  void NAV_TABS_A(const String& name, const String& list, PGM_P st = GP_DEFAULT) {
    _gp_nav_pos = 0;
    *_GPP += F("<div class='navtab'><ul ");
    if (st != GP_DEFAULT) {
      *_GPP += F("style='background:");
      *_GPP += FPSTR(st);
      *_GPP += '\'';
    }
    *_GPP += ">\n";
    GP_parser p(list);
    while (p.parse()) {
      *_GPP += F("<li ");
      *_GPP += F("class='");
      *_GPP += name;
      if (!p.count) *_GPP += F(" navopen");
      *_GPP += F("' onclick='EVopenTab(\"");
      *_GPP += name;
      *_GPP += '/';
      *_GPP += p.count;
      *_GPP += F("\",this,\"block_");
      *_GPP += name;
      *_GPP += F("\");EVsend(\"/EV_click?");
      *_GPP += name;
      *_GPP += '/';
      *_GPP += p.count;
      *_GPP += F("=\");'>");
      *_GPP += p.str;
      *_GPP += F("</li>\n");
    }
    *_GPP += F("</ul></div>\n");
    send();
  }

  void NAV_BLOCK_BEGIN(const String& name, int pos, int disp) {
    *_GPP += F("<div class='navblock block_");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += '/';
    *_GPP += pos;
    *_GPP += "' ";
    if (pos == disp) *_GPP += F("style='display:block'");
    *_GPP += F(">\n");
    send();
  }

  void NAV_BLOCK_BEGIN(const String& name, int pos) {
    *_GPP += F("<div class='navblock block_");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += '/';
    *_GPP += pos;
    *_GPP += "' ";
    if (!pos) *_GPP += F("style='display:block'");
    *_GPP += ">\n";
    send();
  }

  void NAV_TABS(const String& list, PGM_P st = GP_DEFAULT) {
    _gp_nav_id++;
    _gp_nav_pos = 0;
    *_GPP += F("<div class='navtab'><ul ");
    if (st != GP_DEFAULT) {
      *_GPP += F("style='background:");
      *_GPP += FPSTR(st);
      *_GPP += '\'';
    }
    *_GPP += ">\n";
    GP_parser p(list);
    while (p.parse()) {
      *_GPP += F("<li ");
      *_GPP += F("class='nt-");
      *_GPP += _gp_nav_id;
      if (!p.count) *_GPP += F(" navopen");
      *_GPP += F("' onclick='EVopenTab(\"ntab-");
      *_GPP += _gp_nav_id;
      *_GPP += '/';
      *_GPP += p.count;
      *_GPP += F("\",this,\"nb-");
      *_GPP += _gp_nav_id;
      *_GPP += F("\")'>");
      *_GPP += p.str;
      *_GPP += F("</li>\n");
    }
    *_GPP += F("</ul></div>\n");
    send();
  }

  void NAV_BLOCK_BEGIN(void) {
    *_GPP += F("<div class='navblock nb-");
    *_GPP += _gp_nav_id;
    *_GPP += F("' id='ntab-");
    *_GPP += _gp_nav_id;
    *_GPP += '/';
    *_GPP += _gp_nav_pos;
    *_GPP += "' ";
    if (!_gp_nav_pos) *_GPP += F("style='display:block'");
    *_GPP += ">\n";
    send();
    _gp_nav_pos++;
  }

  void NAV_BLOCK_END(void) {
    SEND(F("</div>\n"));
  }

  // ======================= ФАЙЛЫ =======================
  void FILE_UPLOAD_RAW(const String& name, const String& text = "", PGM_P st = GP_GREEN, const String& accept = "", const String& options = "", const String& action = "/EV_upload") {
    *_GPP += F("<div id='");
    *_GPP += name;
    *_GPP += F("'><form action='");
    *_GPP += action;
    *_GPP += F("' method='POST' enctype='multipart/form-data' id='");
    *_GPP += name;
    *_GPP += F("_form' style='display:flex;justify-content:center;'>\n"
               "<div id='ubtn' onclick='EVsaveFile(\"");
    *_GPP += name;
    *_GPP += F("_inp\")'");
    if (st != GP_GREEN) {
      *_GPP += F(" style='background:");
      *_GPP += FPSTR(st);
      *_GPP += "'";
    }
    *_GPP += ">";
    *_GPP += text;
    *_GPP += F("</div>\n"
               "<div id='ubtnclr'><input ");
    *_GPP += options;
    *_GPP += F("name='");
    *_GPP += name;
    if (accept.length()) {
      *_GPP += F("' accept='");
      *_GPP += accept;
    }
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += F("_inp' type='file' onchange=\"popupOpen(getPop('uploadAnim'));setTimeout(popupClose,60000);EVsubmId('");
    *_GPP += name;
    *_GPP += F("_form');\"/></div>\n"
               "</form></div>\n");
    send();
  }

  void FILE_UPLOAD(const String& name, const String& text = "", const String& accept = "", PGM_P st = GP_GREEN) {
    FILE_UPLOAD_RAW(name, (!text.length()) ? "📄" : text, st, accept, F("multiple "));
  }

  void FOLDER_UPLOAD(const String& name, const String& text = "", PGM_P st = GP_GREEN) {
    FILE_UPLOAD_RAW(name, (!text.length()) ? "📁" : text, st, "", F("multiple webkitdirectory allowdirs "));
  }

  void OTA_FIRMWARE(const String& text = "OTA firmware", PGM_P st = GP_GREEN, bool page = 0) {
    FILE_UPLOAD_RAW(F("firmware"), (!text.length()) ? "🔧" : text, st, F(".bin,.bin.gz"), "", page ? F("/ota_update") : F("/EV_OTAupload"));
  }

  void OTA_FILESYSTEM(const String& text = "OTA filesystem", PGM_P st = GP_GREEN, bool page = 0) {
    FILE_UPLOAD_RAW(F("filesystem"), (!text.length()) ? "💾" : text, st, F(".bin,.bin.gz"), "", page ? F("/ota_update") : F("/EV_OTAupload"));
  }

  void IMAGE(const String& uri, const String& w = "") {
    *_GPP += F("<img src='");
    *_GPP += uri;
    *_GPP += F("' style='width:");
    *_GPP += w;
    *_GPP += F("'>\n");
    send();
  }
  void VIDEO(const String& uri, const String& w = "") {
    *_GPP += F("<video src='");
    *_GPP += uri;
    *_GPP += F("' style='width:");
    *_GPP += w;
    *_GPP += F("' controls>Browser doesn't support video.</video>\n");
    send();
  }
  void EMBED(const String& uri, const String& w = "100%", const String& h = "") {
    *_GPP += F("<embed src='");
    *_GPP += uri;
    *_GPP += F("' style='width:");
    *_GPP += w;
    if (h.length()) {
      *_GPP += F(";height:");
      *_GPP += h;
    }
    *_GPP += F(";'>\n");
    send();
  }

  // ======================= ФАЙЛОВЫЙ МЕНЕДЖЕР =======================
  void _fileRow(const String& fpath, int size) {
    *_GPP += "<tr>";
    *_GPP += F("<td align='left' style='padding-right:5px'>\n"
               "<a style='text-decoration:none' href='");
    *_GPP += fpath;
    *_GPP += F("'>");
    *_GPP += fpath;
    *_GPP += F("</a>\n<td align='right'>[");
    *_GPP += String(size / 1000.0, 1);
    *_GPP += F(" kB]\n"
               "<td align='center'>\n"
               "<span title='Rename' style='cursor:pointer' onclick='EVrename(\"");
    *_GPP += fpath;
    *_GPP += F("\")'>📝</span>\n"
               "<a style='text-decoration:none' href='");
    *_GPP += fpath;
    *_GPP += F("' download><span title='Download'>📥</span></a>\n"
               "<span title='Delete' style='cursor:pointer' onclick='EVdelete(\"");
    *_GPP += fpath;
    *_GPP += F("\")'>❌</span>\n");
  }

  void _showFiles(fs::FS *fs, const String& path, const String& odir, __attribute__((unused)) uint8_t levels = 0) {
#ifdef ESP8266
    yield();
    Dir dir = fs->openDir(path);
    while (dir.next()) {
      if (dir.isFile() && dir.fileName().length()) {
        String fpath = '/' + path + dir.fileName();
        if (odir.length() && !fpath.startsWith(odir)) continue;
        _fileRow(fpath, dir.fileSize());
      }
      if (dir.isDirectory()) {
        String p = path;
        p += dir.fileName();
        p += '/';
        Dir sdir = fs->openDir(p);
        _showFiles(fs, p, odir);
      }
    }

#else   // ESP32
    File root = fs->open(path.length() ? path.c_str() : ("/"));
    if (!root || !root.isDirectory()) return;
    File file;
    while (file = root.openNextFile()) {
      if (file.isDirectory()) {
        if (levels) _showFiles(fs, file.path(), odir, levels - 1);
      } else {
        String fpath = path + '/' + file.name();
        if (odir.length() && !fpath.startsWith(odir)) continue;
        _fileRow(fpath, file.size());
      }
    }
#endif
  }

  void FILE_MANAGER(fs::FS *fs, const String& odir = "") {
    *_GPP += F("<table>");
    _showFiles(fs, "", odir, 5);

#ifdef ESP8266
    FSInfo fsi;
    fs->info(fsi);
    *_GPP += F("<tr><td colspan=3 align=center><hr><strong>Used ");
    *_GPP += String(fsi.usedBytes / 1000.0, 2);
    *_GPP += F(" kB from ");
    *_GPP += String(fsi.totalBytes / 1000.0, 2);
    *_GPP += F(" (");
    *_GPP += (fsi.usedBytes * 100 / fsi.totalBytes);
    *_GPP += F("%)</strong>");
#endif
    *_GPP += F("</table>\n");
    send();
  }

  // ================ СИСТЕМНАЯ ИНФОРМАЦИЯ ================
  void SYSTEM_INFO(const String& fwv = "", const String& w = "") {
    TABLE_BEGIN(w);
    // ===========
    TR();
    TD(GP_CENTER, 3);
    LABEL(F("Network"));
    HR();

    TR();
    TD(GP_LEFT); BOLD(F("WiFi Mode"));
    TD(GP_RIGHT); SEND(WiFi.getMode() == WIFI_AP ? F("AP") : (WiFi.getMode() == WIFI_STA ? F("STA") : F("AP_STA")));

    if (WiFi.getMode() != WIFI_AP) {
      TR();
      TD(GP_LEFT); BOLD(F("SSID"));
      TD(GP_RIGHT); SEND(WiFi.SSID());

      TR();
      TD(GP_LEFT); BOLD(F("Local IP"));
      TD(GP_RIGHT); SEND(WiFi.localIP().toString());
    }
    if (WiFi.getMode() != WIFI_STA) {
      TR();
      TD(GP_LEFT); BOLD(F("AP IP"));
      TD(GP_RIGHT); SEND(WiFi.softAPIP().toString());
    }

    if (_gp_mdns && strlen(_gp_mdns)) {
      TR();
      TD(GP_LEFT); BOLD(F("mDNS"));
      TD(GP_RIGHT); SEND(String(_gp_mdns) + ".local");
    }

    TR();
    TD(GP_LEFT); BOLD(F("Subnet"));
    TD(GP_RIGHT); SEND(WiFi.subnetMask().toString());

    TR();
    TD(GP_LEFT); BOLD(F("Gateway"));
    TD(GP_RIGHT); SEND(WiFi.gatewayIP().toString());

    TR();
    TD(GP_LEFT); BOLD(F("MAC Address"));
    TD(GP_RIGHT); SEND(WiFi.macAddress());

    TR();
    TD(GP_LEFT); BOLD(F("RSSI"));
    TD(GP_RIGHT); SEND("📶 " + String(constrain(2 * (WiFi.RSSI() + 100), 0, 100)) + '%');

    // ===========
    TR();
    TD(GP_CENTER, 3);
    LABEL(F("Memory"));
    HR();

    TR();
    TD(GP_LEFT); BOLD(F("Free Heap"));
    TD(GP_RIGHT); SEND(String(ESP.getFreeHeap() / 1000.0, 3) + " kB");

#ifdef ESP8266
    TR();
    TD(GP_LEFT); BOLD(F("Heap Fragm."));
    TD(GP_RIGHT); SEND(String(ESP.getHeapFragmentation()) + '%');
#endif

    TR();
    TD(GP_LEFT); BOLD(F("Sketch Size (Free)"));
    TD(GP_RIGHT); SEND(String(ESP.getSketchSize() / 1000.0, 1) + " kB (" + String(ESP.getFreeSketchSpace() / 1000.0, 1) + ")");

    TR();
    TD(GP_LEFT); BOLD(F("Flash Size"));
    TD(GP_RIGHT); SEND(String(ESP.getFlashChipSize() / 1000.0, 1) + " kB");

    // ===========
    TR();
    TD(GP_CENTER, 3);
    LABEL(F("System"));
    HR();

#ifdef ESP8266
    TR();
    TD(GP_LEFT); BOLD(F("Chip ID"));
    TD(GP_RIGHT); SEND("0x" + String(ESP.getChipId(), HEX));
#endif

    TR();
    TD(GP_LEFT); BOLD(F("Cycle Count"));
    TD(GP_RIGHT); SEND(String(ESP.getCycleCount()));

    TR();
    TD(GP_LEFT); BOLD(F("Cpu Freq."));
    TD(GP_RIGHT); SEND(String(ESP.getCpuFreqMHz()) + F(" MHz"));

    TR();
    TD(GP_LEFT); BOLD(F("Date"));
    GPdate date(_gp_local_unix);
    TD(GP_RIGHT); SEND(_gp_local_unix ? date.encodeDMY() : String("unset"));

    TR();
    TD(GP_LEFT); BOLD(F("Time"));
    GPtime time(_gp_local_unix);
    TD(GP_RIGHT); SEND(_gp_local_unix ? time.encode() : String("unset"));

    TR();
    TD(GP_LEFT); BOLD(F("Uptime"));
    uint32_t sec = millis() / 1000ul;
    uint8_t second = sec % 60ul;
    sec /= 60ul;
    uint8_t minute = sec % 60ul;
    sec /= 60ul;
    uint16_t hour = sec % 24ul;
    sec /= 24ul;
    String s;
    s.reserve(10);
    s += sec;   // day
    s += ':';
    s += hour;
    s += ':';
    s += minute / 10;
    s += minute % 10;
    s += ':';
    s += second / 10;
    s += second % 10;
    TD(GP_RIGHT); SEND(s);

    // ===========
    TR();
    TD(GP_CENTER, 3);
    LABEL(F("Version"));
    HR();

    TR();
    TD(GP_LEFT); BOLD(F("SDK"));
    TD(GP_RIGHT); SEND(ESP.getSdkVersion());

#ifdef ESP8266
    TR();
    TD(GP_LEFT); BOLD(F("Core"));
    TD(GP_RIGHT); SEND(ESP.getCoreVersion());
#endif

    TR();
    TD(GP_LEFT); BOLD(F("GyverPortalMod"));
    TD(GP_RIGHT); SEND(GP_VERSION);

    if (fwv.length()) {
      TR();
      TD(GP_LEFT); BOLD(F("Firmware"));
      TD(GP_RIGHT); SEND(fwv);
    }

    TABLE_END();
  }

  // ======================= КНОПКА =======================
  void BUTTON_RAW(const String& name, const String& value, const String& tar, PGM_P st, const String& width = "", const String& cls = "", int num = -1, bool dis = 0, bool rel = 0) {
    *_GPP += F("<button type='button' ");
    if (cls.length()) {
      *_GPP += F("class='");
      *_GPP += cls;
      *_GPP += "' ";
    }
    *_GPP += F("style='");
    if (st != GP_GREEN) {
      *_GPP += F("background:");
      *_GPP += FPSTR(st);
      *_GPP += ';';
    }
    if (width.length()) {
      *_GPP += F("width:");
      *_GPP += width;
      *_GPP += ';';
    }
    *_GPP += F("' name='");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    if (num >= 0) {
      *_GPP += '_';
      *_GPP += num;
    }
#ifndef GP_NO_PRESS
    *_GPP += F("' onmousedown='if(!_touch)EVpress(this,1)' onmouseup='if(!_touch&&_pressId)EVpress(this,2)' onmouseleave='if(_pressId&&!_touch)EVpress(this,2);' ");
    if (!dis) *_GPP += F("ontouchstart='_touch=1;EVpress(this,1)' ontouchend='EVpress(this,2)");
#endif
    if (tar.length()) {
      *_GPP += F("' onclick=\"EVclickId('");
      *_GPP += name;
      *_GPP += F("','");
      *_GPP += tar;
      *_GPP += F("',");
      *_GPP += rel;
      *_GPP += F(")\"");
    } else {
      *_GPP += F("' onclick='EVclick(this,");
      *_GPP += rel;
      if (num >= 0) {
        *_GPP += ',';
        *_GPP += num;
      }
      *_GPP += F(")'");
    }
    if (dis) *_GPP += F(" disabled");
    *_GPP += ">";
    *_GPP += value;
    *_GPP += F("</button>\n");
    send();
  }

  void BUTTON(const String& name, const String& value, const String& tar = "", PGM_P st = GP_GREEN, const String& width = "", bool dis = 0, bool rel = 0) {
    BUTTON_RAW(name, value, tar, st, width, "", -1, dis, rel);
  }
  void BUTTON_MINI(const String& name, const String& value, const String& tar = "", PGM_P st = GP_GREEN, const String& width = "", bool dis = 0, bool rel = 0) {
    BUTTON_RAW(name, value, tar, st, width, F("miniButton"), -1, dis, rel);
  }
  void BUTTON_MICRO(const String& name, const String& value, const String& tar = "", PGM_P st = GP_GREEN, const String& width = "", bool dis = 0, bool rel = 0) {
    BUTTON_RAW(name, value, tar, st, width, F("microButton"), -1, dis, rel);
  }

  void BUTTON_NUM(const String& name, const String& value, int num, PGM_P st = GP_GREEN, const String& width = "", bool dis = 0, bool rel = 0) {
    BUTTON_RAW(name, value, "", st, width, "", num, dis, rel);
  }
  void BUTTON_NUM_MINI(const String& name, const String& value, int num, PGM_P st = GP_GREEN, const String& width = "", bool dis = 0, bool rel = 0) {
    BUTTON_RAW(name, value, "", st, width, F("miniButton"), num, dis, rel);
  }
  void BUTTON_NUM_MICRO(const String& name, const String& value, int num, PGM_P st = GP_GREEN, const String& width = "", bool dis = 0, bool rel = 0) {
    BUTTON_RAW(name, value, "", st, width, F("microButton"), num, dis, rel);
  }

  // ======================= КНОПКА-ССЫЛКА =======================
  void BUTTON_LINK_RAW(const String& url, const String& value, PGM_P st = GP_GREEN, const String& width = "", const String& cls = "", const String& name = "") {
    *_GPP += F("<input type='button' ");
    if (name.length()) {
      *_GPP += F("name='");
      *_GPP += name;
      *_GPP += F("' id='");
      *_GPP += name;
      *_GPP += "' ";
    }
    if (cls.length()) {
      *_GPP += F("class='");
      *_GPP += cls;
      *_GPP += "' ";
    }
    *_GPP += F("style='");
    if (st != GP_GREEN) {
      *_GPP += F("background:");
      *_GPP += FPSTR(st);
      *_GPP += ';';
    }
    if (width.length()) {
      *_GPP += F("width:");
      *_GPP += width;
      *_GPP += ';';
    }
    *_GPP += F("' value='");
    *_GPP += value;

    if (name.length()) {
      *_GPP += F("' onclick='EVclick(this,\"");
      *_GPP += url;
      *_GPP += F("\");'>\n");
    } else {
      *_GPP += F("' onclick='location.href=\"");
      *_GPP += url;
      *_GPP += F("\";'>\n");
    }
    send();
  }
  void BUTTON_LINK(const String& url, const String& value, PGM_P st = GP_GREEN, const String& width = "", const String& name = "") {
    BUTTON_LINK_RAW(url, value, st, width, "", name);
  }
  void BUTTON_MINI_LINK(const String& url, const String& value, PGM_P st = GP_GREEN, const String& width = "", const String& name = "") {
    BUTTON_LINK_RAW(url, value, st, width, F("miniButton"), name);
  }
  void BUTTON_MICRO_LINK(const String& url, const String& value, PGM_P st = GP_GREEN, const String& width = "", const String& name = "") {
    BUTTON_LINK_RAW(url, value, st, width, F("microButton"), name);
  }

  void TEXT_LINK(const String& url, const String& text, const String& id, PGM_P color) {
    *_GPP += F("<style>a:link.");
    *_GPP += id;
    *_GPP += F("_link{color:");
    *_GPP += FPSTR(color);
    *_GPP += F(";text-decoration:none;} a:visited.");
    *_GPP += id;
    *_GPP += F("_link{color:");
    *_GPP += FPSTR(color);
    *_GPP += F(";} a:hover.");
    *_GPP += id;
    *_GPP += F("_link{filter:brightness(0.75);}</style>\n<a href='");
    *_GPP += url;
    *_GPP += F("' class='");
    *_GPP += id + "_link";
    *_GPP += F("'>");
    *_GPP += text;
    *_GPP += F("</a>\n");
    send();
  }

  // ==================== КНОПКА-СКАЧКА ====================
  void BUTTON_DOWNLOAD_RAW(const String& url, const String& value, PGM_P st = GP_GREEN, const String& width = "", const String& cls = "") {
    *_GPP += F("<a style='text-decoration:none;' href='");
    *_GPP += url;
    *_GPP += F("' download><input type='button' value='");
    *_GPP += value;
    *_GPP += "' ";
    if (cls.length()) {
      *_GPP += F("class='");
      *_GPP += cls;
      *_GPP += "' ";
    }
    *_GPP += F("style='");
    if (st != GP_GREEN) {
      *_GPP += F("background:");
      *_GPP += FPSTR(st);
      *_GPP += ';';
    }
    if (width.length()) {
      *_GPP += F("width:");
      *_GPP += width;
      *_GPP += ';';
    }
    *_GPP += F("'>");
    *_GPP += F("</a>\n");
    send();
  }
  void BUTTON_DOWNLOAD(const String& url, const String& value, PGM_P st = GP_GREEN, const String& width = "") {
    BUTTON_DOWNLOAD_RAW(url, value, st, width);
  }
  void BUTTON_MINI_DOWNLOAD(const String& url, const String& value, PGM_P st = GP_GREEN, const String& width = "") {
    BUTTON_DOWNLOAD_RAW(url, value, st, width, F("miniButton"));
  }
  void BUTTON_MICRO_DOWNLOAD(const String& url, const String& value, PGM_P st = GP_GREEN, const String& width = "") {
    BUTTON_DOWNLOAD_RAW(url, value, st, width, F("microButton"));
  }

  // ========================= ВВОД ========================
  void NUMBER_RAW(const String& name, const String& place = "", const String& value = "", const String& minv = "", const String& maxv = "", const String& width = "", const String& pattern = "", bool dis = 0) {
    *_GPP += F("<input type='number' step='any' name='");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    if (value.length()) {
      *_GPP += F("' value='");
      *_GPP += value;
    }
    if (width.length()) {
      *_GPP += F("' style='width:");
      *_GPP += width;
    }
    if (minv.length()) {
      *_GPP += F("' min='");
      *_GPP += minv;
    }
    if (maxv.length()) {
      *_GPP += F("' max='");
      *_GPP += maxv;
    }
    if (pattern.length()) {
      *_GPP += F("' pattern=");
      *_GPP += pattern;
    }
    *_GPP += F("' placeholder='");
    *_GPP += place;
    *_GPP += F("' onchange='EVclick(this)'");
    if (dis) *_GPP += F(" disabled");
    *_GPP += ">\n";
    send();
  }
  void NUMBER_C(const String& name, const String& place = "", int8_t value = -1, uint8_t min = 0, uint8_t max = 99, const String& width = "", const String& next = "", bool dis = 0) {
    *_GPP += F("<input type='number' step='any' name='");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    if (width.length()) {
      *_GPP += F("' style='min-width:56px;max-width:");
      *_GPP += width;
    }
    *_GPP += F("' placeholder='");
    if (value >= 0) {
      if (value < 10) *_GPP += '0';
      *_GPP += value;
    }
    else *_GPP += place;
    *_GPP += F("' onchange='numNext(this,\"");
    *_GPP += next;
    *_GPP += F("\",1)' oninput='numConst(this,");
    *_GPP += min;
    *_GPP += ',';
    *_GPP += (max <= 99) ? max : 99;
    *_GPP += F(");numNext(this,\"");
    *_GPP += next;
    *_GPP += F("\",0)'");
    if (dis) *_GPP += F(" disabled");
    *_GPP += ">\n";
    send();
  }
  void NUMBER(const String& name, const String& place = "", int value = INT32_MAX, const String& width = "", bool dis = false) {
    NUMBER_RAW(name, place, (value == INT32_MAX ? String("") : String(value)), "", "", width, "", dis);
  }
  void NUMBER_F(const String& name, const String& place = "", float value = NAN, uint8_t dec = 2, const String& width = "", bool dis = false) {
    NUMBER_RAW(name, place, (isnan(value) ? String("") : String(value, (uint16_t)dec)), "", "", width, "", dis);
  }

  void TEXT(const String& name, const String& place = "", const String& value = "", const String& width = "", int maxlength = 0, const String& pattern = "", bool dis = false, bool en = false) {
    *_GPP += F("<input type='text' name='");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += F("' value='");
    *_GPP += value;
    if (width.length()) {
      *_GPP += F("' style='width:");
      *_GPP += width;
    }
    *_GPP += F("' placeholder='");
    *_GPP += place;
    *_GPP += F("' onchange='EVclick(this)'");
    if (en) *_GPP += F(" oninput='textEn(this)'");
    if (dis) *_GPP += F(" disabled");
    if (maxlength) {
      *_GPP += F(" maxlength=");
      *_GPP += maxlength;
    }
    if (pattern.length()) {
      *_GPP += F(" pattern=");
      *_GPP += pattern;
    }
    *_GPP += ">\n";
    send();
  }
  void TEXT_EN(const String& name, const String& place = "", const String& value = "", const String& width = "", int maxlength = 0, const String& pattern = "", bool dis = false) {
    TEXT(name, place, value, width, maxlength, pattern, dis, true);
  }

  void PASS(const String& name, const String& place = "", const String& value = "", int maxlength = 0, const String& pattern = "", bool dis = false, bool eye = 0) {
    *_GPP += F("<div class='inlpass'><input type='password' class='pass' name='");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += F("' value='");
    *_GPP += value;
    *_GPP += F("' placeholder='");
    *_GPP += place;
    *_GPP += '\'';
    if (dis) *_GPP += F(" disabled");
    if (maxlength) {
      *_GPP += F(" maxlength=");
      *_GPP += maxlength;
    }
    if (pattern.length()) {
      *_GPP += F(" pattern=");
      *_GPP += pattern;
    }
    *_GPP += ">\n";
    if (eye) {
      *_GPP += F("<span class='eyepass' style='margin-top:13px;' onclick='EVeye(this)'>"
                 "<svg viewBox='0 0 20 20' style='width:25px;height:25px;fill:currentcolor;'>"
                 "<path fill-rule='evenodd' d='M0 20V0zM20 0v20zm-7.204 6.143C11.889 5.734 10.953 5.5 10 5.5c-2.632 0-5.165 1.59-7.204 4.5.924 1.319 1.953 2.35 3.044 "
                 "3.1l1.583-1.584A2.993 2.993 0 0 1 10 7c.526 0 1.048.148 1.516.423zm4.381-2.259-4.6 4.6A2.993 2.993 0 0 1 10 13a3 3 0 0 1-1.516-.423l-4.6 4.6a.5.5 0 0 "
                 "1-.707 0l-.354-.354a.5.5 0 0 1 0-.707l1.943-1.942c-1.221-.882-2.373-2.082-3.405-3.616a1.01 1.01 0 0 1-.001-1.116C3.796 5.814 6.898 4 10 4c1.327 0 "
                 "2.651.346 3.924 1.016l2.192-2.193a.5.5 0 0 1 .707 0l.354.354a.5.5 0 0 1 0 .707m-6.116 5.055A1.5 1.5 0 0 0 10 8.5a1.502 1.502 0 0 0-1.061 2.561zM10 16a8.2 8.2 0 0 "
                 "1-2.44-.378l1.244-1.245c.396.074.794.123 1.196.123 2.632 0 5.165-1.59 7.204-4.5a13.6 13.6 0 0 0-1.867-2.155l1.053-1.053a15.5 15.5 0 0 1 2.251 2.653c.223.331.222.78 0 "
                 "1.111C16.205 14.185 13.103 16 10 16'></path></svg></span>");
    }
    *_GPP += F("</div>\n");
    send();
  }
  void PASS_EYE(const String& name, const String& place = "", const String& value = "", int maxlength = 0, const String& pattern = "", bool dis = false) {
    PASS(name, place, value, maxlength, pattern, dis, true);
  }

  // ======================= НАСТРОЙКА =======================
  void CHECK(const String& name, bool state = 0, PGM_P st = GP_DEFAULT, bool dis = false) {
    if (st != GP_DEFAULT) {
      *_GPP += F("<style>#__");
      *_GPP += name;
      *_GPP += F(" input:checked+span::before{border-color:");
      *_GPP += FPSTR(st);
      *_GPP += F(";background-color:");
      *_GPP += FPSTR(st);
      *_GPP += F("}</style>\n");
    }
    *_GPP += F("<label id='__");
    *_GPP += name;
    *_GPP += F("' class='check_c'><input type='checkbox' name='");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += "' ";
    if (state) *_GPP += F("checked ");
    if (dis) *_GPP += F("disabled ");
    *_GPP += F("onclick='EVclick(this)'><span></span></label>\n");
    send();
  }
  void SWITCH(const String& name, bool state = 0, PGM_P st = GP_DEFAULT, bool dis = false, const String& sw_upd = "", bool sw_val = false) {
    if (st != GP_DEFAULT) {
      *_GPP += F("<style>#__");
      *_GPP += name;
      *_GPP += F(" input:checked+.slider{background-color:");
      *_GPP += FPSTR(st);
      *_GPP += F("}</style>\n");
    }
    *_GPP += F("<label id='__");
    *_GPP += name;
    *_GPP += F("' class='switch'><input class='_sw_c' type='checkbox' name='");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += "' ";
    if (state) *_GPP += F("checked ");
    if (dis) *_GPP += F("disabled ");
    *_GPP += F("onclick='EVclick(this)");
    if (sw_upd.length()) {
      *_GPP += F(";swUpd(\"");
      *_GPP += sw_upd;
      *_GPP += F("\",");
      *_GPP += sw_val;
      *_GPP += ')';
    }
    *_GPP += F("'>\n<span class='slider");
    if (dis) *_GPP += F(" dsbl");
    *_GPP += F("' id='_");
    *_GPP += name;
    *_GPP += F("'></span></label>\n");
    send();
  }

  void DATE(const String& name, bool dis = false) {
    GPdate d;
    DATE(name, d, dis);
  }
  void DATE(const String& name, GPdate d, bool dis = false) {
    *_GPP += F("<input step='any' type='date' name='");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    if (d.year) {
      *_GPP += F("' value='");
      *_GPP += d.encode();
    }
    *_GPP += "' ";
    if (dis) *_GPP += F("disabled ");
    *_GPP += F("onchange='EVclick(this)'>\n");
    send();
  }

  void TIME(const String& name, bool dis = false) {
    GPtime t;
    TIME(name, t, dis);
  }
  void TIME(const String& name, GPtime t, bool dis = false) {
    *_GPP += F("<input step='1' type='time' name='");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += F("' value='");
    *_GPP += t.encode();
    *_GPP += "' ";
    if (dis) *_GPP += F("disabled ");
    *_GPP += F("onchange='EVclick(this)'>\n");
    send();
  }

  void SLIDER(const String& name, const String& min_lable, const String& max_lable, float value = 0, float min = 0, float max = 100, float step = 1, PGM_P st = GP_GREEN, bool dis = 0, bool oninp = 0, bool maxsz = 0, const String& lable = "", const String& color = "") {
    if (maxsz) {
      *_GPP += F("<lable class='rangeLable");
      if (dis) *_GPP += F(" dsbl");
      *_GPP += F("'>");
      *_GPP += lable;
      *_GPP += F("</lable>\n");
    }

    *_GPP += F("<div class='range");
    if (maxsz) *_GPP += F(" rangeLarge");
    if (dis) *_GPP += F(" dsbl");
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += F("_dsp' style='background-image:linear-gradient(");
    *_GPP += FPSTR(st);
    *_GPP += ',';
    *_GPP += FPSTR(st);
    *_GPP += F(");background-size:0% 100%'></div>\n");

    *_GPP += F("<input type='hidden' name='_range' class='range_inp' id='");
    *_GPP += name;
    if (color.length()) {
      *_GPP += F("' placeholder='");
      *_GPP += color;
    }
    *_GPP += F("' value='");
    *_GPP += value;
    *_GPP += F("' min='");
    *_GPP += min;
    *_GPP += F("' max='");
    *_GPP += max;
    *_GPP += F("' step='");
    *_GPP += step;
    *_GPP += '\'';
    if (oninp) *_GPP += F(" checked");
    *_GPP += F(">\n<output align='center' id='");
    *_GPP += name;
    *_GPP += F("_val' name='");
    *_GPP += min_lable;
    *_GPP += ',';
    *_GPP += max_lable;
    if (!maxsz) {
      *_GPP += F("' style='background:");
      *_GPP += FPSTR(st);
      *_GPP += F(";");
    }
    *_GPP += F("' class='");
    if (maxsz) *_GPP += F("rangeValue");
    if (color.length()) *_GPP += F("rangeColor");
    if (dis) *_GPP += F(" dsbl");
    *_GPP += F("'></output>\n");
    send();
  }
  void SLIDER_C(const String& name, const String& min_lable, const String& max_lable, float value = 0, float min = 0, float max = 100, float step = 1, PGM_P st = GP_GREEN, bool dis = 0) {
    SLIDER(name, min_lable, max_lable, value, min, max, step, st, dis, 1);
  }

  void SLIDER_COLOR(const String& name, const String& color, float value = 0, float min = 0, float max = 100, PGM_P st = GP_GREEN, bool dis = 0) {
    SLIDER(name, "", "", value, min, max, 1, st, dis, 0, 0, "", color);
  }
  void SLIDER_COLOR_C(const String& name, const String& color, float value = 0, float min = 0, float max = 100, PGM_P st = GP_GREEN, bool dis = 0) {
    SLIDER(name, "", "", value, min, max, 1, st, dis, 1, 0, "", color);
  }

  void SLIDER_MAX(const String& lable, const String& min_lable, const String& max_lable, const String& name, float value = 0, float min = 0, float max = 100, float step = 1, PGM_P st = GP_GREEN, bool dis = 0) {
    SLIDER(name, min_lable, max_lable, value, min, max, step, st, dis, 0, 1, lable);
  }
  void SLIDER_MAX_C(const String& lable, const String& min_lable, const String& max_lable, const String& name, float value = 0, float min = 0, float max = 100, float step = 1, PGM_P st = GP_GREEN, bool dis = 0) {
    SLIDER(name, min_lable, max_lable, value, min, max, step, st, dis, 1, 1, lable);
  }

  void SPINNER_BTN(const String& name, float step, PGM_P st, uint8_t dec, bool dis) {
    *_GPP += F("<input type='button' class='spinBtn ");
    *_GPP += (step > 0) ? F("spinR") : F("spinL");
    *_GPP += F("' name='");
    *_GPP += name;
    *_GPP += F("' min='");
    *_GPP += step;
    *_GPP += F("' max='");
    *_GPP += dec;
    *_GPP += F("' onmouseleave='if(_pressId)clearInterval(_spinInt);_spinF=_pressId=null' onmousedown='_pressId=this.name;_spinInt=setInterval(()=>{EVspin(this);_spinF=1},");
    *_GPP += _spin_prd;
    *_GPP += F(")' onmouseup='clearInterval(_spinInt)' onclick='if(!_spinF)EVspin(this);_spinF=0' value='");
    *_GPP += (step > 0) ? '+' : '-';
    *_GPP += F("' ");
    *_GPP += F(" style='background:");
    *_GPP += FPSTR(st);
    *_GPP += F(";'");
    if (dis) *_GPP += F(" disabled");
    *_GPP += F(">\n");
  }
  void SPINNER(const String& name, float value = 0, float min = NAN, float max = NAN, float step = 1, uint16_t dec = 0, PGM_P st = GP_GREEN, const String& w = "", bool dis = 0) {
    *_GPP += F("<div id='spinner' class='spinner'>\n");
    SPINNER_BTN(name, -step, st, dec, dis);
    *_GPP += F("<input type='number' name='");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    if (w.length()) {
      *_GPP += F("' style='width:");
      *_GPP += w;
    }
    *_GPP += F("' step='");
    floatDec(step, dec);
    *_GPP += F("' onkeyup='EVspinw(this)' onkeydown='EVspinw(this)' onchange='");
    if (!dec) *_GPP += F("EVspinc(this);");
    *_GPP += F("EVclick(this);EVspinw(this)' value='");
    floatDec(value, dec);
    if (!isnan(min)) {
      *_GPP += F("' min='");
      floatDec(min, dec);
    }
    if (!isnan(max)) {
      *_GPP += F("' max='");
      floatDec(max, dec);
    }
    *_GPP += F("' ");
    if (dis) *_GPP += F("disabled ");
    if (!w.length()) *_GPP += F("class='spin_inp'");
    *_GPP += F(">\n");
    SPINNER_BTN(name, step, st, dec, dis);
    *_GPP += F("</div>\n");
    send();
  }

  void COLOR(const String& name, uint32_t value = 0, bool dis = false) {
    *_GPP += F("<input type='color' name='");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += F("' value='");
    GPcolor col(value);
    *_GPP += col.encode();
    *_GPP += "' ";
    if (dis) *_GPP += F("disabled ");
    *_GPP += F("onchange='EVclick(this)'>\n");
    send();
  }
  void COLOR(const String& name, GPcolor col, bool dis = false) {
    *_GPP += F("<input type='color' name='");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += F("' value='");
    *_GPP += col.encode();
    *_GPP += "' ";
    if (dis) *_GPP += F("disabled ");
    *_GPP += F("onchange='EVclick(this)'>\n");
    send();
  }

  void RADIO(const String& name, int num, const String& value = "", int val = -1, PGM_P st = GP_DEFAULT, bool dis = 0) {
    if (st != GP_DEFAULT) {
      *_GPP += F("<style>.rad_");
      *_GPP += name;
      *_GPP += F(":after{border-color:");
      *_GPP += FPSTR(st);
      *_GPP += F("}.rad_");
      *_GPP += name;
      *_GPP += F(":checked:after{background-color:");
      *_GPP += FPSTR(st);
      *_GPP += F("}</style>\n");
    }

    *_GPP += F("<div class='radBlock'><input class='rad rad_");
    *_GPP += name;
    if (dis) *_GPP += F(" dsbl");
    *_GPP += F("' type='radio' name='");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += '_';
    *_GPP += num;
    *_GPP += F("' value='");
    *_GPP += num;
    *_GPP += F("' onchange='EVclick(this)'");
    if (val == num) *_GPP += F(" checked");
    if (dis) *_GPP += F(" disabled");
    *_GPP += F(">\n");

    if (value.length()) {
      *_GPP += F("<label class='radLable' for='");
      *_GPP += name;
      *_GPP += '_';
      *_GPP += num;
      *_GPP += F("'>");
      *_GPP += value;
      *_GPP += F("</label>\n");
    }
    *_GPP += "</div>\n";

    send();
  }

  void SELECT(const String& name, const String& list, int sel = 0, bool nums = 0, bool dis = 0, bool rel = 0) {
    if (sel < 0) sel = 0;
    *_GPP += F("<select name='");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += "' ";
    if (dis) *_GPP += F("disabled ");
    *_GPP += F("onchange='EVclick(this,");
    *_GPP += rel;
    *_GPP += F(")'>\n");

    GP_parser p(list);
    int idx = 0;
    while (p.parse()) {
      *_GPP += F("<option value='");
      *_GPP += idx;
      *_GPP += "'";
      if (p.count == sel) *_GPP += F(" selected");
      *_GPP += F(">");
      if (nums) {
        *_GPP += idx;
        *_GPP += ". ";
      }
      *_GPP += p.str;
      *_GPP += F("</option>\n");
      idx++;
    }
    *_GPP += F("</select>\n");
    send();
  }
  void SELECT(const String& name, String* list, int sel = 0, bool nums = 0, bool dis = false, bool rel = 0) {
    if (sel < 0) sel = 0;
    *_GPP += F("<select name='");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += "' ";
    if (dis) *_GPP += F("disabled ");
    *_GPP += F("onchange='EVclick(this,");
    *_GPP += rel;
    *_GPP += F(")'>\n");
    int idx = 0;
    while (list[idx].length()) {
      *_GPP += F("<option value='");
      *_GPP += idx;
      *_GPP += "'";
      if (idx == sel) *_GPP += F(" selected");
      *_GPP += F(">");
      if (nums) {
        *_GPP += idx;
        *_GPP += ". ";
      }
      *_GPP += list[idx];
      *_GPP += F("</option>\n");
      idx++;
    }
    *_GPP += F("</select>\n");
    send();
  }
  void SELECT(const String& name, char** list, int sel = 0, bool nums = 0, bool dis = false, bool rel = 0) {
    if (sel < 0) sel = 0;
    *_GPP += F("<select name='");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += "' ";
    if (dis) *_GPP += F("disabled ");
    *_GPP += F("onchange='EVclick(this,");
    *_GPP += rel;
    *_GPP += F(")'>\n");
    int idx = 0;
    while (list[idx] != nullptr) {
      *_GPP += F("<option value='");
      *_GPP += idx;
      *_GPP += "'";
      if (idx == sel) *_GPP += F(" selected");
      *_GPP += F(">");
      if (nums) {
        *_GPP += idx;
        *_GPP += ". ";
      }
      *_GPP += list[idx];
      *_GPP += F("</option>\n");
      idx++;
    }
    *_GPP += F("</select>\n");
    send();
  }

  void SELECT_LIST(const String& name, const String& list, int sel = 0, bool nums = 0, bool dis = 0, bool rel = 0) {
    if (sel < 0) sel = 0;
    *_GPP += F("<input type='select' name='");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += F("' step='");
    *_GPP += rel;
    *_GPP += F("' min='");
    *_GPP += nums;
    *_GPP += F("' max='");
    *_GPP += sel;
    *_GPP += F("' placeholder='");
    *_GPP += list;
    *_GPP += F("' onclick='selectList(this)' readonly");
    if (dis) *_GPP += F(" disabled\n");
    *_GPP += F(">\n");
    send();
  }
  void SELECT_LIST_STYLE(PGM_P st_1, PGM_P st_2 = GP_GREEN) {
    *_GPP += F("<style>.selActive{background:");
    *_GPP += FPSTR(st_1);
    *_GPP += F("!important;}");
    if (st_2 != GP_GREEN) {
      *_GPP += F(".selBlock{border:2px solid ");
      *_GPP += FPSTR(st_2);
      *_GPP += F("!important;}");
    }
    *_GPP += F("</style>\n");
  }

  // ======================= ГРАФИКИ =======================
  void PLOT_STOCK_BEGIN(boolean local = 0, boolean lang = 0) {
    if (local) *_GPP += F("<script src='/gp_data/PLOT_STOCK.js'></script>\n<script src='/gp_data/PLOT_STOCK_DARK.js'></script>\n<script src='/gp_data/PLOT_STOCK_EXPORT.js'></script>\n");
    else *_GPP += F("<script src='https://code.highcharts.com/stock/highstock.js'></script>\n<script src='https://code.highcharts.com/themes/dark-unica.js'></script>\n<script src='https://code.highcharts.com/modules/exporting.js'></script>\n");
    if (lang) *_GPP += F("<script>Highcharts.setOptions({lang:{contextButtonTitle:'Меню',viewFullscreen:'Во весь экран',exitFullscreen:'Свернуть',printChart:'Печать...',resetZoom:'Сбросить',resetZoomTitle:'Сбросить маштаб'}});</script>\n");
    send();
  }

  void PLOT_STOCK_ADD(uint32_t time, int16_t val, uint8_t dec) {
    *_GPP += '[';
    *_GPP += time;
    *_GPP += F("000");
    *_GPP += ',';
    if (dec) *_GPP += (float)val / dec;
    else *_GPP += val;
    *_GPP += F("],\n");
    send();
  }
  void PLOT_STOCK_DARK(const String& id, const char** labels, uint32_t* times, int16_t* vals_0, int16_t* vals_1, uint8_t size, uint8_t type = 0, uint8_t dec = 0, uint16_t height = 400, PGM_P st_0 = GP_RED, PGM_P st_1 = GP_GREEN) {
    *_GPP += F("<div class='chartBlock' style='width:95%;height:");
    *_GPP += height;
    *_GPP += F("px' id='");
    *_GPP += id;
    *_GPP += F("'></div>");

    *_GPP += F("<script>Highcharts.setOptions({"
               "global:{buttonTheme:{height:12,fill:'#505053',stroke:'#505053',style:{color:'#DDDDDD'},"
               "states:{hover:{fill:'#737373'},select:{fill:'#505053'}}}},\ncolors:['"
              );

    *_GPP += FPSTR(st_0);
    *_GPP += F("','");
    *_GPP += FPSTR(st_1);
    *_GPP += F("']});\nHighcharts.chart('");
    *_GPP += id;
    *_GPP += F("',{chart:{type:'spline',borderRadius:10,panning:true,panKey:'shift',\n"
               "zooming:{type:'x',mouseWheel:false,resetButton:{position:{align:'left',verticalAlign:'top',x:43,y:9},relativeTo:'chart'}}},\n"

               "title:{style:{color:'#00000000'}},\n"
               "rangeSelector:'none',\n"

               "plotOptions:{series:{marker:{enabled:false},lineWidth:3.5}},\n"

               "tooltip:{crosshairs:true,shared:true,style:{fontSize:'0.95em'}},\n"
               "legend:{floating:true,backgroundColor:undefined,align:'right',verticalAlign:'top'},\n"

               "exporting:{buttons:{contextButton:{align:'left',x:0,y:-1,menuItems:['printChart','viewFullscreen']}}},\n"
               "navigation:{menuStyle:{background:'#E0E0E0'},menuItemStyle:{color:'#000000'}},\n"

               "time:{useUTC:true},\n"

               "xAxis:{type:'datetime'},\n"
               "yAxis:[{title:{enabled:false},tickLength:0,\n"
               "labels:{align:'left',x:3,y:16},\n"
               "showFirstLabel:false},\n"
               "{title:{enabled:false},tickLength:0,\n"
               "labels:{align:'right',x:-3,y:16},\n"
               "showFirstLabel:false,opposite:true}],\n"

               "credits:{enabled:false},series:[\n"
              );

    if (vals_0 != NULL) {
      *_GPP += F("{name:'");
      *_GPP += labels[0];
      *_GPP += F("',data:[\n");
      send();
      for (uint16_t s = 0; s < size; s++) {
        PLOT_STOCK_ADD(times[s], vals_0[s], dec);
      }
      *_GPP += F("],\n");
      if (type == 1) *_GPP += F("tooltip:{valueSuffix:'°C'}");
      else if (type == 2) *_GPP += F("tooltip:{valueSuffix:'mm.Hg'}");
      *_GPP += F("},\n");
    }
    if (vals_1 != NULL) {
      *_GPP += F("{name:'");
      *_GPP += labels[1];
      *_GPP += F("',data:[\n");
      send();
      if (type == 1) dec = 0;
      for (uint16_t s = 0; s < size; s++) {
        PLOT_STOCK_ADD(times[s], vals_1[s], dec);
      }
      *_GPP += F("],\n");
      if (type == 1) *_GPP += F("tooltip:{valueSuffix:'%'},\n");
      *_GPP += F("marker:{symbol:'circle'},yAxis:1},\n");
    }
    *_GPP += F("]});</script>\n");
    send();
  }

  // ======================= ВЫВОД ЛОГА =======================

  void AREA_LOG(const String& name, int rows = 5, int size = 12, int prd = 1000, const String& w = "") {
    *_GPP += F("<div class='inliner'><textarea name='_gplog' style='height:auto;");
    *_GPP += F("font-size:");
    *_GPP += size;
    *_GPP += F("px;");
    if (w.length()) {
      *_GPP += F("width:");
      *_GPP += w;
    }
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += F("' rows='");
    *_GPP += rows;
    *_GPP += F("' disabled></textarea></div>\n");

    *_GPP += F("<script>EVupdate('");
    *_GPP += name;
    *_GPP += F("');\n");
    *_GPP += F("setInterval(()=>EVupdate('");
    *_GPP += name;
    *_GPP += F("'),");
    *_GPP += prd;
    *_GPP += F(");</script>\n");
    send();
  }

  void AREA_TEXT(const String& name, const String& place = "", const String& value = "", const String& width = "", int maxlength = 0, const String& pattern = "", bool dis = false) {
    *_GPP += F("<input type='text' class='areaText' name='");
    *_GPP += name;
    *_GPP += F("' id='");
    *_GPP += name;
    *_GPP += F("' value='");
    *_GPP += value;
    if (width.length()) {
      *_GPP += F("' style='width:");
      *_GPP += width;
    }
    *_GPP += F("' placeholder='");
    *_GPP += place;
    *_GPP += F("' onchange='EVclick(this)'");
    if (dis) *_GPP += F(" disabled");
    if (maxlength) {
      *_GPP += F(" maxlength=");
      *_GPP += maxlength;
    }
    if (pattern.length()) {
      *_GPP += F(" pattern=");
      *_GPP += pattern;
    }
    *_GPP += ">\n";

    if (dis) {
      *_GPP += F("<script>setInterval(()=>textBlink('");
      *_GPP += name;
      *_GPP += F("'),500);</script>\n");
    }
    send();
  }

  void AREA_BUTTON(const String& name, const String& value, const String& tar = "", PGM_P st = GP_GREEN, const String& width = "", bool dis = 0, bool rel = 0) {
    BUTTON_RAW(name, value, tar, st, width, F("areaButton"), dis, rel);
  }
};
