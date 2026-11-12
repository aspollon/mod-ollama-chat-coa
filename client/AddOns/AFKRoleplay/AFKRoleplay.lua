-- AFK Roleplay: roleplay sheets for players and bots. The realm (mod-ollama-chat-coa) keeps them and has the
-- language model write the bots'; this add-on asks for them over hidden "AFKRP" addon whispers to yourself.
local PREFIX = "AFKRP"
local de = GetLocale() == "deDE"
local L = de and {
  NA = "Name", TI = "Titel", AG = "Alter", HO = "Herkunft", AP = "Aussehen", PE = "Wesen", HI = "Geschichte", CU = "Gerade",
  mine = "Mein Bogen", save = "Speichern", cancel = "Abbrechen", writing = "Wird gerade geschrieben ... (das kann eine Minute dauern)",
  none = "Kein Rollenspielbogen.", gone = "Nicht in Reichweite oder offline.", failed = "Konnte nicht geschrieben werden. Versuch es später noch einmal.",
  busy = "Die Sprach-KI ist gerade ausgelastet. Versuch es gleich noch einmal.", asking = "Frage den Bogen ab ...",
  saved = "Dein Bogen ist gespeichert. Bots, die mit dir sprechen, kennen ihn jetzt.", edit = "Mein Rollenspielbogen",
  help = "/rp (Ziel) | /rp edit | /rp bonds | /rp <Name>", nothing = "Du hast noch keinen Bogen. /rp edit legt ihn an.",
  bonds = "Beziehungen", nobonds = "Noch kennt dich kein Bot. Sprich mit ihnen!", talks = "Gespräche", shared = "Gemeinsam erlebt",
  last = "Zuletzt", affinity = "Zuneigung", romance = "Romantik", groupGood = "Freunde und Vertraute", groupMid = "Bekannte",
  groupBad = "Abneigung und Feindschaft",
  stranger = "Fremd", enemy = "Feind", dislike = "Kann dich nicht leiden", distrust = "Misstraut dir", know = "Bekannt",
  like = "Mag dich", friend = "Freund", close = "Enger Freund", drawn = "Fühlt sich zu dir hingezogen", beloved = "Geliebt",
} or {
  NA = "Name", TI = "Title", AG = "Age", HO = "Home", AP = "Looks", PE = "Manner", HI = "History", CU = "Right now",
  mine = "My sheet", save = "Save", cancel = "Cancel", writing = "Being written ... (this can take a minute)",
  none = "No roleplay sheet.", gone = "Out of reach or offline.", failed = "Could not be written. Try again later.",
  busy = "The language model is busy. Try again in a moment.", asking = "Asking for the sheet ...",
  saved = "Your sheet is saved. Bots who talk to you know it now.", edit = "My roleplay sheet",
  help = "/rp (target) | /rp edit | /rp bonds | /rp <name>", nothing = "You have no sheet yet. /rp edit makes one.",
  bonds = "Bonds", nobonds = "No bot knows you yet. Go and talk to them!", talks = "Talks", shared = "Been through together",
  last = "Last time", affinity = "Affection", romance = "Romance", groupGood = "Friends and confidants", groupMid = "Acquaintances",
  groupBad = "Dislike and enmity",
  stranger = "Stranger", enemy = "Enemy", dislike = "Dislikes you", distrust = "Distrusts you", know = "Acquaintance",
  like = "Likes you", friend = "Friend", close = "Close friend", drawn = "Drawn to you", beloved = "Beloved",
}
local FIELDS = { "NA", "TI", "AG", "HO", "AP", "PE", "HI", "CU" }
local MAX = { NA = 40, TI = 80, AG = 40, HO = 80, AP = 1200, PE = 1200, HI = 3000, CU = 300 }

local sheets, titles, buf, peeked, bonds = {}, {}, {}, {}, {}
local queue = {}
local viewName
local function me() return (UnitName("player")) end
local function size(f, w, h) f:SetWidth(w); f:SetHeight(h) end
local function say(msg) DEFAULT_CHAT_FRAME:AddMessage("|cffd0a060AFK Roleplay|r: " .. msg) end

-- the text as it travels: newlines as '~'; bytes cut without splitting a UTF-8 character
local function encode(s) return (s or ""):gsub("\r", ""):gsub("\t", " "):gsub("~", "-"):gsub("\n", "~") end
local function decode(s) return (s or ""):gsub("~", "\n") end
local function cut(s, n)
  if #s <= n then return s end
  local i = n
  while i > 0 do
    local b = s:byte(i + 1)
    if not b or b < 128 or b >= 192 then break end
    i = i - 1
  end
  return s:sub(1, i)
