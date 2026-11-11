#include "mod-ollama-chat_rp.h"
#include "mod-ollama-chat_api.h"
#include "mod-ollama-chat_coa.h"
#include "mod-ollama-chat_config.h"
#include "mod-ollama-chat_dispatch.h"
#include "mod-ollama-chat_personality.h"
#include "mod-ollama-chat_world.h"

#include "Chat.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "GameTime.h"
#include "Group.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include <cctype>
#include <map>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Design and wire format: docs/2026-11-11-roleplay-sheets.md
namespace
{
    char const* const PREFIX = "AFKRP";

    bool     g_enable = true, g_pregen = true, g_inChat = true;
    float    g_range = 40.0f;
    uint32_t g_perHour = 20;
    std::string g_language;

    struct Sheet
    {
        std::map<std::string, std::string> f;   // NA TI AG HO AP PE HI CU
        bool bot = false;
    };

    // field codes, their longest text, and the order the client shows them
    std::vector<std::pair<std::string, size_t>> const FIELDS = {
        { "NA", 40 }, { "TI", 80 }, { "AG", 40 }, { "HO", 80 }, { "AP", 1200 }, { "PE", 1200 }, { "HI", 3000 }, { "CU", 300 } };

    std::unordered_map<uint64_t, Sheet> g_sheets;                       // world thread
    std::unordered_set<uint64_t> g_pending, g_dirty, g_failed;
    std::unordered_map<uint64_t, std::vector<uint64_t>> g_waiting;      // bot -> players who asked
    std::unordered_map<uint64_t, std::map<std::string, std::string>> g_upload;   // player -> field being sent
    bool g_loaded = false;
    uint32_t g_tick = 0, g_flush = 0, g_hourTimer = 0, g_madeThisHour = 0;

    struct Done { uint64_t guid; bool ok; Sheet sheet; };
    std::mutex g_doneMutex;
    std::vector<Done> g_done;                                           // worker -> world

    size_t MaxOf(std::string const& code)
    {
        for (auto const& [c, n] : FIELDS)
            if (c == code)
                return n;
        return 0;
    }

    // UTF-8 safe cut at n bytes
    std::string Cut(std::string const& s, size_t n)
    {
        if (s.size() <= n)
            return s;
        while (n > 0 && (static_cast<unsigned char>(s[n]) & 0xC0) == 0x80)
            --n;
        return s.substr(0, n);
    }

    std::string Encode(std::string s)    // newlines travel as '~', tabs would split the message
    {
        for (char& c : s)
            if (c == '\n') c = '~'; else if (c == '\t' || c == '\r') c = ' ';
        return s;
    }

    std::string Decode(std::string s)
    {
        for (char& c : s)
            if (c == '~') c = '\n'; else if (c == '\t' || c == '\r') c = ' ';
        return s;
    }

    std::vector<std::string> Split(std::string const& s, size_t parts)
    {
        std::vector<std::string> out;
        size_t start = 0;
        while (out.size() + 1 < parts)
        {
            size_t tab = s.find('\t', start);
            if (tab == std::string::npos)
                break;
            out.push_back(s.substr(start, tab - start));
            start = tab + 1;
        }
        out.push_back(s.substr(start));
        return out;
    }

