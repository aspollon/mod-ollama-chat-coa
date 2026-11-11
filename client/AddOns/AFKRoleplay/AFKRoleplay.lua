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
  help = "/rp (Ziel) | /rp edit | /rp <Name>", nothing = "Du hast noch keinen Bogen. /rp edit legt ihn an.",
} or {
  NA = "Name", TI = "Title", AG = "Age", HO = "Home", AP = "Looks", PE = "Manner", HI = "History", CU = "Right now",
  mine = "My sheet", save = "Save", cancel = "Cancel", writing = "Being written ... (this can take a minute)",
  none = "No roleplay sheet.", gone = "Out of reach or offline.", failed = "Could not be written. Try again later.",
  busy = "The language model is busy. Try again in a moment.", asking = "Asking for the sheet ...",
  saved = "Your sheet is saved. Bots who talk to you know it now.", edit = "My roleplay sheet",
  help = "/rp (target) | /rp edit | /rp <name>", nothing = "You have no sheet yet. /rp edit makes one.",
}
local FIELDS = { "NA", "TI", "AG", "HO", "AP", "PE", "HI", "CU" }
local MAX = { NA = 40, TI = 80, AG = 40, HO = 80, AP = 1200, PE = 1200, HI = 3000, CU = 300 }

local sheets, titles, buf, peeked = {}, {}, {}, {}
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
local vscroll = CreateFrame("ScrollFrame", "AFKRP_ViewScroll", view, "UIPanelScrollFrameTemplate")
vscroll:SetPoint("TOPLEFT", 22, -70); vscroll:SetPoint("BOTTOMRIGHT", -40, 48)
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
    send("get\t" .. name)
  end
  refresh(); view:Show()
end
AFKRoleplay = { Open = open }

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
  elseif input == "help" or input == "?" then say(L.help)
  elseif input ~= "" then open(input)
  elseif UnitIsPlayer("target") then open((UnitName("target")))
  else open(me()) end
end