end

local function send(body) queue[#queue + 1] = body end
local sender = CreateFrame("Frame")
local wait = 0
sender:SetScript("OnUpdate", function(_, dt)
  wait = wait - dt
  if wait > 0 or #queue == 0 then return end
  wait = 0.12
  SendAddonMessage(PREFIX, table.remove(queue, 1), "WHISPER", me())
end)

-- ---------------------------------------------------------------- the sheet window
local function backdrop(f)
  f:SetBackdrop({ bgFile = "Interface\\DialogFrame\\UI-DialogBox-Background", edgeFile = "Interface\\DialogFrame\\UI-DialogBox-Border",
                  tile = true, tileSize = 32, edgeSize = 32, insets = { left = 11, right = 12, top = 12, bottom = 11 } })
end

local view = CreateFrame("Frame", "AFKRP_View", UIParent)
size(view, 440, 500); view:SetPoint("CENTER", -160, 40); view:SetFrameStrata("DIALOG"); view:Hide()
view:EnableMouse(true); view:SetMovable(true); view:RegisterForDrag("LeftButton")
view:SetScript("OnDragStart", view.StartMoving); view:SetScript("OnDragStop", view.StopMovingOrSizing)
backdrop(view); tinsert(UISpecialFrames, "AFKRP_View")
CreateFrame("Button", nil, view, "UIPanelCloseButton"):SetPoint("TOPRIGHT", -6, -6)
view.name = view:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge"); view.name:SetPoint("TOP", 0, -22)
view.title = view:CreateFontString(nil, "OVERLAY", "GameFontHighlight"); view.title:SetPoint("TOP", view.name, "BOTTOM", 0, -4)
view.title:SetTextColor(1, 0.82, 0)
-- ---------------------------------------------------------------- bonds, shown like reputation
-- each step: the affection range its bar fills (romance for drawn/beloved) and its colour (the reputation colours)
local STEP = {
  enemy = { -100, -60, 1 }, dislike = { -60, -25, 2 }, distrust = { -25, -5, 3 }, stranger = { 0, 0, 4 },
  know = { -5, 15, 4 }, like = { 15, 40, 5 }, friend = { 40, 70, 6 }, close = { 70, 100, 8 },
  drawn = { 30, 60, "pink" }, beloved = { 60, 100, "pink" },
}
local function stepColor(st)
  local c = (STEP[st] or STEP.stranger)[3]
  if c == "pink" then return 1, 0.45, 0.75 end
  local fc = FACTION_BAR_COLORS and FACTION_BAR_COLORS[c]
  if fc then return fc.r, fc.g, fc.b end
  return 0.9, 0.7, 0
end
local function fillBar(bar, b)
  local st = b and b.standing or "stranger"
  local lo, hi = unpack(STEP[st] or STEP.stranger)
  local v = (st == "drawn" or st == "beloved") and b.romance or (b and b.affinity or 0)
  bar:SetMinMaxValues(0, 1)
  bar:SetValue(hi > lo and math.max(0, math.min(1, (v - lo) / (hi - lo))) or 0)
  bar:SetStatusBarColor(stepColor(st))
  bar.text:SetText(L[st] or st)
  bar.bond = b
end
local function bondTooltip(owner, name, b)
  GameTooltip:SetOwner(owner, "ANCHOR_RIGHT")
  GameTooltip:AddLine(name, 1, 1, 1)
  GameTooltip:AddLine(L[b.standing] or b.standing, stepColor(b.standing))
  GameTooltip:AddDoubleLine(L.affinity, b.affinity .. " / 100", 1, 0.82, 0, 1, 1, 1)
  if (b.romance or 0) > 0 then GameTooltip:AddDoubleLine(L.romance, b.romance .. " / 100", 1, 0.82, 0, 1, 1, 1) end
  GameTooltip:AddDoubleLine(L.talks, b.talks or 0, 1, 0.82, 0, 1, 1, 1)
  if (b.shared or 0) > 0 then GameTooltip:AddDoubleLine(L.shared, b.shared, 1, 0.82, 0, 1, 1, 1) end
  if b.topic and b.topic ~= "" then GameTooltip:AddLine(L.last .. ": " .. b.topic, 0.8, 0.8, 0.8, true) end
  GameTooltip:Show()
end
local function makeBar(parent, w)
  local bar = CreateFrame("StatusBar", nil, parent)
  size(bar, w, 14); bar:SetStatusBarTexture("Interface\\TargetingFrame\\UI-StatusBar")
  local bg = bar:CreateTexture(nil, "BACKGROUND"); bg:SetAllPoints(); bg:SetTexture(0, 0, 0, 0.5)
  bar.text = bar:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall"); bar.text:SetPoint("CENTER")
  bar:EnableMouse(true)
  bar:SetScript("OnLeave", function() GameTooltip:Hide() end)
  return bar
end

view.bond = makeBar(view, 260); view.bond:SetPoint("TOP", view.title, "BOTTOM", 0, -6); view.bond:Hide()
view.bond:SetScript("OnEnter", function(self) if self.bond then bondTooltip(self, viewName, self.bond) end end)

local vscroll = CreateFrame("ScrollFrame", "AFKRP_ViewScroll", view, "UIPanelScrollFrameTemplate")
vscroll:SetPoint("TOPLEFT", 22, -84); vscroll:SetPoint("BOTTOMRIGHT", -40, 48)
local vchild = CreateFrame("Frame", nil, vscroll); size(vchild, 370, 10); vscroll:SetScrollChild(vchild)
view.body = vchild:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
view.body:SetPoint("TOPLEFT"); view.body:SetWidth(370); view.body:SetJustifyH("LEFT"); view.body:SetJustifyV("TOP")
local mineBtn = CreateFrame("Button", nil, view, "UIPanelButtonTemplate")
size(mineBtn, 120, 22); mineBtn:SetPoint("BOTTOMLEFT", 18, 16); mineBtn:SetText(L.mine)

local function section(code, text)
  if not text or text == "" then return "" end
  return "|cffffd100" .. L[code] .. "|r\n" .. text .. "\n\n"
end

local function refresh()
  if not viewName then return end
  local s = sheets[viewName] or {}
  local f = s.f or {}
  view.name:SetText((f.NA and f.NA ~= "") and (f.NA .. "  |cff888888(" .. viewName .. ")|r") or viewName)
  view.title:SetText(f.TI and f.TI ~= "" and ("<" .. f.TI .. ">") or "")
  if bonds[viewName] and viewName ~= me() then fillBar(view.bond, bonds[viewName]); view.bond:Show() else view.bond:Hide() end
  local text
  if s.state == "writing" then text = "|cff999999" .. L.writing .. "|r"
  elseif s.state == "asking" then text = "|cff999999" .. L.asking .. "|r"
  elseif s.state == "none" then text = L.none
  elseif s.state == "gone" then text = L.gone
  elseif s.state == "failed" then text = L.failed
  elseif s.state == "busy" then text = L.busy
  else
    text = section("AG", f.AG) .. section("HO", f.HO) .. section("AP", f.AP) .. section("PE", f.PE) .. section("HI", f.HI) .. section("CU", f.CU)
    if text == "" then text = viewName == me() and L.nothing or L.none end
  end
  view.body:SetText(text)
  vchild:SetHeight(view.body:GetStringHeight() + 10)
end

local function open(name)
  if not name or name == "" then return end
  viewName = name
  if name == me() then
    sheets[name] = { f = AFKRoleplayDB.sheet or {}, state = "done" }
  else
    sheets[name] = sheets[name] or { f = {} }
    sheets[name].state = "asking"
    bonds[name] = nil
    send("get\t" .. name)
  end
  refresh(); view:Show()
end
AFKRoleplay = { Open = open }

-- ---------------------------------------------------------------- every bond, like the reputation pane
local list = CreateFrame("Frame", "AFKRP_Bonds", UIParent)
size(list, 380, 460); list:SetPoint("CENTER", 220, 40); list:SetFrameStrata("DIALOG"); list:Hide()
list:EnableMouse(true); list:SetMovable(true); list:RegisterForDrag("LeftButton")
list:SetScript("OnDragStart", list.StartMoving); list:SetScript("OnDragStop", list.StopMovingOrSizing)
backdrop(list); tinsert(UISpecialFrames, "AFKRP_Bonds")
CreateFrame("Button", nil, list, "UIPanelCloseButton"):SetPoint("TOPRIGHT", -6, -6)
local ltitle = list:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge"); ltitle:SetPoint("TOP", 0, -20); ltitle:SetText(L.bonds)
local lscroll = CreateFrame("ScrollFrame", "AFKRP_BondsScroll", list, "UIPanelScrollFrameTemplate")
lscroll:SetPoint("TOPLEFT", 22, -50); lscroll:SetPoint("BOTTOMRIGHT", -40, 22)
local lchild = CreateFrame("Frame", nil, lscroll); size(lchild, 310, 10); lscroll:SetScrollChild(lchild)
local lempty = lchild:CreateFontString(nil, "OVERLAY", "GameFontHighlight"); lempty:SetPoint("TOPLEFT", 4, -6)
lempty:SetWidth(300); lempty:SetJustifyH("LEFT"); lempty:SetText(L.nobonds)
local rows, headers, incoming = {}, {}, {}

local function row(i)
  if rows[i] then return rows[i] end
  local r = CreateFrame("Button", nil, lchild); size(r, 310, 20)
  r.name = r:CreateFontString(nil, "OVERLAY", "GameFontNormal"); r.name:SetPoint("LEFT", 10, 0); r.name:SetWidth(120)
  r.name:SetJustifyH("LEFT")
  r.bar = makeBar(r, 170); r.bar:SetPoint("RIGHT", -4, 0)
  r.bar:SetScript("OnEnter", function(self) bondTooltip(self, r.who, self.bond) end)
  r.bar:SetScript("OnMouseUp", function() open(r.who) end)
  r:SetScript("OnClick", function() open(r.who) end)
  rows[i] = r
  return r
end
local function header(i)
  if headers[i] then return headers[i] end
  local h = lchild:CreateFontString(nil, "OVERLAY", "GameFontNormal"); h:SetJustifyH("LEFT")
  headers[i] = h
  return h
end
local GROUP = { beloved = 1, drawn = 1, close = 1, friend = 1, like = 1, know = 2, stranger = 2, distrust = 3, dislike = 3, enemy = 3 }
local function showList(entries)
  for _, r in ipairs(rows) do r:Hide() end
  for _, h in ipairs(headers) do h:Hide() end
  if #entries == 0 then lempty:Show() else lempty:Hide() end
  local y, n, nh = -4, 0, 0
  for g, title in ipairs({ L.groupGood, L.groupMid, L.groupBad }) do
    local first = true
    for _, e in ipairs(entries) do
      if (GROUP[e.standing] or 2) == g then
        if first then
          first = false; nh = nh + 1
          local h = header(nh); h:ClearAllPoints(); h:SetPoint("TOPLEFT", 0, y); h:SetText(title); h:Show(); y = y - 20
        end
        n = n + 1
        local r = row(n); r.who = e.name; r.name:SetText(e.name); fillBar(r.bar, e)
        r:ClearAllPoints(); r:SetPoint("TOPLEFT", 0, y); r:Show(); y = y - 22
      end
    end
    if not first then y = y - 6 end
  end
  lchild:SetHeight(-y + 10)
end
local function openBonds()
  incoming = {}
  showList({})
  lempty:SetText(L.asking)
  send("bonds")
  list:Show()
end
local bondsBtn = CreateFrame("Button", nil, view, "UIPanelButtonTemplate")
size(bondsBtn, 120, 22); bondsBtn:SetPoint("LEFT", mineBtn, "RIGHT", 8, 0); bondsBtn:SetText(L.bonds)
bondsBtn:SetScript("OnClick", openBonds)

-- ---------------------------------------------------------------- your own sheet
local edit = CreateFrame("Frame", "AFKRP_Edit", UIParent)
size(edit, 480, 600); edit:SetPoint("CENTER", 200, 0); edit:SetFrameStrata("DIALOG"); edit:Hide()
edit:EnableMouse(true); edit:SetMovable(true); edit:RegisterForDrag("LeftButton")
edit:SetScript("OnDragStart", edit.StartMoving); edit:SetScript("OnDragStop", edit.StopMovingOrSizing)
backdrop(edit); tinsert(UISpecialFrames, "AFKRP_Edit")
CreateFrame("Button", nil, edit, "UIPanelCloseButton"):SetPoint("TOPRIGHT", -6, -6)
local etitle = edit:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge"); etitle:SetPoint("TOP", 0, -20); etitle:SetText(L.edit)
local boxes = {}
local y = -50
local function label(code, yy)
  local t = edit:CreateFontString(nil, "OVERLAY", "GameFontNormal"); t:SetPoint("TOPLEFT", 24, yy); t:SetText(L[code])
end
for _, code in ipairs({ "NA", "TI", "AG", "HO" }) do
  label(code, y - 4)
  local e = CreateFrame("EditBox", "AFKRP_Edit" .. code, edit, "InputBoxTemplate")
  size(e, 320, 20); e:SetPoint("TOPLEFT", 130, y); e:SetAutoFocus(false); e:SetMaxLetters(MAX[code])
  boxes[code] = e; y = y - 28
end
local heights = { AP = 70, PE = 70, HI = 130, CU = 40 }
for _, code in ipairs({ "AP", "PE", "HI", "CU" }) do
  label(code, y - 2); y = y - 18
  local holder = CreateFrame("Frame", nil, edit)
  holder:SetPoint("TOPLEFT", 22, y); size(holder, 430, heights[code])
  holder:SetBackdrop({ bgFile = "Interface\\Tooltips\\UI-Tooltip-Background", edgeFile = "Interface\\Tooltips\\UI-Tooltip-Border",
                       tile = true, tileSize = 16, edgeSize = 12, insets = { left = 3, right = 3, top = 3, bottom = 3 } })
  holder:SetBackdropColor(0, 0, 0, 0.6)
  local sc = CreateFrame("ScrollFrame", "AFKRP_EditScroll" .. code, holder, "UIPanelScrollFrameTemplate")
  sc:SetPoint("TOPLEFT", 6, -6); sc:SetPoint("BOTTOMRIGHT", -26, 6)
  local e = CreateFrame("EditBox", "AFKRP_Edit" .. code, sc)
  e:SetMultiLine(true); e:SetAutoFocus(false); e:SetFontObject(ChatFontNormal); e:SetWidth(390)
  e:SetScript("OnEscapePressed", e.ClearFocus)
  e:SetScript("OnCursorChanged", function(self, _, yy, _, h)
    local top, height = sc:GetVerticalScroll(), sc:GetHeight()
    if -yy < top then sc:SetVerticalScroll(-yy) elseif -yy + h > top + height then sc:SetVerticalScroll(-yy + h - height) end
  end)
  holder:EnableMouse(true); holder:SetScript("OnMouseDown", function() e:SetFocus() end)
  sc:SetScrollChild(e)
  boxes[code] = e; y = y - heights[code] - 8
end

local function upload(sheet)
  for _, code in ipairs(FIELDS) do
    local text = encode(cut(sheet[code] or "", MAX[code] * 2))
    local seq = 0
    repeat
      local chunk = cut(text, 190)
      text = text:sub(#chunk + 1)
      send("set\t" .. code .. "\t" .. seq .. "\t" .. (text == "" and "1" or "0") .. "\t" .. chunk)
      seq = seq + 1
    until text == ""
  end
end

local saveBtn = CreateFrame("Button", nil, edit, "UIPanelButtonTemplate")
size(saveBtn, 120, 22); saveBtn:SetPoint("BOTTOMRIGHT", -22, 18); saveBtn:SetText(L.save)
local cancelBtn = CreateFrame("Button", nil, edit, "UIPanelButtonTemplate")
size(cancelBtn, 120, 22); cancelBtn:SetPoint("RIGHT", saveBtn, "LEFT", -8, 0); cancelBtn:SetText(L.cancel)
cancelBtn:SetScript("OnClick", function() edit:Hide() end)
saveBtn:SetScript("OnClick", function()
  local s = {}
  for _, code in ipairs(FIELDS) do s[code] = boxes[code]:GetText() or "" end
  AFKRoleplayDB.sheet = s
  upload(s)
  edit:Hide(); say(L.saved)
  if viewName == me() then open(me()) end
end)
local function openEdit()
  local s = AFKRoleplayDB.sheet or {}
  for _, code in ipairs(FIELDS) do boxes[code]:SetText(s[code] or "") end
  edit:Show()
end
mineBtn:SetScript("OnClick", openEdit)

-- ---------------------------------------------------------------- buttons, tooltip, messages
local targetBtn = CreateFrame("Button", "AFKRP_TargetButton", TargetFrame, "UIPanelButtonTemplate")
size(targetBtn, 34, 18); targetBtn:SetPoint("TOPLEFT", TargetFrame, "TOPLEFT", 6, -4); targetBtn:SetText("RP"); targetBtn:Hide()
targetBtn:SetScript("OnClick", function() open((UnitName("target"))) end)

local function addInspectButton()
  if not InspectFrame or AFKRP_InspectButton then return end
  local b = CreateFrame("Button", "AFKRP_InspectButton", InspectFrame, "UIPanelButtonTemplate")
  size(b, 80, 20); b:SetPoint("TOPRIGHT", InspectFrame, "TOPRIGHT", -44, -44); b:SetText(de and "RP-Bogen" or "RP sheet")
  b:SetScript("OnClick", function() open((UnitName(InspectFrame.unit or "target"))) end)
end

GameTooltip:HookScript("OnTooltipSetUnit", function(self)
  local _, unit = self:GetUnit()
  if not unit or not UnitIsPlayer(unit) then return end
  local name = UnitName(unit)
  if not name then return end
  local t = titles[name]
  if t and t ~= "" then
    self:AddLine("<" .. t .. ">", 1, 0.82, 0); self:Show()
  elseif name ~= me() and (not peeked[name] or GetTime() - peeked[name] > 600) then
    peeked[name] = GetTime(); send("peek\t" .. name)
  end
end)

local function onMessage(msg)
  local cmd, rest = msg:match("^(%a+)\t(.*)$")
  if cmd == "f" then
    local name, code, seq, last, chunk = strsplit("\t", rest, 5)
    buf[name] = buf[name] or {}
    buf[name][code] = (seq == "0" and "" or (buf[name][code] or "")) .. (chunk or "")
    if last == "1" then
      sheets[name] = sheets[name] or { f = {} }
      sheets[name].f[code] = decode(buf[name][code]); buf[name][code] = nil
    end
  elseif cmd == "s" then
    local name, state = strsplit("\t", rest, 2)
    sheets[name] = sheets[name] or { f = {} }
    sheets[name].state = state
    if state == "done" or state == "bot" then titles[name] = sheets[name].f.TI or "" end
    if name == viewName then refresh() end
  elseif cmd == "b" then
    local name, st, aff, rom, talks, shared, topic = strsplit("\t", rest, 7)
    bonds[name] = { standing = st, affinity = tonumber(aff) or 0, romance = tonumber(rom) or 0, talks = tonumber(talks) or 0,
                    shared = tonumber(shared) or 0, topic = decode(topic) }
    if name == viewName then refresh() end
  elseif cmd == "bl" then
    local name, st, aff, rom, talks, shared = strsplit("\t", rest, 6)
    incoming[#incoming + 1] = { name = name, standing = st, affinity = tonumber(aff) or 0, romance = tonumber(rom) or 0,
                                talks = tonumber(talks) or 0, shared = tonumber(shared) or 0 }
  elseif cmd == "be" then
    lempty:SetText(L.nobonds)
    showList(incoming)
  elseif cmd == "t" then
    local name, title = strsplit("\t", rest, 2)
    titles[name] = title or ""
  end
end

local ev = CreateFrame("Frame")
ev:RegisterEvent("ADDON_LOADED"); ev:RegisterEvent("CHAT_MSG_ADDON"); ev:RegisterEvent("PLAYER_TARGET_CHANGED")
ev:RegisterEvent("PLAYER_ENTERING_WORLD")
local uploaded = false
ev:SetScript("OnEvent", function(_, event, a1, a2, a3, a4)
  if event == "ADDON_LOADED" then
    if a1 == "AFKRoleplay" then AFKRoleplayDB = AFKRoleplayDB or {} end
    if a1 == "Blizzard_InspectUI" then addInspectButton() end
  elseif event == "CHAT_MSG_ADDON" then
    if a1 == PREFIX and a3 == "WHISPER" and a4 == me() then onMessage(a2) end
  elseif event == "PLAYER_TARGET_CHANGED" then
    if UnitIsPlayer("target") then targetBtn:Show() else targetBtn:Hide() end
  elseif event == "PLAYER_ENTERING_WORLD" and not uploaded then
    uploaded = true
    if InspectFrame then addInspectButton() end
    local s = AFKRoleplayDB and AFKRoleplayDB.sheet
    if s then for _, code in ipairs(FIELDS) do if (s[code] or "") ~= "" then upload(s); break end end end
  end
end)

SLASH_AFKROLEPLAY1 = "/rp"
SlashCmdList.AFKROLEPLAY = function(input)
  input = (input or ""):match("^%s*(.-)%s*$")
  if input == "edit" then openEdit()
  elseif input == "bonds" then openBonds()
  elseif input == "help" or input == "?" then say(L.help)
  elseif input ~= "" then open(input)
  elseif UnitIsPlayer("target") then open((UnitName("target")))
  else open(me()) end
end