    void Send(Player* to, std::string const& body)
    {
        if (!to || !to->GetSession())
            return;
        WorldPacket data;
        ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER, LANG_ADDON, to, to, std::string(PREFIX) + "\t" + body);
        to->SendDirectMessage(&data);
    }

    void SendSheet(Player* to, std::string const& name, Sheet const& sheet)
    {
        for (auto const& [code, max] : FIELDS)
        {
            auto it = sheet.f.find(code);
            std::string text = it == sheet.f.end() ? std::string() : Encode(it->second);
            uint32_t seq = 0;
            do
            {
                std::string chunk = Cut(text, 180);
                text = text.substr(chunk.size());
                Send(to, "f\t" + name + "\t" + code + "\t" + std::to_string(seq++) + "\t" + (text.empty() ? "1" : "0") + "\t" + chunk);
            } while (!text.empty());
        }
        Send(to, "s\t" + name + "\t" + (sheet.bot ? "bot" : "done"));
    }

    void Save(uint64_t guid)
    {
        auto it = g_sheets.find(guid);
        if (it == g_sheets.end())
        {
            CharacterDatabase.Execute("DELETE FROM mod_ollama_chat_rp WHERE guid = {}", guid);
            return;
        }
        auto get = [&](char const* c)
        {
            auto f = it->second.f.find(c);
            std::string v = f == it->second.f.end() ? std::string() : f->second;
            CharacterDatabase.EscapeString(v);
            return v;
        };
        CharacterDatabase.Execute(
            "REPLACE INTO mod_ollama_chat_rp (guid, is_bot, rp_name, title, age, home, appearance, personality, history, currently, updated) "
            "VALUES ({}, {}, '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', {})",
            guid, it->second.bot ? 1 : 0, get("NA"), get("TI"), get("AG"), get("HO"), get("AP"), get("PE"), get("HI"), get("CU"),
            static_cast<uint32_t>(GameTime::GetGameTime().count()));
    }

    void Load()
    {
        g_loaded = true;
        QueryResult r = CharacterDatabase.Query(
            "SELECT guid, is_bot, rp_name, title, age, home, appearance, personality, history, currently FROM mod_ollama_chat_rp");
        if (!r)
            return;
        char const* codes[] = { "NA", "TI", "AG", "HO", "AP", "PE", "HI", "CU" };
        do
        {
            Field* f = r->Fetch();
            Sheet& s = g_sheets[f[0].Get<uint64_t>()];
            s.bot = f[1].Get<uint8_t>() != 0;
            for (int i = 0; i < 8; ++i)
                s.f[codes[i]] = f[2 + i].Get<std::string>();
        } while (r->NextRow());
        LOG_INFO("module.ollamachat", "[Ollama Chat] {} roleplay sheets loaded.", g_sheets.size());
    }

    bool SameStart(std::string const& s, std::string const& head)   // case-insensitive (MSVC has no strncasecmp)
    {
        if (s.size() < head.size())
            return false;
        for (size_t i = 0; i < head.size(); ++i)
            if (std::toupper(static_cast<unsigned char>(s[i])) != std::toupper(static_cast<unsigned char>(head[i])))
                return false;
        return true;
    }

    std::string ClassName(uint8_t cls)
    {
        ChrClassesEntry const* e = sChrClassesStore.LookupEntry(cls);
        return e && e->name[0] ? e->name[0] : "adventurer";
    }

    // The language model writes a bot's sheet once, from who the bot already is.
    void Request(Player* bot, uint64_t asker)
    {
        uint64_t const guid = bot->GetGUID().GetRawValue();
        if (asker)
            g_waiting[guid].push_back(asker);
        if (g_pending.count(guid))
            return;

        std::string const personality = GetPersonalityPromptAddition(GetBotPersonality(bot));
        std::string prompt =
            "Write a short roleplay character sheet for " + bot->GetName() + ", a " +
            (bot->getGender() == GENDER_FEMALE ? "female " : "male ") + Coa_RaceName(bot->getRace()) + " " +
            ClassName(bot->getClass()) + " of the " + (bot->GetTeamId() == TEAM_ALLIANCE ? "Alliance" : "Horde") +
            " in the world of Warcraft, in the time of the war against the Lich King. ";
        if (!personality.empty())
            prompt += "Their temperament: " + personality + " ";
        prompt += "Invent a believable life that fits their people, their calling and their temperament. Nothing about game "
                  "mechanics, levels or being a bot, and do not describe their looks. Answer with exactly these six lines "
                  "and nothing else:\n"
                  "TITLE: a short epithet or title, at most six words, without the name\n"
                  "AGE: a few words\n"
                  "HOME: where they come from\n"
                  "PERSONALITY: two or three sentences\n"
                  "HISTORY: a short backstory of four to six sentences\n"
                  "CURRENTLY: one sentence, what they are doing or seeking right now";
        if (!g_language.empty())
            prompt += "\nWrite the text after each label in " + g_language + "; keep the labels in English.";

        g_pending.insert(guid);
        bool const queued = OllamaDispatch_SubmitJob([guid, prompt]()
        {
            OllamaApiResult api = QueryOllama(prompt, OllamaRequestKind::Sentiment, 700);
            Done d{ guid, false, {} };
            if (api.ok)
            {
                std::map<std::string, std::string> labels = {
                    { "TITLE:", "TI" }, { "AGE:", "AG" }, { "HOME:", "HO" }, { "PERSONALITY:", "PE" }, { "HISTORY:", "HI" }, { "CURRENTLY:", "CU" } };
                std::string text = api.text, current;
                size_t pos = 0;
                while (pos <= text.size())
                {
                    size_t nl = text.find('\n', pos);
                    std::string line = text.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos);
                    pos = nl == std::string::npos ? text.size() + 1 : nl + 1;
                    size_t a = line.find_first_not_of(" *-#");
                    if (a == std::string::npos)
                        continue;
                    line = line.substr(a);
                    bool label = false;
                    for (auto const& [l, code] : labels)
                    {
                        if (line.size() >= l.size() && SameStart(line, l))
                        {
                            current = code;
                            std::string v = line.substr(l.size());
                            size_t b = v.find_first_not_of(" *");
                            d.sheet.f[code] = b == std::string::npos ? std::string() : v.substr(b);
                            label = true;
                            break;
                        }
                    }
                    if (!label && !current.empty())      // a field that runs over several lines
                        d.sheet.f[current] += " " + line;
                }
                for (auto& [code, v] : d.sheet.f)
                {
                    while (!v.empty() && (v.back() == ' ' || v.back() == '*'))
                        v.pop_back();
                    v = Cut(v, MaxOf(code));
                }
                d.ok = !d.sheet.f["TI"].empty() && !d.sheet.f["HI"].empty();
            }
            d.sheet.bot = true;
            std::lock_guard<std::mutex> lock(g_doneMutex);
            g_done.push_back(std::move(d));
        });
        if (!queued)
        {
            g_pending.erase(guid);
            for (uint64_t p : g_waiting[guid])
                if (Player* pl = ObjectAccessor::FindPlayer(ObjectGuid(p)))
                    Send(pl, "s\t" + bot->GetName() + "\tbusy");
            g_waiting.erase(guid);
        }
    }

    void Handle(Player* player, std::string const& body)
    {
        uint64_t const me = player->GetGUID().GetRawValue();
        std::vector<std::string> p = Split(body, 6);
        std::string const& cmd = p[0];

        if ((cmd == "get" || cmd == "peek") && p.size() >= 2)
        {
            std::string const& name = p[1];
            Player* who = ObjectAccessor::FindPlayerByName(name, false);
            if (!who)
            {
                Send(player, (cmd == "peek" ? "t\t" : "s\t") + name + (cmd == "peek" ? "\t" : "\tgone"));
                return;
            }
            auto it = g_sheets.find(who->GetGUID().GetRawValue());
            if (cmd == "peek")
            {
                std::string title = it == g_sheets.end() ? std::string() : it->second.f["TI"];
                Send(player, "t\t" + name + "\t" + title);
                return;
            }
            if (it != g_sheets.end())
                SendSheet(player, name, it->second);
            else if (OllamaIsBotPlayer(who) && g_enable)
            {
                Send(player, "s\t" + name + "\twriting");
                Request(who, me);
            }
            else
                Send(player, "s\t" + name + "\tnone");
        }
        else if (cmd == "set" && p.size() >= 5)
        {
            std::string const& code = p[1];
            if (!MaxOf(code))
                return;
            std::string& acc = g_upload[me][code];
            if (p[2] == "0")
                acc.clear();
            acc += p.size() >= 5 ? p[4] : std::string();
            if (p[3] == "1")
            {
                Sheet& s = g_sheets[me];
                s.bot = false;
                s.f[code] = Cut(Decode(acc), MaxOf(code));
                g_upload[me].erase(code);
                g_dirty.insert(me);
            }
        }
        else if (cmd == "clear")
        {
            g_sheets.erase(me);
            g_dirty.insert(me);
        }
    }

    // a bot without a sheet near a real player (group members first), while the language model has nothing to do
    void Pregenerate()
    {
        if (!g_pending.empty() || g_madeThisHour >= g_perHour)
            return;
        OllamaDispatchStats st = OllamaDispatch_GetStats();
        if (st.queuedRequests > 0 || st.inFlight > 0)
            return;
        Player* pick = nullptr;
        auto want = [](Player* b)
        {
            uint64_t g = b->GetGUID().GetRawValue();
            return b->IsInWorld() && OllamaIsBotPlayer(b) && !g_sheets.count(g) && !g_failed.count(g);
        };
        auto const& players = ObjectAccessor::GetPlayers();
        for (auto const& [guid, real] : players)
        {
            if (!OllamaIsRealPlayer(real) || !real->IsInWorld())
                continue;
            if (Group* group = real->GetGroup())
                for (GroupReference* ref = group->GetFirstMember(); ref && !pick; ref = ref->next())
                    if (Player* m = ref->GetSource(); m && m != real && want(m))
                        pick = m;
            if (pick)
                break;
        }
        if (!pick && !g_OnlyPartyBots)
            for (auto const& [guid, real] : players)
            {
                if (!OllamaIsRealPlayer(real) || !real->IsInWorld())
                    continue;
                for (auto const& [g2, b] : players)
                    if (b != real && want(b) && b->GetMapId() == real->GetMapId() && b->IsWithinDistInMap(real, g_range))
                    {
                        pick = b;
                        break;
                    }
                if (pick)
                    break;
            }
        if (pick)
        {
            ++g_madeThisHour;
            Request(pick, 0);
        }
    }

    std::string Short(std::string const& s, size_t n)
    {
        std::string out = Cut(s, n);
        if (out.size() < s.size())
            out += "...";
        for (char& c : out)
            if (c == '\n')
                c = ' ';
        return out;
    }
}

void Rp_LoadConfig()
{
    g_enable   = sConfigMgr->GetOption<bool>("OllamaChat.RP.Enable", true);
    g_pregen   = sConfigMgr->GetOption<bool>("OllamaChat.RP.Pregenerate", true);
    g_range    = sConfigMgr->GetOption<float>("OllamaChat.RP.PregenerateRange", 40.0f);
    g_perHour  = sConfigMgr->GetOption<uint32_t>("OllamaChat.RP.PregeneratePerHour", 20);
    g_inChat   = sConfigMgr->GetOption<bool>("OllamaChat.RP.UseSheetsInChat", true);
    g_language = sConfigMgr->GetOption<std::string>("OllamaChat.RP.Language", "");
}

void Rp_Update(uint32_t diff)
{
    if (!g_loaded)
        Load();

    std::vector<Done> done;
    {
        std::lock_guard<std::mutex> lock(g_doneMutex);
        done.swap(g_done);
    }
    for (Done& d : done)
    {
        g_pending.erase(d.guid);
        Player* bot = ObjectAccessor::FindPlayer(ObjectGuid(d.guid));
        std::string const name = bot ? bot->GetName() : std::string();
        if (d.ok)
        {
            g_sheets[d.guid] = d.sheet;
            Save(d.guid);
        }
        else
            g_failed.insert(d.guid);
        for (uint64_t p : g_waiting[d.guid])
            if (Player* pl = ObjectAccessor::FindPlayer(ObjectGuid(p)); pl && !name.empty())
            {
                if (d.ok)
                    SendSheet(pl, name, d.sheet);
                else
                    Send(pl, "s\t" + name + "\tfailed");
            }
        g_waiting.erase(d.guid);
    }

    g_flush += diff;
    if (g_flush >= 5000)
    {
        g_flush = 0;
        for (uint64_t g : g_dirty)
            Save(g);
        g_dirty.clear();
    }

    g_hourTimer += diff;
    if (g_hourTimer >= 3600000)
    {
        g_hourTimer = 0;
        g_madeThisHour = 0;
        g_failed.clear();     // the model may have been away
    }

    g_tick += diff;
    if (g_tick >= 20000)
    {
        g_tick = 0;
        if (g_enable && g_pregen)
            Pregenerate();
    }
}

std::string Rp_PromptSection(Player* bot, Player* player)
{
    if (!g_inChat || !bot)
        return std::string();
    std::string out;
    auto mine = g_sheets.find(bot->GetGUID().GetRawValue());
    if (mine != g_sheets.end() && mine->second.bot)
    {
        auto& f = mine->second.f;
        out += " Who you are, keep to it: " + f["TI"] + ". " + Short(f["PE"], 300) + " Your past: " + Short(f["HI"], 450) +
               (f["CU"].empty() ? std::string() : " Right now: " + Short(f["CU"], 150));
    }
    if (player && player != bot)
    {
        auto theirs = g_sheets.find(player->GetGUID().GetRawValue());
        if (theirs != g_sheets.end() && !theirs->second.bot)
        {
            auto& f = theirs->second.f;
            std::string about;
            if (!f["NA"].empty()) about += " Calls themselves " + Short(f["NA"], 40) + ".";
            if (!f["TI"].empty()) about += " Known as: " + Short(f["TI"], 80) + ".";
            if (!f["AG"].empty()) about += " Age: " + Short(f["AG"], 40) + ".";
            if (!f["HO"].empty()) about += " From: " + Short(f["HO"], 80) + ".";
            if (!f["AP"].empty()) about += " Looks: " + Short(f["AP"], 300);
            if (!f["PE"].empty()) about += " Manner: " + Short(f["PE"], 200);
            if (!f["HI"].empty()) about += " Their past, as far as people know: " + Short(f["HI"], 400);
            if (!f["CU"].empty()) about += " Right now: " + Short(f["CU"], 150);
            if (!about.empty())
                out += " What " + player->GetName() + " shows of themselves:" + about;
        }
    }
    return out.empty() ? out : out + " ";
}

void OllamaRpChat::OnPlayerBeforeSendChatMessage(Player* player, uint32& type, uint32& lang, std::string& msg)
{
    if (lang != LANG_ADDON || type != CHAT_MSG_WHISPER || !player || msg.compare(0, 6, "AFKRP\t") != 0)
        return;
    Handle(player, msg.substr(6));
}
